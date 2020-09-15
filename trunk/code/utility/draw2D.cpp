	
// tpbds -- convenience 2D draw

#include "main.h"
#include "draw2D.h"

// Rotates 2D position around given pivot.
inline const Vector3 RotPos(const Vector3 &curPos, const Vector3 &quadPivot, float rotAngRad)
{
	if (rotAngRad == 0.f)
	{
		// This is usually the case.
		return curPos;
	}
	else
	{
		// A bit meaty in terms of cycle count for what it does, I'll tell you what.
		const Matrix4x4 mRot = Matrix4x4::RotationZ(rotAngRad);
		return mRot * (curPos - quadPivot) + quadPivot;
	}
}

void EffectQuad_CenteredUV(Renderer &renderer, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, const Vector2 &UVscale)
{
	const Vector2 adjSize(size.m_X, -size.m_Y);

	Vector3 quadPivot;
	quadPivot.m_X = topLeft.m_X + size.m_X * 0.5f;
	quadPivot.m_Y = topLeft.m_Y + size.m_Y * 0.5f;
	quadPivot.m_Z = 0.f;

	ScratchVertices quadVerts(renderer, FV_POSITION | FV_UV, 4);
	quadVerts[0].Position() = RotPos(Vector3(topLeft), quadPivot, rotAngRad);
	quadVerts[0].UV() = Vector2(-0.5f, -0.5f).Scale(UVscale);
	quadVerts[1].Position() = RotPos(Vector3(topLeft.m_X + adjSize.m_X, topLeft.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[1].UV() = Vector2(0.5f, -0.5f).Scale(UVscale);
	quadVerts[2].Position() = RotPos(Vector3(topLeft.m_X, topLeft.m_Y + adjSize.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[2].UV() = Vector2(-0.5f, 0.5f).Scale(UVscale);
	quadVerts[3].Position() = RotPos(Vector3(topLeft + adjSize), quadPivot, rotAngRad); 
	quadVerts[3].UV() = Vector2(0.5f, 0.5f).Scale(UVscale);

	quadVerts.SetForDraw(renderer);
	renderer.DrawQuadList(1);
}

void EffectQuad_TopLeftUV(Renderer &renderer, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, const Vector2 &UVscale)
{
	const Vector2 adjSize(size.m_X, -size.m_Y);

	Vector3 quadPivot;
	quadPivot.m_X = topLeft.m_X + size.m_X * 0.5f;
	quadPivot.m_Y = topLeft.m_Y + size.m_Y * 0.5f;
	quadPivot.m_Z = 0.f;

	ScratchVertices quadVerts(renderer, FV_POSITION | FV_UV, 4);
	quadVerts[0].Position() = RotPos(Vector3(topLeft), quadPivot, rotAngRad);
	quadVerts[0].UV() = Vector2(0.f, 0.f).Scale(UVscale);
	quadVerts[1].Position() = RotPos(Vector3(topLeft.m_X + adjSize.m_X, topLeft.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[1].UV() = Vector2(1.f, 0.f).Scale(UVscale);
	quadVerts[2].Position() = RotPos(Vector3(topLeft.m_X, topLeft.m_Y + adjSize.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[2].UV() = Vector2(0.f, 1.f).Scale(UVscale);
	quadVerts[3].Position() = RotPos(Vector3(topLeft + adjSize), quadPivot, rotAngRad);
	quadVerts[3].UV() = Vector2(1.f, 1.f).Scale(UVscale);

	quadVerts.SetForDraw(renderer);
	renderer.DrawQuadList(1);
}

void SpriteQuad(Renderer &renderer, const Renderer::ShaderPair *pShaders, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, const Vector2 &UVscale, D3DCOLOR vtxColor)
{
	Vector2 adjTopLeft = topLeft;
	adjTopLeft.m_X = topLeft.m_X * 2.f - 1.f;
	adjTopLeft.m_Y = -(topLeft.m_Y * 2.f - 1.f);

	Vector2 adjSize = size;
	adjSize.m_X = size.m_X * 2.f;
	adjSize.m_Y = -size.m_Y * 2.f;

	Vector3 quadPivot;
	quadPivot.m_X = adjTopLeft.m_X + adjSize.m_X * 0.5f;
	quadPivot.m_Y = adjTopLeft.m_Y + adjSize.m_Y * 0.5f;
	quadPivot.m_Z = 0.f;

	ScratchVertices quadVerts(renderer, flexBitsSprite2D, 4);
	quadVerts[0].Position() = RotPos(Vector3(adjTopLeft), quadPivot, rotAngRad);
	quadVerts[0].UV() = Vector2(0.f, 0.f).Scale(UVscale);
	quadVerts[1].Position() = RotPos(Vector3(adjTopLeft.m_X + adjSize.m_X, adjTopLeft.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[1].UV() = Vector2(1.f, 0.f).Scale(UVscale);
	quadVerts[2].Position() = RotPos(Vector3(adjTopLeft.m_X, adjTopLeft.m_Y + adjSize.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[2].UV() = Vector2(0.f, 1.f).Scale(UVscale);
	quadVerts[3].Position() = RotPos(Vector3(adjTopLeft + adjSize), quadPivot, rotAngRad);
	quadVerts[3].UV() = Vector2(1.f, 1.f).Scale(UVscale);

	for (unsigned int iVert = 0; iVert < 4; ++iVert)
	{
		quadVerts[iVert].Color() = vtxColor;
	}	
	
	if (pShaders)
	{
		renderer.SetShaders(pShaders);
	}
	else
	{
		renderer.SetStockShaders(Renderer::SS_SPRITE2D);
	}
	
	quadVerts.SetForDraw(renderer);
	renderer.DrawQuadList(1);
}

void FontQuad(Renderer &renderer, const Vector2 &topLeft, const Vector2 &size, float rotAngRad, float leftU, float topV, float charDeltaU, float charDeltaV)
{
	Vector2 adjTopLeft = topLeft;
	adjTopLeft.m_X = topLeft.m_X * 2.f - 1.f;
	adjTopLeft.m_Y = -(topLeft.m_Y * 2.f - 1.f);

	Vector2 adjSize = size;
	adjSize.m_X = size.m_X * 2.f;
	adjSize.m_Y = -size.m_Y * 2.f;

	Vector3 quadPivot;
	quadPivot.m_X = adjTopLeft.m_X + adjSize.m_X * 0.5f;
	quadPivot.m_Y = adjTopLeft.m_Y + adjSize.m_Y * 0.5f;
	quadPivot.m_Z = 0.f;
	
	ScratchVertices quadVerts(renderer, flexBitsSprite2D, 4);
	quadVerts[0].Position() = RotPos(Vector3(adjTopLeft), quadPivot, rotAngRad);
	quadVerts[0].UV() = Vector2(leftU, topV);
	quadVerts[1].Position() = RotPos(Vector3(adjTopLeft.m_X + adjSize.m_X, adjTopLeft.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[1].UV() = Vector2(leftU + charDeltaU, topV);
	quadVerts[2].Position() = RotPos(Vector3(adjTopLeft.m_X, adjTopLeft.m_Y + adjSize.m_Y, 1.f), quadPivot, rotAngRad);
	quadVerts[2].UV() = Vector2(leftU, topV + charDeltaV);
	quadVerts[3].Position() = RotPos(Vector3(adjTopLeft + adjSize), quadPivot, rotAngRad);
	quadVerts[3].UV() = Vector2(leftU + charDeltaU, topV + charDeltaV);
	
	for (unsigned int iVert = 0; iVert < 4; ++iVert)
	{
		quadVerts[iVert].Color() = 0xffffffff;
	}	

	renderer.SetStockShaders(Renderer::SS_SPRITE2D);
	quadVerts.SetForDraw(renderer);
	renderer.DrawQuadList(1);
}

void FadeQuad(Renderer &renderer, float intensity, bool isSubtractive, uint32_t addPolyFlags /* = 0 */)
{
	TPB_ASSERT((addPolyFlags & kPolyFlagBlendMask) == 0);

	const Vector2 topLeft(-1.f, 1.f);
	const Vector2 size(2.f, -2.f);

	ScratchVertices quadVerts(renderer, flexBitsSprite2D, 4);
	quadVerts[0].Position() = Vector3(topLeft);
	quadVerts[1].Position() = Vector3(topLeft.m_X + size.m_X, topLeft.m_Y, 1.f);
	quadVerts[2].Position() = Vector3(topLeft.m_X, topLeft.m_Y + size.m_Y, 1.f);
	quadVerts[3].Position() = Vector3(topLeft + size); 

	const DWORD vtxColor = AlphaAndRGBToD3DCOLOR(intensity, !isSubtractive * 0xffffff);
	for (unsigned int iVert = 0; iVert < 4; ++iVert)
	{
		quadVerts[iVert].UV() = Vector2(0.f);
		quadVerts[iVert].Color() = vtxColor;
	}	

	renderer.SetPolyFlags(kPolyFlagAlpha | kPolyFlagNoZBuffer | addPolyFlags);
	renderer.SetTexture(0, NULL, kTexFlagDef);
	renderer.SetStockShaders(Renderer::SS_SPRITE2D);
	quadVerts.SetForDraw(renderer);
	renderer.DrawQuadList(1);
}
