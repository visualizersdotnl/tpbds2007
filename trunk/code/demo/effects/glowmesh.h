
#ifndef _GLOWMESH_H_
#define _GLOWMESH_H_

#include "../../utility/resources.h"
#include "../../utility/particlerenderer.h"

class GlowMesh
{
public:
	GlowMesh(Renderer &renderer, ResourceHub &resHub);
	~GlowMesh();
	
	bool Initialize();
	void Draw(float time); // Effect screws up if the supplied time skips back or forth.

private:
	struct Particle
	{
		unsigned int iVertex;
		float timeOfBirth;
		float timeToLive;
		float timeToFade;
		float size;
	};

	Renderer &m_renderer;

	ResourceHub::Mesh3DS *m_pMesh3DS;
	ResourceHub::Texture *m_pEnvMap, *m_pFlareTexture;
	ResourceHub::Shaders *m_pShaders;

	ParticleRenderer m_particleRenderer;
	Particle *m_pParticles;
};

#endif // _GLOWMESH_H_
