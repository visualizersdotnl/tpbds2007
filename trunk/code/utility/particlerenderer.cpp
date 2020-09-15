
// tpbds -- Simple CPU-based particle renderer.

#include "main.h"
#include "particlerenderer.h"

// Stock shaders.
#include "stockshaders/particle.h"

ParticleRenderer::ParticleRenderer(Renderer &renderer, unsigned int numParticles) :
	m_renderer(renderer),
	m_numParticles(numParticles),
	m_flexVtx(renderer, flexBitsParticle),
	m_pShaders(renderer.CreateShaderPair(g_vs30_utility_ss_particle, g_ps30_utility_ss_particle, flexBitsParticle)),
	m_pVB(NULL),
	m_iWrite(-1)
{
}

ParticleRenderer::~ParticleRenderer()
{
	m_renderer.DestroyShaderPair(m_pShaders);
	m_renderer.DestroyVertexBuffer(m_pVB);
}

bool ParticleRenderer::AllocateVertexBuffer()
{
	TPB_ASSERT(m_pVB == NULL);
	m_pVB = m_renderer.CreateVertexBuffer(m_numParticles * 4 * m_flexVtx.GetStride(), true);
	return m_pVB != NULL;
}

void ParticleRenderer::Lock()
{
	TPB_ASSERT(m_pVB != NULL);
	m_flexVtx.SetStream(m_pVB->Lock());
	m_iWrite = 0;
}

void ParticleRenderer::AddParticle(const Vector3 &position, float size, float intensity)
{
	TPB_ASSERT(m_iWrite > -1 && (unsigned int) m_iWrite < m_numParticles);

	// Quad is centered.
	const float halfSize = size * 0.5f;
	const Vector3 A(position.m_X - halfSize, position.m_Y + halfSize, position.m_Z);
	const Vector3 B(position.m_X + halfSize, position.m_Y + halfSize, position.m_Z);
	const Vector3 C(position.m_X - halfSize, position.m_Y - halfSize, position.m_Z);
	const Vector3 D(position.m_X + halfSize, position.m_Y - halfSize, position.m_Z);

	unsigned int iVertex = (unsigned int) m_iWrite << 2;
	m_flexVtx[iVertex].Position() = A;
	m_flexVtx[iVertex].PointSize() = intensity;
	m_flexVtx[iVertex++].UV() = Vector2(0.f, 0.f);
	m_flexVtx[iVertex].Position() = B;
	m_flexVtx[iVertex].PointSize() = intensity;
	m_flexVtx[iVertex++].UV() = Vector2(1.f, 0.f);
	m_flexVtx[iVertex].Position() = C;
	m_flexVtx[iVertex].PointSize() = intensity;
	m_flexVtx[iVertex++].UV() = Vector2(0.f, 1.f);
	m_flexVtx[iVertex].Position() = D;
	m_flexVtx[iVertex].PointSize() = intensity;
 	m_flexVtx[iVertex].UV() = Vector2(1.f, 1.f);
		
	++m_iWrite;
}

void ParticleRenderer::Unlock()
{
	TPB_ASSERT(m_pVB != NULL);
	m_pVB->Unlock();
	m_flexVtx.SetStream(NULL);
	m_iWrite = -1;
}

void ParticleRenderer::Draw(
	bool useStockShaders,
	const Matrix4x4 &mTransform /* =  Matrix4x4::Identity() */, 
	int numParticles /* = -1 */) const
{
	TPB_ASSERT(m_pVB != NULL && m_iWrite == -1);
	TPB_ASSERT(numParticles == -1 || (unsigned int) numParticles <= m_numParticles);

	m_renderer.SetVertexShaderConstantM4x4(VS_MCUSTOM_CI, mTransform);

	if (useStockShaders)
	{
		m_renderer.SetShaders(m_pShaders);
	}

	m_renderer.SetVertexFormat(m_flexVtx);
	m_renderer.SetVertexBuffer(m_pVB, m_flexVtx.GetStride());
	m_renderer.DrawQuadList((numParticles != -1) ? numParticles : m_numParticles);
}
