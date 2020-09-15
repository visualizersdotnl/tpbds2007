
// tpbds -- (de)saturation blitter

#ifndef _DESATURATE_H_
#define _DESATURATE_H_

#include "resources.h"

class DesaturateBlitter : public Blitter, public NoCopy
{
public:
	DesaturateBlitter(Renderer &renderer, ResourceHub &resHub);
	~DesaturateBlitter() {}

	void SetDesaturation(float blendFac)
	{
		TPB_ASSERT(m_blendFac >= 0.f && m_blendFac <= 1.f); // Saves a clamp() in the shader.
		m_blendFac = blendFac;
	}

	virtual void Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const;

private:
	const ResourceHub::Shaders *m_pShaders;

	float m_blendFac;
};

#endif // _DESATURATE_H_
