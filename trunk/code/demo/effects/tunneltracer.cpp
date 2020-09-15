
// Oldschool free-directional tunnel.

#include "main.h"
#include "TunnelTracer.h"
#include "../../utility/draw2d.h"

TunnelTracer::TunnelTracer(Renderer &renderer, ResourceHub &resHub) :
	m_renderer(renderer)
{
	m_pShaders = resHub.RequestShaders("code/demo/effects/tracequad.vs", "code/demo/effects/fdtunnel.ps", FV_POSITION | FV_UV);
	m_pTexture = resHub.RequestTexture("content/from_1995/frame2.jpg", true, false);
}

void TunnelTracer::Draw(float time, const float *pOverrideParams, const Matrix4x4 &mParam /* = Matrix4x4::Identity() */)
{
	// default parameters
	const float tunnelParams[12] =
	{
		1.f,         // radius
		time * 2.3f, // vScroll
		1.f,         // uTile
		0.2f,        // vTile
		0.f,         // flowerScale
		0.f,         // flowerFreq
		0.f,         // flowerPhase
		0.1f,        // shadeDepthMul
		-0.2f,       // mipBias
		0.f,         // toBox
		0.f, 0.f     // unused
	};

	const Matrix4x4 mRot = mParam;

	m_renderer.SetPixelShaderConstantF(PS_USER_CI, 3, (pOverrideParams != NULL) ? pOverrideParams : tunnelParams);
	m_renderer.SetPixelShaderConstantM4x4(PS_USER_CI + 3, mRot);

	m_renderer.SetPolyFlags(kPolyFlagAdditive | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, m_pTexture->Get(), kTexFlagDef);
	m_renderer.SetShaders(m_pShaders->Get());
	EffectQuad_CenteredUV(m_renderer, Vector2(-1.f, 1.f), Vector2(2.f, 2.f), 0.f, Vector2(m_renderer.GetAspectRatio(), 1.f));
}
