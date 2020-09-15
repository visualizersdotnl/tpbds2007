
// tpbds -- (de)saturation blitter

#include "main.h"
#include "rendertargets.h" // includes desaturate.h
#include "draw2d.h"

DesaturateBlitter::DesaturateBlitter(Renderer &renderer, ResourceHub &resHub) :
	Blitter(renderer),
	m_pShaders(NULL),
	m_blendFac(1.f)
{
	m_pShaders = resHub.RequestShaders("code/utility/blitter.vs", "code/utility/desaturate.ps", FV_POSITION | FV_UV);
}

void DesaturateBlitter::Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const
{
	AssertBlitParameters(pTexture, blendMode, alpha);
	TPB_ASSERT(m_pShaders != NULL); // Initialized?
	
	m_renderer.SetPixelShaderConstantV(PS_USER_CI, Vector4(m_blendFac, alpha, 0.f, 0.f));
	
	m_renderer.SetPolyFlags(blendMode | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, pTexture, kTexFlagPointSamplingBi | kTexFlagAddressClamp);
	m_renderer.SetShaders(m_pShaders->Get());
	EffectQuad_TopLeftUV(m_renderer, Vector2(-1.f, 1.f), Vector2(2.f, 2.f), 0.f, Vector2(1.f, 1.f));
}
