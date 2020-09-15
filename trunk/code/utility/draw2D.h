
// tpbds -- convenience 2D draw (and a few layout utility functions below)

#ifndef _DRAW2D_H_
#define _DRAW2D_H_

// Notes on render state:
// - EffectQuad*() -- No state set.
// - SpriteQuad()  -- Stock shaders optional (Renderer::SS_SPRITE2D) if pShaders == NULL.
// - FontQuad()    -- Only sets shaders (Renderer::SS_SPRITE2D).
// - FadeQuad()    -- Sets full state.
//
// EffectQuad_CenteredUV() and EffectQuad_TopLeftUV() accept "native" Direct3D coordinates.
// This means X ranges from -1 (left) to 1 (right), and Y ranges from 1 (top) to -1 (bottom).
//
// SpriteQuad() and FontQuad() use an adjusted, more intuitive coordinate system:
// Both X and Y range from 0 (left, top) to 1 (right, bottom).

// sends 2D quad at Z = 1.f with centered [-0.5, 0.5] scalable UVs
void EffectQuad_CenteredUV(Renderer &renderer, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, const Vector2 &UVscale);

// sends 2D quad at Z = 1.f with [0.0, 1.0] scalable UVs
void EffectQuad_TopLeftUV(Renderer &renderer, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, const Vector2 &UVscale);

// sends 2D quad at Z = 1.f with [0.0, 1.0] scalable UVs
void SpriteQuad(Renderer &renderer, const Renderer::ShaderPair *pShaders, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, const Vector2 &UVscale, D3DCOLOR vtxColor);

// mostly identical to SpriteQuad(), but can be used with font UVs
void FontQuad(Renderer &renderer, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, float leftU, float topV, float charDeltaU, float charDeltaV);

// sends full 2D quad at Z = 1.f to darken or brighten
void FadeQuad(Renderer &renderer, float intensity, bool isSubtractive, uint32_t addPolyFlags = 0);

// -- layout utility functions --

// center an image on the Demo resolution's X-axis in preparation for SpriteQuad()
// Returns m_X = xOffs, m_Y = xSize
inline const Vector2 CenterImageOnX(const Renderer::Texture *pTexture)
{
	const Vector2 texRes = pTexture->GetResolution();
	const float demoResX = (float) Demo::s_xRes;
	const float xOffs = (demoResX - texRes.m_X) * 0.5f;
	return Vector2(1.f / demoResX).Scale(Vector2(xOffs, texRes.m_X));
}

// center an image on the Demo resolution's Y-axis in preparation for SpriteQuad()
// Returns m_X = yOffs, m_Y = ySize
inline const Vector2 CenterImageOnY(const Renderer::Texture *pTexture)
{
	const Vector2 texRes = pTexture->GetResolution();
	const float demoResY = (float) Demo::s_yRes;
	const float yOffs = (demoResY - texRes.m_Y) * 0.5f;
	return Vector2(1.f / demoResY).Scale(Vector2(yOffs, texRes.m_Y));
}

// center an image on the Demo resolution in preparation for SpriteQuad()
inline const Vector4 CenterImage(const Renderer::Texture *pTexture)
{
	const Vector2 texRes = pTexture->GetResolution();
	const Vector2 demoRes((float) Demo::s_xRes, (float) Demo::s_yRes);
	const Vector2 xyOffs = (demoRes - texRes) * 0.5f;
	return Vector4(
		xyOffs.m_X / demoRes.m_X,
		texRes.m_X / demoRes.m_X,
		xyOffs.m_Y / demoRes.m_Y,
		texRes.m_Y / demoRes.m_Y);
}

#endif // _DRAW2D_H_
