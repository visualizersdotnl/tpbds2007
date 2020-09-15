
#ifndef _DEMO_IMPL_H_
#define _DEMO_IMPL_H_

// Rocket
#define IGNORE_ROCKET 1 // Toggle to fully disable.
// #define SYNC_PLAYER // Uncomment for normal playback.
#include "../../3rdparty/rocket-0.9/sync/sync.h"

#include "../utility/resources.h"
#include "../utility/legacy/Blobs.h"
#include "../utility/rendertargets.h"
#include "../utility/gain.h"

#include "effects/planetracer.h"
#include "effects/tunneltracer.h"
#include "effects/glowmesh.h"

// Demo's RenderTarget instances.
enum DemoRenderTargets
{
	RT_BACK_BUFFER = 0, // Hack: using the first slot for the back buffer.
	RT_FULL_1,
	RT_FULL_2,
	RT_FULL_3,
	RT_FULL_4,
	RT_QUARTER_1,
	RT_QUARTER_2,
	RT_SIXTEENTH_1,
	RT_SIXTEENTH_2,
	kNumDemoRenderTargets
};

// RenderTarget descriptor.
struct DemoRenderTargetDesc
{
	unsigned int mipLevel;
	RenderTarget *pRT;
};

class DemoImpl : public Demo
{
public:
//	static bool DoCompatibilityChecks(UINT iAdapter, IDirect3D9 &d3dObj, const D3DCAPS9 &caps);
//	static Demo *Create(Renderer &renderer);

	DemoImpl(Renderer &renderer);
	~DemoImpl();

	virtual bool Load();
	virtual bool IsLoaded() const;

	virtual bool RenderFrame(float time, float timeInBeats, float FPS, float systemTime);

private:
	ResourceHub m_resHub;

	// Rocket device & tracks.
	sync_device *m_hRocket;
	const sync_track *m_pKickSync;
	const sync_track *m_pKickSyncLerp;
	
	// render targets
	RenderTargetStack m_renderTargetStack;
	DemoRenderTargetDesc m_demoRenderTargets[kNumDemoRenderTargets];

	// effects
	Blobs m_blobs;
	Renderer::ShaderPair *m_pBlobShaders;
	PlaneTracer m_planeTracer;
	TunnelTracer m_tunnelTracer;
	GlowMesh m_glowMesh;

	// filter(s)
	GainShaders m_gainShaders;

	// blitters
	ColorMatrixBlitter m_colorMatrixBlitter;
	DesaturateBlitter m_desaturateBlitter;
	KawaseBlurBlitter m_kawaseBlurBlitter;
	HypnoglowBlitter m_hypnoglowBlitter;

	// hub resources
	ResourceHub::Texture *m_pBlobTexture;
	ResourceHub::Texture *m_pBackgroundTexture;
};

#endif // _DEMO_IMPL_H_
