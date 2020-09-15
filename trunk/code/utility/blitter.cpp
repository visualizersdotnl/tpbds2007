
// tpbds -- simple blitter implementations

#include "main.h"
#include "blitter.h"
#include "draw2d.h"

void TileBlitter::Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const 
{
	AssertBlitParameters(pTexture, blendMode, alpha);
	TPB_ASSERT(m_xTile >= 0.f && m_yTile >= 0.f);
	m_renderer.SetPolyFlags(blendMode | kPolyFlagNoZBuffer);
	const bool tiling = m_xTile > 1.f || m_yTile > 1.f;
	m_renderer.SetTexture(0, pTexture, (tiling) ? kTexFlagImageTile : kTexFlagImageClamp);
	SpriteQuad(m_renderer, NULL, Vector2(0.f), Vector2(1.f), 0.f, Vector2(m_xTile, m_yTile), AlphaAndRGBToD3DCOLOR(alpha, 0xffffff));
}

void MosaikBlitter::Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const
{
	AssertBlitParameters(pTexture, blendMode, alpha);
	m_renderer.SetPolyFlags(blendMode | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, pTexture, kTexFlagPointSamplingBi | kTexFlagAddressClamp);
	SpriteQuad(m_renderer, NULL, Vector2(0.f), Vector2(1.f), 0.f, Vector2(1.f), AlphaAndRGBToD3DCOLOR(alpha, m_vtxColorRGB));
}

