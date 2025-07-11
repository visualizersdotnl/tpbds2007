
// codename: secret desire

#include "main.h"
#include "DemoImpl.h"
#include "../utility/systemfont.h"
#include "../utility/draw2D.h"

// Rocket hooks.
static sync_cb s_rocketHooks = 
{ 
	Audio_Rocket_Pause, 
	Audio_Rocket_SetRow, 
	Audio_Rocket_IsPlaying 
};

// Corresponding size for each RenderTarget.
// 1 gives a quarter of the output resolution.
const unsigned int kDemoRenderTargetSizes[kNumDemoRenderTargets] = 
{
	0, // RT_BACK_BUFFER
	0, // RT_FULL_1,
	0, // RT_FULL_2,
	0, // RT_FULL_3,
	0, // RT_FULL_4,
	1, // RT_QUARTER_1,
	1, // RT_QUARTER_2,
	2, // RT_SIXTEENTH_1,
	2, // RT_SIXTEENTH_2
};

// settings
const std::wstring Demo::s_demoName(L"втихомолку желание");
const std::string Demo::s_mp3Path("content/desire.mp3");
const unsigned int Demo::s_xRes = 1280;
const unsigned int Demo::s_yRes = 720;
const std::string Demo::s_dispModeHint("Presented in 1280*720 (16:9).");
const float Demo::s_soundtrackBPM = 135.f; // desire.mp3
const unsigned int Demo::s_rocketRowsPerBeat = 32;

// Used with Demo::m_loadState.
enum DemoLoadState
{
	DEMO_NOT_LOADED  = 0,
	DEMO_LOADING     = 1,
	DEMO_LOADED      = 2
};

// ResourceHub archive switches.
const bool kBuildArchive = false;
const bool kLoadFromArchive = false;
const std::string kArchivePath = "secretdesire.wad";

// Blob parameters.
const unsigned int kNumBlob4s = 1; //18;     // *= 4
const unsigned int kBlobGridDepth = 16 ; // Max. 64
const float kBlobSpaceSize = 1.f; //3.5f;      // Size of cube.

/* static */ bool Demo::DoCompatibilityChecks(UINT iAdapter, IDirect3D9 &d3dObj, const D3DCAPS9 &caps)
{
	// caps.NumSimultaneousRTs

	// This is where we'll be checking if any out-of-the-ordinary feature we use will actually work.
	return true;
}

/* static */ Demo *Demo::Create(Renderer &renderer) { return new DemoImpl(renderer); }

DemoImpl::DemoImpl(Renderer &renderer) :
	Demo(renderer, DEMO_NOT_LOADED)
,	m_resHub(renderer)
,	m_blobs(renderer, kNumBlob4s, kBlobGridDepth, kBlobSpaceSize)
,	m_planeTracer(renderer, m_resHub)
,	m_tunnelTracer(renderer, m_resHub)
,	m_glowMesh(renderer, m_resHub)
,	m_gainShaders(m_resHub)
,	m_colorMatrixBlitter(renderer, m_resHub)
,	m_desaturateBlitter(renderer, m_resHub)
,	m_kawaseBlurBlitter(renderer, m_resHub)
,	m_hypnoglowBlitter(renderer, 1, m_resHub)
,	m_hRocket(NULL)
{
	SystemFont_RequestTexture(m_resHub);

	// erase DemoRenderTarget array before fiddling with it
	memset(m_demoRenderTargets, 0, sizeof(DemoRenderTargetDesc) * kNumDemoRenderTargets);

	// test resources
	m_pBlobTexture = m_resHub.RequestTexture("content/misc/wirenv.jpg", true, false);
	m_pBackgroundTexture = m_resHub.RequestTexture("content/misc/chamomile_fractal.jpg", false, true);

	// manual allocation(s)
	m_pBlobShaders = m_renderer.CreateShaderPair(g_vs30_utility_ss_blobs, g_ps30_utility_ss_blobs, flexBitsBlobs);
}

DemoImpl::~DemoImpl()
{
	// Free Rocket resources.
	if (m_hRocket != NULL)
		sync_destroy_device(m_hRocket);

	// Dump render targets.
	for (unsigned int iRT = 0; iRT < kNumDemoRenderTargets; ++iRT)
	{
		// Array has been erased on construction.
		delete m_demoRenderTargets[iRT].pRT; 
	}

	// Get rid of manual allocation(s).
	m_renderer.DestroyShaderPair(m_pBlobShaders);
}

