
// tpbds -- linear (matrix) color transform blitter

#include "main.h"
#include "rendertargets.h" // includes colormatrix.h
#include "draw2d.h"

ColorMatrixBlitter::ColorMatrixBlitter(Renderer &renderer, ResourceHub &resHub) :
	Blitter(renderer),
	m_pShaders(NULL),
	m_mColorTrans(Matrix4x4::Identity())
{
	m_pShaders = resHub.RequestShaders("code/utility/blitter.vs", "code/utility/colormatrix.ps", FV_POSITION | FV_UV);
}

void ColorMatrixBlitter::SetHueShiftMatrix(float shiftAngRad)
{
	// Sinusoidal transform.
	const float kPhase = 8.f * atanf(1.f) / 3.f;
	const float kA = 2.f / 3.f;
	const float kB = 1.f - kA;
	const float A = kA * cosf(shiftAngRad) + kB;
	const float B = kA * cosf(shiftAngRad - kPhase) + kB;
	const float C = kA * cosf(shiftAngRad - 2 * kPhase) + kB;

	FIX_ME // RGB weights (NTSC) not applied!

	const float matrix[16] = 
	{
		  A,   B,   C, 0.f,
		  C,   A,   B, 0.f,
		  B,   C,   A, 0.f,
		0.f, 0.f, 0.f, 1.f
	};

	m_mColorTrans = Matrix4x4::FromArray(matrix);
}

void ColorMatrixBlitter::SetSaturateMatrix(float amount)
{
	TPB_ASSERT(amount >= -1.f); // Thought: grey is grey (-1.f), but saturate (> 0.f) all you like.

	// Interpolation values.
	const float Q = -amount;
	const float Q1 = 1 - Q;

	// Weights (NTSC).
	const float weightR = 0.3f;
	const float weightG = 0.59f;
	const float weightB = 0.11f;

	const float matrix[16] = {
		Q1 + weightR * Q,      weightR * Q,      weightR * Q, 0.f,
		     weightG * Q, Q1 + weightG * Q,      weightG * Q, 0.f,
		     weightB * Q,      weightB * Q, Q1 + weightB * Q, 0.f,
		             0.f,              0.f,              0.f, 1.f
	};

	m_mColorTrans = Matrix4x4::FromArray(matrix);
}

void ColorMatrixBlitter::Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const
{
	AssertBlitParameters(pTexture, blendMode, alpha);
	TPB_ASSERT(m_pShaders != NULL); // Initialized?

	m_renderer.SetPixelShaderConstantM4x4(PS_USER_CI, m_mColorTrans);
	m_renderer.SetPixelShaderConstantV(PS_USER_CI + 4, Vector4(alpha, 0.f, 0.f, 0.f));

	m_renderer.SetPolyFlags(blendMode | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, pTexture, kTexFlagPointSamplingBi | kTexFlagAddressClamp);
	m_renderer.SetShaders(m_pShaders->Get());
	EffectQuad_TopLeftUV(m_renderer, Vector2(-1.f, 1.f), Vector2(2.f, 2.f), 0.f, Vector2(1.f, 1.f));
}
