
// Wrapper for 2D pixel shader driven effects.

#ifndef _PIXEL_EFFECT_H_
#define _PIXEL_EFFECT_H_

class PixelEffect
{
public:
	// Static initialziation allowed.
	PixelEffect(const std::string &psPath) :
		m_psPath(psPath),
		m_pShaders(NULL) {}

	~PixelEffect() {}

	void Request(ResourceHub &resHub)
	{
		m_pShaders = resHub.RequestShaders("code/demo/effects/tracequad.vs", m_psPath, FV_POSITION | FV_UV);
	}

	// Tweak or expand as needed.
	void Draw(uint32_t polyFlags) const
	{
		s_pRenderer->SetPolyFlags(polyFlags | kPolyFlagNoZBuffer); // Always no Z-buffer!
		s_pRenderer->SetShaders(m_pShaders->Get());
		EffectQuad_CenteredUV(*s_pRenderer, Vector2(-1.f, 1.f), Vector2(2.f, 2.f), 0.f, Vector2(s_pRenderer->GetAspectRatio() * 2.f, 2.f));
	}

private:	
	const std::string m_psPath;
	const ResourceHub::Shaders *m_pShaders;
};

#endif // _PIXEL_EFFECT_H_
