
// tpbds -- tweakable gain Filter 

#ifndef _GAIN_H_
#define _GAIN_H_

#include "filter.h"
#include "resources.h"

class GainShaders : public Filter
{
public:
	GainShaders(ResourceHub &resHub)
	{
		m_pShaders = resHub.RequestShaders("code/utility/blitter.vs", "code/utility/gain.ps", FV_POSITION | FV_UV);
	}
	
	GainShaders() {}

	// Intensity at 0.f gives a normal picture, anything onwards pushes the contrast.
	// Values below zero have the opposite effect.
	void UploadParameters(Renderer &renderer, float intensity) const
	{
		renderer.SetPixelShaderConstantV(PS_USER_CI, Vector4(intensity, 0.f, 0.f, 0.f));
	}

	virtual const Renderer::ShaderPair *Get() const
	{
		TPB_ASSERT(m_pShaders != NULL);
		return m_pShaders->Get();
	}
};

#endif // _GAIN_H_