bool DemoImpl::Load()
{
	TPB_ASSERT(m_loadState == DEMO_NOT_LOADED);
	m_loadState = DEMO_LOADING;

#if !IGNORE_ROCKET
	// Initialize Rocket.
	m_hRocket = sync_create_device("sync");
	if (m_hRocket == NULL)
	{
		SetLastError("Unable to initialize GNU Rocket.");
		return false;
	}

	// Grab Rocket sync. tracks.
	m_pKickSync = sync_get_track(m_hRocket, "KickSync");
	m_pKickSyncLerp = sync_get_track(m_hRocket, "KickSyncLerp");

#ifndef SYNC_PLAYER
	// Hook Rocket to audio layer.
	sync_set_callbacks(m_hRocket, &s_rocketHooks, NULL);
	
	if (0 != sync_connect(m_hRocket, "localhost", SYNC_DEFAULT_PORT))
	{
		SetLastError("Unable to connect to Rocket.");
		return false;
	}
#endif
#endif // !IGNORE_ROCKET

	// Load resources (blocking).
	while (!m_resHub.IsLoaded())
	{
		if (!m_resHub.Load(kLoadFromArchive, kArchivePath))
		{
			return false;
		}
		
		// Build archive if requested.
		if (kBuildArchive)
		{
			if (!m_resHub.WriteArchive(kArchivePath))
			{
				return false;
			}
		}
	}

	// Allocate render targets.
	for (unsigned int iRT = 0; iRT < kNumDemoRenderTargets; ++iRT)
	{
		const unsigned int mipLevel = kDemoRenderTargetSizes[iRT];
		m_demoRenderTargets[iRT].mipLevel = mipLevel;
		m_demoRenderTargets[iRT].pRT = new RenderTarget(m_renderer, mipLevel);
		if (iRT != RT_BACK_BUFFER) // The back buffer is a special case that remains unallocated.
		{
			if (!m_demoRenderTargets[iRT].pRT->AllocateSurface())
			{
				return false;
			}
		}
	}

	// Initialize effects & blitters.
	if (!m_blobs.AllocateBuffers()) return false;
	if (!m_planeTracer.Initialize()) return false;
	if (!m_glowMesh.Initialize()) return false;
	if (!m_kawaseBlurBlitter.AllocateSurfaces()) return false;
	if (!m_hypnoglowBlitter.AllocateSurfaces()) return false;

	m_loadState = DEMO_LOADED;
	return true;
}

bool DemoImpl::IsLoaded() const
{
	return m_loadState == DEMO_LOADED;
}

