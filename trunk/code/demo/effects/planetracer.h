
#ifndef _PLANETRACER_H_
#define _PLANETRACER_H_

#include "../../utility/resources.h"

class PlaneTracer
{
public:
	PlaneTracer(Renderer &renderer, ResourceHub &resHub);
	~PlaneTracer() {}

	bool Initialize() { return true; }
	void Draw(float time, const float *pOverrideParams); 

private:
	Renderer &m_renderer;
	
	ResourceHub::Texture *m_pTexture;
	ResourceHub::Shaders *m_pShaders;
};

#endif // _PLANCETRACER_H_
