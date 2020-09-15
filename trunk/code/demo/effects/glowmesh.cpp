
// My overused early 2000s glow effect.

#include "main.h"
#include "glowmesh.h"

const unsigned int kNumParticles = 32; // 680;

GlowMesh::GlowMesh(Renderer &renderer, ResourceHub &resHub) :
	m_renderer(renderer),
	m_particleRenderer(renderer, kNumParticles),
	m_pParticles(new Particle[kNumParticles])
{
	m_pMesh3DS = resHub.RequestMesh3DS("content/misc/plek02.3ds", 4.5f);
	m_pEnvMap = resHub.RequestTexture("content/misc/mfx_envmap.jpg", true, false);
	m_pFlareTexture = resHub.RequestTexture("content/misc/mfx_flare.jpg", true, false);
	m_pShaders = resHub.RequestShaders("code/demo/effects/glowmesh.vs", "code/demo/effects/glowmesh.ps", ResourceHub::Mesh3DS::kFlexVertexFlags);

	for (unsigned int iParticle = 0; iParticle < kNumParticles; ++iParticle)
	{
		m_pParticles[iParticle].iVertex = iParticle;
		m_pParticles[iParticle].timeOfBirth = 0.f;
		m_pParticles[iParticle].timeToLive = 0.f;
	}
}
	
GlowMesh::~GlowMesh() 
{
	delete[] m_pParticles;
}

bool GlowMesh::Initialize()
{
	if (!m_particleRenderer.AllocateVertexBuffer())
	{
		return false;
	}
	
	return true;
}

void GlowMesh::Draw(float time)
{
	const float vRot = time * 0.1f;
	Matrix4x4 mWorldRot = Matrix4x4::RotationY(vRot) * Matrix4x4::RotationX(vRot) * Matrix4x4::RotationZ(-vRot + cosf(vRot));
	Matrix4x4 mWorld = mWorldRot;
//	mWorld.SetTranslation(Vector3(1.f, 0.f, 0.f));
//	mWorld.SetTranslation(Vector3(0.3f, 0.6f, 0.f));
	m_renderer.SetVertexShaderConstantM4x4(VS_MWORLD_CI, mWorld);
	m_renderer.SetVertexShaderConstantM4x4(VS_MCUSTOM_CI, mWorld.AffineInverse());

	unsigned int numToDraw = 0;

	m_particleRenderer.Lock();
	for (unsigned int iParticle = 0; iParticle < kNumParticles; ++iParticle)
	{
		Particle &particle = m_pParticles[iParticle];

		if (time >= particle.timeOfBirth + particle.timeToLive)
		{
			particle.iVertex = rand() % m_pMesh3DS->Get()->GetNumVertices();
			particle.timeOfBirth = time;
			particle.timeToLive = 4.f + randf(4.f);
			particle.timeToFade = 1.f + randf(1.f);
			particle.size = 0.3f + randf(1.5f + fmodf(time, 1.f)); 
		}

		const float lifeTime = time - particle.timeOfBirth;
		float intensity;
		if (lifeTime < particle.timeToFade) 
		{
			intensity = lifeTime * (1.f / particle.timeToFade);
		}
		else if (lifeTime > particle.timeToLive - particle.timeToFade) 
		{
			intensity = (particle.timeToLive - lifeTime) * (1.f / particle.timeToFade);
		}
		else
		{
			intensity = 1.f;
		}
		
		const Vector3 worldPos = mWorld * m_pMesh3DS->Get()->GetFlexVertex()[particle.iVertex].Position();
		const float normalZ = (mWorldRot * m_pMesh3DS->Get()->GetFlexVertex()[particle.iVertex].Normal()).m_Z;

		// Hack that eliminates front-facing "backsides".
		// Object must remain centered on the Z-axis!
		const float clampedZ = -std::min(0.f, worldPos.m_Z); 

		const float posFade = clampedZ * -normalZ; // Negative normals face inwards.
		if (posFade > kEpsilon)
		{
			intensity *= posFade;
			m_particleRenderer.AddParticle(worldPos, particle.size, intensity);
			++numToDraw;
		}
		else
		{
			// cull by killing it off
			particle.timeToLive = 0.f;
		}
	}
	m_particleRenderer.Unlock();

	View view(m_renderer);
	view.m_yFieldOfViewRad = kPI / 3.2f;
	view.Update();
	view.UploadMatrices(true, &mWorld);

	m_renderer.SetVertexShaderConstantV(VS_STOCK_LIGHT_POS_CI, Vector3(0.f, 0.f, -6.f));
	m_renderer.SetVertexShaderConstantV(VS_STOCK_LIGHT_COLOR_CI, Vector3(1.2f, 1.1f, 1.4f));

	m_renderer.SetPolyFlags(kPolyFlagOpaque);
	m_renderer.SetTexture(0, m_pEnvMap->Get(), kTexFlagDef);
//	m_renderer.SetTexture(0, NULL, kTexFlagDef);
	m_renderer.SetShaders(m_pShaders->Get());
	m_pMesh3DS->Get()->Draw();

	m_renderer.SetPolyFlags(kPolyFlagAdditive | kPolyFlagNoZBuffer);
	m_renderer.SetTexture(0, m_pFlareTexture->Get(), kTexFlagDef);	
	m_particleRenderer.Draw(true, view, numToDraw);
}
