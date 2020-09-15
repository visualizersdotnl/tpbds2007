
// tpbds -- filter base

#ifndef _FILTER_H_
#define _FILTER_H_

// Filter is a thin wrapper around some of the shaders employed by the blitters.
// They're meant for practical use outside the confines of the blitter design.
class Filter : public NoCopy
{
public:
	virtual ~Filter() {}

	virtual const Renderer::ShaderPair *Get() const = 0;

protected:
	const ResourceHub::Shaders *m_pShaders;
};

#endif // _FILTER_H_
