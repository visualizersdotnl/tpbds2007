
// untitled Bypass demo

#include "main.h"
#include "DemoImpl.h"
#include "../../utility/systemfont.h"
#include "../../utility/draw2D.h"

// These ugly bitches facilitate the parts.
static Renderer *s_pRenderer;
static sync_device *s_hRocket;
static RenderTargetStack *s_pRenderTargetStack;
static const DemoRenderTargetDesc *s_demoRenderTargets; // Points to an array (index with RT_FULL_1 et cetera).
static ColorMatrixBlitter *s_pColorMatrixBlitter;
static DesaturateBlitter *s_pDesaturateBlitter;
static KawaseBlurBlitter *s_pKawaseBlurBlitter;
static HypnoglowBlitter *s_pHypnoglowBlitter;

// Parts.
#include "pixeleffect.h" // Helper.
#include "rainbowroad.cpp"

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
const std::wstring Demo::s_demoName(L"untitled Bypass demo");
const std::string Demo::s_mp3Path("content/testhack/mosaik-04-melo.mp3");
const unsigned int Demo::s_xRes = 1280;
const unsigned int Demo::s_yRes = 720;
const std::string Demo::s_dispModeHint("Presented in 1280*720 (16:9).");
const bool Demo::s_volRampTrick = false;
const float Demo::s_soundtrackBPM = 120.f; // mosaik-04-melo.mp3
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
const std::string kArchivePath = "doom1.wad";

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
,	m_colorMatrixBlitter(renderer, m_resHub)
,	m_desaturateBlitter(renderer, m_resHub)
,	m_kawaseBlurBlitter(renderer, m_resHub)
,	m_hypnoglowBlitter(renderer, 1, m_resHub)
,	m_hRocket(NULL)
{
	SystemFont_RequestTexture(m_resHub);

	// erase DemoRenderTarget array before fiddling with it
	memset(m_demoRenderTargets, 0, sizeof(DemoRenderTargetDesc) * kNumDemoRenderTargets);

	// Request resources for each part.
	RainbowRoad::RequestResources(m_resHub);
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

	// Destroy parts.
	RainbowRoad::Destroy();
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

	// Initialize blitters.
	if (!m_kawaseBlurBlitter.AllocateSurfaces()) return false;
	if (!m_hypnoglowBlitter.AllocateSurfaces()) return false;

	// Set static pointers (declared on top).
	s_pRenderer = &m_renderer;
	s_hRocket = m_hRocket;
	s_pRenderTargetStack = &m_renderTargetStack;
	s_demoRenderTargets = m_demoRenderTargets;
	s_pColorMatrixBlitter = &m_colorMatrixBlitter;
	s_pDesaturateBlitter = &m_desaturateBlitter;
	s_pKawaseBlurBlitter = &m_kawaseBlurBlitter;
	s_pHypnoglowBlitter = &m_hypnoglowBlitter;

	// Create parts.
	if (!RainbowRoad::Create()) return false;

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
#endif // !IGNORE_ROCKET

	// - render --

	m_renderer.BeginFrame(0);
	{
		// push back buffer to the top of the render target stack
		// no need to clear or anything, BeginFrame() just did so!
		m_renderTargetStack.Push(*m_demoRenderTargets[RT_BACK_BUFFER].pRT, false, false);

		// upload timer values to a pixel shader constant
		m_renderer.SetPixelShaderConstantV(PS_VTIMERS_CI, Vector4(time, time * 0.5f, timeInBeats, timeInBeats * 0.5f));

		// Render part.
		RainbowRoad::Render(time);

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
