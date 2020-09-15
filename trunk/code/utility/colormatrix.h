
// tpbds -- linear (matrix) color transform blitter

#ifndef _COLORMATRIX_H_
#define _COLORMATRIX_H_

#include "resources.h"

class ColorMatrixBlitter : public Blitter, public NoCopy
{
public:
	ColorMatrixBlitter(Renderer &renderer, ResourceHub &resHub);
	~ColorMatrixBlitter() {}

	void SetMatrix(const Matrix4x4 &mColorTrans)
	{
		m_mColorTrans = mColorTrans;
	}

	// Built-in tricks written by Pan / Spinning Kids.
	void SetHueShiftMatrix(float shiftAngRad);
	void SetSaturateMatrix(float amount); // -1 = Desaturated, 0 = Normal, > 0 = Saturate.

	virtual void Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const;

private:
	const ResourceHub::Shaders *m_pShaders;

	Matrix4x4 m_mColorTrans;
};

#endif // _COLORMATRIX_H_
