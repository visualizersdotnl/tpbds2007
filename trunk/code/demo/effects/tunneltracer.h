
#ifndef _TUNNELTRACER_H_
#define _TUNNELTRACER_H_

#include "../../utility/resources.h"

class TunnelTracer
{
public:
	TunnelTracer(Renderer &renderer, ResourceHub &resHub);
	~TunnelTracer() {}

	bool Initialize() { return true; }
	void Draw(float time, const float *pOverrideParams, const Matrix4x4 &mParam = Matrix4x4::Identity());

private:
	Renderer &m_renderer;
	
	ResourceHub::Texture *m_pTexture;
	ResourceHub::Shaders *m_pShaders;
};

#endif // _PLANCETRACER_H_
