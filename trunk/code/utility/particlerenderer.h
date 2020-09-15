
// tpbds -- Simple CPU-based particle renderer.

// This renderer is unfit for a massive amount of particles as it is purely CPU-based,
// and generates 4 full vertices for each particle, forming a quad.

#ifndef _PARTICLE_RENDERER_H_
#define _PARTICLE_RENDERER_H_

class ParticleRenderer : public NoCopy
{
public:
	ParticleRenderer(Renderer &renderer, unsigned int numParticles);
	~ParticleRenderer();

	bool AllocateVertexBuffer();

	void Lock();
	void AddParticle(const Vector3 &position, float size, float intensity);
	void Unlock();

	// numParticles - Number of particles to draw. Pass -1 to draw all particles.
	void Draw(bool useStockShaders, const Matrix4x4 &mTransform = Matrix4x4::Identity(), int numParticles = -1) const;

	void Draw(bool useStockShaders, const View &view, int numParticles = -1) const 
	{ 
		Draw(useStockShaders, view.m_mProj * view.m_mView, numParticles); 
	}

private:
	Renderer &m_renderer;
	const unsigned int m_numParticles;

	FlexVertex m_flexVtx;
	Renderer::ShaderPair *m_pShaders;
	Renderer::VertexBuffer *m_pVB;

	int m_iWrite;
};

#endif // _PARTICLE_RENDERER_H_
