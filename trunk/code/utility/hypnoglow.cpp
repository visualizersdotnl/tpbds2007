
// tbpds -- hypnoglow: legacy zoom blur (based on Kawase blur blitter)

#include "main.h"
#include "rendertargets.h" // includes hypnoglow.h
#include "draw2d.h"

HypnoglowBlitter::HypnoglowBlitter(Renderer &renderer, unsigned int mipLevel, ResourceHub &resHub) :
	Blitter(renderer),
	m_mipLevel(mipLevel),
	m_gainShaders(resHub),
	m_pShaders(NULL),
	m_numIterations(4),
	m_baseScale(1.f),
	m_passMul(1.05f),
	m_spreadTaps(false),
	m_glowBias(0.f),
	m_gainIntensity(0.f)
{
	m_pShaders = resHub.RequestShaders("code/utility/blitter.vs", "code/utility/kawaseblur.ps", FV_POSITION | FV_UV);
	m_pTargets[0] = NULL;
	m_pTargets[1] = NULL;
}

HypnoglowBlitter::~HypnoglowBlitter()
{
	m_renderer.DestroyTexture(m_pTargets[0]);
	m_renderer.DestroyTexture(m_pTargets[1]);
}
	
bool HypnoglowBlitter::AllocateSurfaces()
{
	for (unsigned int iTarget = 0; iTarget < 2; ++iTarget)
	{
		TPB_ASSERT(m_pTargets[iTarget] == NULL);
		m_pTargets[iTarget] = m_renderer.CreateReferenceRenderTarget(m_mipLevel, D3DFMT_X8R8G8B8);
		if (m_pTargets[iTarget] == NULL)
		{
			return false;
		}
	}	
	
	return true;
}

void HypnoglowBlitter::Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const
{
	TPB_ASSERT(m_pTargets[0] != NULL);

	const Renderer::Texture *pCurrentRenderTarget = m_renderer.GetRenderTarget(0);

	// First reduce the original image.
	m_renderer.SetRenderTarget(0, m_pTargets[0], 0, 0);
	m_renderer.SetPolyFlags(kPolyFlagOpaque | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, pTexture, kTexFlagImageClamp);
	m_gainShaders.UploadParameters(m_renderer, m_gainIntensity);
	SpriteQuad(m_renderer, m_gainShaders.Get(), Vector2(0.f), Vector2(1.f), 0.f, Vector2(1.f), AlphaAndRGBToD3DCOLOR(1.f, 0xffffff));

	// And run the filter.
	Filter(blendMode, alpha, true, pCurrentRenderTarget);
}

void HypnoglowBlitter::Filter(uint32_t blendMode, float alpha, bool restoreTargetValid, const Renderer::Texture *pTargetToRestore) const
{
	AssertFilterParameters(blendMode, alpha);
	TPB_ASSERT(m_pTargets[0] != NULL && m_pTargets[1] != NULL);
	TPB_ASSERT(m_pShaders != NULL);

	const Renderer::Texture *pCurrentRenderTarget = (!restoreTargetValid)
		? m_renderer.GetRenderTarget(0)
		: pTargetToRestore;

	unsigned int bufferFlip = 0;
	float scale = m_baseScale;

	// Derive UV delta for filter.
	Vector2 uvDelta = m_pTargets[0]->GetResolution();
	uvDelta.m_X = 1.f / uvDelta.m_X;
	uvDelta.m_Y = 1.f / uvDelta.m_Y;
	uvDelta *= 0.5f;

	// Do passes.
	for (unsigned int iPass = 0; iPass < m_numIterations; ++iPass)
	{
		float fPass;
		if (!m_spreadTaps) 
		{
			// Taps stay put.
			fPass = 1.f;
		}
		else 
		{
			// Taps spread out (causing extreme blur).
			fPass = (float) iPass * 2.f + 1.f;
		}
		
		m_renderer.SetPixelShaderConstantV(PS_USER_CI, Vector4(uvDelta.m_X * fPass, uvDelta.m_Y * fPass, 0.5f + m_glowBias, 0.f));

		m_renderer.SetRenderTarget(0, m_pTargets[bufferFlip ^ 1], D3DCLEAR_TARGET, 0);
		m_renderer.SetTexture(0, m_pTargets[bufferFlip], kTexFlagImageClamp);
		m_renderer.SetShaders(m_pShaders->Get());

		m_renderer.SetPolyFlags(kPolyFlagAlphaMod | kPolyFlagNoZBuffer);
		EffectQuad_TopLeftUV(m_renderer, Vector2(-1.f, 1.f), Vector2(2.f, 2.f), 0.f, Vector2(1.f, 1.f));

		const float newScale = 2.f * scale;
		const Vector2 topLeft(-newScale * 0.5f, newScale * 0.5f);
		const Vector2 size(newScale);

		m_renderer.SetPolyFlags(kPolyFlagAdditive | kPolyFlagNoZBuffer);
		EffectQuad_TopLeftUV(m_renderer, topLeft, size, 0.f, Vector2(1.f, 1.f));
		
		bufferFlip ^= 1;
		scale *= m_passMul;
	}

	// Now blit back the resulting image.
	// This would look better if it uses something more than straight bilinear sampling.
	m_renderer.SetRenderTarget(0, pCurrentRenderTarget, 0, 0);
	m_renderer.SetPolyFlags(blendMode | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, m_pTargets[bufferFlip], kTexFlagImageClamp);
	SpriteQuad(m_renderer, NULL, Vector2(0.f), Vector2(1.f), 0.f, Vector2(1.f), AlphaAndRGBToD3DCOLOR(alpha, 0xffffff));
}
