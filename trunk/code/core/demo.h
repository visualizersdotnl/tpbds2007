
// tpbds -- Demo base class.

// A demo must implement this class and it's static members.

#ifndef _DEMO_H_
#define _DEMO_H_

class Demo : public NoCopy
{
public:
	// This function allows to check for any out-of-the-ordinary graphics adapter features.
	// Please use D3D9CAPS9 structure as-is and do not query your own.
	// Please provide a fitting error description (SetLastError()) on failure.
	// Must be implemented by heir.
	static bool DoCompatibilityChecks(UINT iAdapter, IDirect3D9 &d3dObj, const D3DCAPS9 &caps);

	// Factory function.
	// Must be implemented by heir.
	static Demo *Create(Renderer &renderer);

	virtual ~Demo() {}

	virtual bool Load() = 0;
	virtual bool IsLoaded() const = 0;

	// All parameters except systemTime can be non-contiguous.
	virtual bool RenderFrame(float time, float timeInBeats, float FPS, float systemTime) = 0;

	// Demo settings.
	// Note: the resolution defines the output aspect ratio!
	static const std::wstring s_demoName;
	static const std::string s_mp3Path;
	static const unsigned int s_xRes;
	static const unsigned int s_yRes;
	static const std::string s_dispModeHint;       // Typically a description of the native resolution.
	static const float s_soundtrackBPM;            // BPM or -1 (disables use of Rocket and Audio_BeatSeek()).
	static const unsigned int s_rocketRowsPerBeat; // Rocket track granularity, typically 16 or 32.

protected:
	Demo(Renderer &renderer, int loadState) :
		m_renderer(renderer),
		m_loadState(loadState) {}

	Renderer &m_renderer;

	// This variable can be used to keep track of the loading progress using custom values.
	int m_loadState;
};

#endif // _DEMO_H_