bool DemoImpl::RenderFrame(float time, float timeInBeats, float FPS, float systemTime)
{
	// -- update Rocket --

#if !IGNORE_ROCKET
	const double rocketRow = Audio_Rocket_GetRow();

#ifndef SYNC_PLAYER
	if (sync_update(m_hRocket, (int) floor(rocketRow)))
	{
		sync_connect(m_hRocket, "localhost", SYNC_DEFAULT_PORT);
	}
#endif

	const float kickSync = sync_get_val(m_pKickSync, rocketRow);
	const float kickSyncLerp = sync_get_val(m_pKickSyncLerp, rocketRow);
#else
	const float kickSync = 0.f; 
	const float kickSyncLerp = 0.f;
#endif // IGNORE_ROCKET

	// - render --

	m_renderer.BeginFrame(0);
	{
		// push back buffer to the top of the render target stack
		// no need to clear or anything, BeginFrame() just did so!
		m_renderTargetStack.Push(*m_demoRenderTargets[RT_BACK_BUFFER].pRT, false, false);

		// upload timer values to a pixel shader constant
		m_renderer.SetPixelShaderConstantV(PS_VTIMERS_CI, Vector4(time, time * 0.5f, timeInBeats, timeInBeats * 0.5f));

#if 1 // Test: simple tracer(s).
//		m_planeTracer.Draw(time, NULL);

		float toBox = kickSync; // fmodf(time * 0.25f, 2.f);
		if (toBox > 1.f) {
			toBox = 1.f - (toBox - 1.f);
		}

		const float tunnelParams[12] =
		{
			1.4f,        // radius
			time * 2.3f, // vScroll
			1.f,         // uTile
			0.1f,        // vTile
			0.06f,       // flowerScale
			6.f,         // flowerFreq
			time,        // flowerPhase
			0.1f,        // shadeDepthMul
			-0.1f,       // mipBias
			toBox,       // toBox
			0.f, 0.f     // unused
		};

		const Matrix4x4 mTunnel = Matrix4x4::Identity(); // Matrix4x4::RotationZ(timeInBeats * 0.25f * kPI);
//		m_tunnelTracer.Draw(time, tunnelParams, mTunnel);
#endif

#if 0 // Test: background image.
		m_renderTargetStack.Push(*m_demoRenderTargets[RT_FULL_1].pRT, true, false);
		{
			m_renderer.SetPolyFlags(kPolyFlagOpaque | kPolyFlagNoZBuffer);
			m_renderer.SetTexture(0, m_pBackgroundTexture->Get(), kTexFlagImageClamp);
			SpriteQuad(m_renderer, NULL, Vector2(0.f), Vector2(1.f), 0.f, Vector2(1.f), 0xffffffff);
		}
		m_renderTargetStack.Pop(false);

//		m_kawaseBlurBlitter.SetParameters(4, 1.5f);
//		m_demoRenderTargets[RT_FULL_1].pRT->Blit(m_kawaseBlurBlitter, kPolyFlagOpaque, 1.f);

		m_demoRenderTargets[RT_FULL_1].pRT->Blit(TileBlitter(m_renderer, 1.f, 1.f), kPolyFlagOpaque, 1.f);
#endif

#if 0 // Test: glow mesh.
		m_glowMesh.Draw(time);
#endif

#if 1 // Test: blobs with a hint of godrays.
		for (int iBlob4 = 0; iBlob4 < kNumBlob4s; ++iBlob4)
		{
			for (int iBlob = 0; iBlob < 4; ++iBlob)
			{
				// Acceptable, but not great.
				const float xOffs = (float) ( iBlob  * 43 + iBlob4 * 54 );
				const float yOffs = (float) ( iBlob4 * 36 - iBlob  * 16 );
				const float zOffs = (float) ( iBlob  * 19 + iBlob4 * 28 );
				m_blobs.m_pBlob4s[iBlob4].X[iBlob] = 0.25f*sinf(time + xOffs);
				m_blobs.m_pBlob4s[iBlob4].Y[iBlob] = 0.25f*cosf(time + yOffs);
				m_blobs.m_pBlob4s[iBlob4].Z[iBlob] = 0.25f*sinf(time + zOffs);
			}
		}

		m_blobs.Generate(40.f); //60.f + cosf(time * 0.5f) * 20.f);

//		Matrix4x4 mWorld = Matrix4x4::Identity();
		Matrix4x4 mWorld = Matrix4x4::RotationYPR(Vector3(time * 0.8f, time * 0.6f, time * 0.4f));
		m_renderer.SetVertexShaderConstantM4x4(VS_MWORLD_CI, mWorld);
		m_renderer.SetVertexShaderConstantM4x4(VS_MCUSTOM_CI, mWorld.AffineInverse());

		View view(m_renderer);
		view.m_camPos.m_Z = -4.f - kickSyncLerp;
		view.Update();
		view.UploadMatrices(true, &mWorld);

		m_renderer.SetVertexShaderConstantV(VS_STOCK_LIGHT_POS_CI, Vector3(0.f, 0.f, -5.f));
		m_renderer.SetVertexShaderConstantV(VS_STOCK_LIGHT_COLOR_CI, Vector3(1.f, 1.f, 1.f));

		m_renderer.SetPolyFlags(kPolyFlagOpaque);
//		m_renderer.SetTexture(0, NULL, kTexFlagDef);
		m_renderer.SetTexture(0, m_pBlobTexture->Get(), kTexFlagDef);
		m_renderer.SetShaders(m_pBlobShaders);

		m_renderTargetStack.Push(*m_demoRenderTargets[RT_QUARTER_1].pRT, true, true);
		{
			m_blobs.Draw();
		}
		m_renderTargetStack.Pop(true);

		m_blobs.Draw();

//		m_colorMatrixBlitter.SetHueShiftMatrix(time);
//		m_demoRenderTargets[RT_QUARTER_1].pRT->Blit(m_colorMatrixBlitter, kPolyFlagAdditive, 0.4f);

//		m_demoRenderTargets[RT_QUARTER_1].pRT->Blit(TileBlitter(m_renderer, 1.f, 1.f), kPolyFlagOpaque, 1.f);
		m_hypnoglowBlitter.SetParameters(8, 1.05f, 1.01f, false, 0.f, 1.f);
		m_demoRenderTargets[RT_QUARTER_1].pRT->Blit(m_hypnoglowBlitter, kPolyFlagAdditive, 0.5f);
#endif

		// Stats.
		const std::string timeInBeatsStr = "timeInBeats = " + ToString(timeInBeats);
		const std::string timeStr = "time = " + ToString(time);
		SystemFont_Draw(m_renderer, timeInBeatsStr, 1, 36);
		SystemFont_Draw(m_renderer, timeStr, 1, 37);
		const std::string fpsStr = "FPS = " + ToString(FPS);
		SystemFont_Draw(m_renderer, fpsStr, 1, 38);

		// pop back buffer from render target stack and assert that it's now empty
		m_renderTargetStack.Pop(false);
		TPB_ASSERT(m_renderTargetStack.GetSize() == 0);
	}
	m_renderer.EndFrame();

	return true;
}
