
// Oldschool free-directional planes.

#include "main.h"
#include "planetracer.h"
#include "../../utility/draw2d.h"

PlaneTracer::PlaneTracer(Renderer &renderer, ResourceHub &resHub) :
	m_renderer(renderer)
{
	m_pShaders = resHub.RequestShaders("code/demo/effects/tracequad.vs", "code/demo/effects/fdplanes.ps", FV_POSITION | FV_UV);
	m_pTexture = resHub.RequestTexture("content/from_1995/grid2b.jpg", true, false);
}

void PlaneTracer::Draw(float time, const float *pOverrideParams)
{
	// default parameters
	const float planeParams[12] = 
	{
		0.1f,        // uTile
		0.05f,       // vTile
		6.f,         // planeOffs
		0.05f,       // shadeDepthMul
		time * 1.3f, // vScroll
		1.2f,        // wobbleInfluence
		7.8f,        // wobbleFreq
		time * 1.1f, // wobblePhase
		0.f,         // mipBias
		0.f,	     // unused
		0.f,         // unused
		0.f          // unused
	};

	float yRot = time * 0.3f;
	Matrix4x4 mRot = Matrix4x4::RotationZ(time * 0.2f); 
	mRot *= Matrix4x4::RotationY(yRot);

	m_renderer.SetPixelShaderConstantF(PS_USER_CI, 3, (pOverrideParams != NULL) ? pOverrideParams : planeParams);
	m_renderer.SetPixelShaderConstantM4x4(PS_USER_CI + 3, mRot);

	m_renderer.SetPolyFlags(kPolyFlagAdditive | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, m_pTexture->Get(), kTexFlagDef);
	m_renderer.SetShaders(m_pShaders->Get());
	EffectQuad_CenteredUV(m_renderer, Vector2(-1.f, 1.f), Vector2(2.f, 2.f), 0.f, Vector2(m_renderer.GetAspectRatio(), 1.f));
}
