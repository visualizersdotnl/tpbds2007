
// tpbds -- simple 4-tap blur blitter (the name Kawase was mentioned at GDC 2003)

#ifndef _KAWASE_H_
#define _KAWASE_H_

#include "resources.h"
#include "gain.h"

class KawaseBlurBlitter : public Blitter, public NoCopy
{
public:
	KawaseBlurBlitter(Renderer &renderer, ResourceHub &resHub);
	~KawaseBlurBlitter();

	// Kawase's blur requires 2 custom render targets.
	bool AllocateSurfaces();

	// numIterations - Number of passes; more passes mean more blur; 4 is default and looks good.
	// gainIntensity - Pushes the contrast of the source image; 0.f (default) means unaltered, upwards pushes the contrast.
	void SetParameters(unsigned int numIterations, float gainIntensity) 
	{ 
		TPB_ASSERT(m_numIterations != 0);
		m_numIterations = numIterations; 
		m_gainIntensity = gainIntensity;
	}

	virtual void Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const;

	// Mechanism to override default blit from render target.
	Renderer::Texture *GetSourceTarget() { TPB_ASSERT(m_pTargets[0]); return m_pTargets[0]; }
	void Filter(uint32_t blendMode, float alpha, bool restoreTargetValid, const Renderer::Texture *pTargetToRestore) const;

private:
	GainShaders m_gainShaders;

	const ResourceHub::Shaders *m_pShaders;
	Renderer::Texture *m_pTargets[2];

	unsigned int m_numIterations;
	float m_gainIntensity;
};

#endif // _KAWASE_H_
