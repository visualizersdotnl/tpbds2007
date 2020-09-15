
// tbpds -- hypnoglow: legacy zoom blur blitter

#ifndef _HYPNOGLOW_H_
#define _HYPNOGLOW_H_

#include "resources.h"
#include "gain.h"

class HypnoglowBlitter : public Blitter, public NoCopy
{
public:
	HypnoglowBlitter(Renderer &renderer, unsigned int mipLevel, ResourceHub &resHub);
	~HypnoglowBlitter();

	// Blur requires 2 custom render targets.
	bool AllocateSurfaces();

	// numIterations - Number of passes; more passes mean more blur.
	// baseScale - Scale of the second quad that is rendered to achieve blur (typically 1.f).
	// passMul - Multiplier on baseScale for each pass.
	// spreadTaps - Spreading the taps (like the Kawase blur does) greatly enhances the amount of blur.
	// glowBias - By default this uses 50:50 blending. You can push that in any direction with a (usually tiny) value (0.5f += ...).
	// gainIntensity - Pushes the contrast of the source image; 0.f (default) means unaltered, upwards pushes the contrast.
	void SetParameters(unsigned int numIterations, float baseScale, float passMul, bool spreadTaps, float glowBias, float gainIntensity) 
	{ 
		TPB_ASSERT(m_numIterations != 0);
		m_numIterations = numIterations; 

		// I could assert these values but it's pretty much "whatever you want to" ;)
		m_baseScale = baseScale;
		m_passMul = passMul;
		m_spreadTaps = spreadTaps;
		m_glowBias = glowBias;
		m_gainIntensity = gainIntensity;
	}

	virtual void Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const;

	// Mechanism to override default blit from render target.
	Renderer::Texture *GetSourceTarget() { TPB_ASSERT(m_pTargets[0]); return m_pTargets[0]; }
	void Filter(uint32_t blendMode, float alpha, bool restoreTargetValid, const Renderer::Texture *pTargetToRestore) const;

private:
	const unsigned int m_mipLevel;

	GainShaders m_gainShaders;
	const ResourceHub::Shaders *m_pShaders;
	Renderer::Texture *m_pTargets[2];

	unsigned int m_numIterations;
	float m_baseScale;
	float m_passMul;
	bool m_spreadTaps;
	float m_glowBias;
	float m_gainIntensity;
};

#endif // _HYPNOGLOW_H_
