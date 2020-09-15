
// tpbds -- TPB demo system 2.0 (using Windows, DirectX 9 & Bass)
// project initiated late 2007
// acquired by Bypass in 2011 and will henceforth be marketed as "bpsds"

// written by Niels J. de Wit a.k.a superplek / bypass & TPB (ndewit@gmail.com)

// contributions by:
// - Tim van Klooster (tim / tbl)
// - Marco Foco (pan / spinning kids)
// - Alex Bartholomeus (deadline / superstition)
// - Jean-Noel Dhooge (stil / mandarine)

// third party:
// - BASS 2.3 - Copyright (c) 1999-2007 Ian Luck. All rights reserved.
// - Intel SPMD Program Compiler (ispc)
// - GNU Rocket by Erik Faye-Lund & Egbert Teeselink (optional, must be implemented by the Demo class)

// Required to build:
// - Visual Studio 2008.
// - DirectX 9 SDK with paths set up correctly for both target platforms, including executables (for FXC).
// - If using SSE code generation, set precision mode to FAST.
// - Compile with _UNICODE (ASCII functions used explicitly when needed).

// Specific library dependencies:
// - ws2_32.lib (for Rocket)
// - 3rdparty/bass/c/bass.lib 
// - d3dx9(d).lib

// required to run:
// - at least Windows XP or an equivalent Windows release (only tested on XP)
// - DirectX 9
// - x86 with at least SSE3 (legacy blobs use SSE3 instructions)
// - reasonable GPU (shader model 3.0 support required)

// directory structure:
// /3rdparty         - everything third-party
// /code/core        - system core
// /code/demo        - Demo class implementation, effects, flow et cetera (in short: the fun part)
// /code/utlity      - reusable utility code that may only depend on the system core
// /content          - assets
// /release          - release folder with *some* fixed files (buildrel.bat target)
// /VS9              - all Visual Studio 2008 related project files

// solution structure (mainly because the project still lacks proper dir. structure):
// - actual folders are preceded by a slash (e.g. /demo/effects)
// - other folders used to visually split up files are not (e.g. rendering)

// My personal style guide (not an obligation):
// - pointers are compared to NULL explictly, not to 0 or by boolean expression
// - pointers are pPointer and handles are hHandle, with little to no exceptions
// - pointers use C++ casting, values use C-style casting
// - #define: VALUE (not preferred), MACRO, FLAG & TOGGLE
// - for (global) constants use kValue (const T kValue, static is unnecessary, even for tables)
// - stdint.h (C99) is included: use it!
// - 64-bit: use size_t whenever appropriate
// - 64-bit: compile for 64-bit target on a regular basis
// - default warning level must be 3 and a warning is to be treated as error
// - only use the inline keyword when explicitly needed *and* wanted
// - use of UTF-16 strings is obligatory in places it could actually matter (e.g. Windows API calls)
// - non-local variables are often prefixed
// - cast values (C-style) explicitly in calculations
// - little to no unnecessary parentheses (know your operator precedence rules)
// - comment whenever appropriate, review them every now and then (and try *not* to comment when shitfaced)
// - within core, all depend on main.h (precompiled header) -- outside of core, most things do
// - use FIX_ME to flag (potential) issues or incomplete solutions
// - use TPB_IMPLEMENT to flag whatever still needs implementation
// - don't add any files you're not planning to implement on short notice (causes clutter)

// modularity:
// - tier 1: core (it needs a Demo class implementation to compile and run, for now)
// - tier 2: utility can only rely on core (and 3rdparty)
// - tier 3: demo (provides Demo class implementation)

// how shaders are integrated:
// - stock shaders are built on compile and reside in /stockshaders folders, outside of core this mechanism is deprecated
// - utility's ResourceHub supports runtime loading of shaders
// - ResourceHub precompiles referenced shaders when building an archive and stores resulting bytecode

// important things to do for the final release:
// - ensure FINAL_BUILD is 1
// - check DemoImpl for unnecessary resources and make sure an archive is built and loaded from!
// - revise infofile (and remove soundtrack_permission.txt if not using the Secret Desire remix)
// - pack release (using buildrel.bat)
// - make sure the right D3DX DLL is supplied (saves us a lot of whining on Pouet.net)
// - copy it to a remote location and test it

// to do (also, keep in mind: FIX_ME and TPB_IMPLEMENT):
// - fit and use a new random generator
// - fix those darn X3206 (truncation) warnings! (shaders)
// - shader compile warnings are currently ignored by ResourceHub, fix this!
// - fix Rocket's file I/O for playback
// - read Tim's mail on why the tangent basis calculation works carefully and document it
// - implement 2-pass Gaussian blur blitter
// - implement a few easy to use interpolators (smoothstep() & from Sol/Trauma's page)
// - introduce custom texture loader (using devIL)
// - introduce AVI player (use Deadline's one?)
// - FlexVertex creates many Direct3D vertex declarations on the fly, is this a problem?
// - fix issues with legacy blobs (see Blobs.cpp)
// - give a more detailed error description on failing IDirect3DDevice9::CreateTexture()
// - add support for 16-bit half-floats
// - introduce some basics for multi-threading & jobs (performance-oriented parallelism)
// - split up the project physically (into more subfolders)
// - improve Math module (see math.h)
// - check all constructors and make sure that, if possible, only parameters are used for initialization
// - change #define to const unsigned int in stdps.h and stdvs.h?
// - idea: http://www.geekologie.com/2010/05/eye_candy_crazy_3d_mapped_sens.php
// - isolate part of the core functionality that should not be touched by non-core code
// - perhaps it might be wise to make the Renderer and ResourceHub singletons
// - Renderer:
//   + support for custom vertex formats (speed issue I now just hack around: tag hacks with FIX_ME!)
//     perhaps a stripped base-type FlexVertex only exposing what Renderer needs?
//   + finish gamma correction
//   + set vertex texture sampler state
//   + tie transient heap to STL?
//   + replace Renderer::DrawQuadList*() with a PT_QUAD_LIST primitive type
// - Mesh class:
//   + implement LightWave object loader (see mesh.h)
// - RenderTarget class:
//   + depth buffer awareness
//   + add support for client-owned targets (such as the back buffer, so I can remove the NULL-hacks)
// - ResourceHub class:
//   + find a sensible way to toggle between production and debuggable shaders
//   + finish feature to quickly load a single resource from an archive
//   + asynchronous loading (see resourcehub.h)

// bugs / issues:
// - D3DPOOL_MANAGED textures cease to work if out of VRAM; would be nice to "trap" this event
// - fix screen saver & energy conservation blocking
// - Direct3D prim. buffer lock failure must be handled appropriately
// - strict aliasing (C99) adherence
// - QueryPerformanceTimer()'s frequency may vary due to possible CPU power management software
// - GPU performance takes a hit on the GMA/PowerVR-type series (due to requirements for tiling not being met)
// - FlexVertex []-operator returns an unwindable object: it can not be inlined with C++ exceptions enabled

#include "main.h"

// -- global settings --

#include "globalsettings.h"

// -- renderer settings --

// Toggle to allow display adapters not capable of hardware TNL (or as Kusma puts it: "Polish hardwarë").
const bool kAllowSoftVP = false; 

// Shader model req.
// Core's stock shaders are 3.0, so that's the lower limit.
const DWORD kReqShaderVerMajor = 3;
const DWORD kReqShaderVerMinor = 0;

// fixed surface formats (if you change these, patch the string(s) in Direct3DDevice9::Create())
const D3DFORMAT kFrontBufferFormat  = D3DFMT_X8R8G8B8;
const D3DFORMAT kBackBufferFormat   = D3DFMT_A8R8G8B8;
const D3DFORMAT kDepthStencilFormat = D3DFMT_D24S8;

// -- deferred error --

// error handling works by letting failing function(s) set a global error description before they return
// the last description set is shown right before the application terminates

static std::string s_lastErr;

void SetLastError(const std::string &desc) 
{ 
	TPB_ASSERT(!desc.empty());
	s_lastErr = desc; 
}

// -- Direct3D 9 object --

class Direct3D9 : public NoCopy
{
public:
	Direct3D9() :
		m_hDLL(NULL),
		m_pD3D9(NULL) {}

	~Direct3D9()
	{
		if (m_hDLL != NULL)
		{
			SAFE_RELEASE(m_pD3D9);
			FreeLibrary(m_hDLL);
		}
	}

	bool Load()
	{
		m_hDLL = LoadLibraryA("d3d9.dll");
		if (m_hDLL != NULL)
		{
			typedef IDirect3D9 * (WINAPI *DIRECT3DCREATE9)(UINT SDKVersion);
			DIRECT3DCREATE9 d3dCreate = reinterpret_cast<DIRECT3DCREATE9>(GetProcAddress(m_hDLL, "Direct3DCreate9"));
			m_pD3D9 = d3dCreate(D3D_SDK_VERSION);
			return true;
		}
		
		SetLastError("Can't load Direct3D 9 DLL. Is DirectX installed?");
		return false;
	}

	IDirect3D9 &Get() const { TPB_ASSERT(m_pD3D9 != NULL); return *m_pD3D9; }

private:
	HINSTANCE m_hDLL;
	IDirect3D9 *m_pD3D9;
};

// -- configuration  --

struct Config
{
	unsigned int iAudioDevice; // Audio device index (-1 for default).
	UINT D3DAdapter;           // Direct3D display adapter (D3DADAPTER_DEFAULT for default).
	unsigned int xRes, yRes;   // Output resolution.
	float displayAspectRatio;  // Display aspect ratio (usually yRes/xRes).
	unsigned int refreshRate;  // Refresh rate.
	bool windowed;             // Fullscreen or windowed?
	bool vSync;                // Wait for vertical sync. or present immediately?
};

// Must be initialized by UserConfig().
static Config s_config;

#if !SKIP_DIALOG

#include "../../VS9/resource.h"
#include "monitordescription.h"

struct DisplayAdapter
{
	std::wstring name;
	D3DDISPLAYMODE curDisplayMode;
	bool curDisplayModeValid;
 	std::vector<D3DDISPLAYMODE> displayModes;
};

static std::vector<DisplayAdapter> s_displayAdapters;

static void OnDisplayAdapterSelect(HWND hDialog, const DisplayAdapter &dispAd)
{
	// Wipe comboboxes.
	SendDlgItemMessage(hDialog, IDC_DISPLAY_MODES, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(hDialog, IDC_ASPECT_RATIOS, CB_RESETCONTENT, 0, 0);

	// Add display modes to combobox and try to pick a preferred one.
	int iPreferredMode = -1;
	const unsigned int numDisplayModes = dispAd.displayModes.size();
	for (unsigned int iMode = 0; iMode < numDisplayModes; ++iMode)
	{
		// Add description.
		const D3DDISPLAYMODE &dispMode = dispAd.displayModes[iMode];
		const std::string modeDesc = ToString(dispMode.Width) + '*' + ToString(dispMode.Height) + " at " + ToString(dispMode.RefreshRate) + "Hz";
		SendDlgItemMessageA(hDialog, IDC_DISPLAY_MODES, CB_ADDSTRING, 0, (LPARAM) modeDesc.c_str());

		if (dispAd.curDisplayModeValid)
		{
			// Display adapter is active: match current display mode.
			if (!memcmp(&dispMode, &dispAd.curDisplayMode, sizeof(D3DDISPLAYMODE)))
			{
				iPreferredMode = iMode;
			}
		}
		else
		{
			// Alternatively find one that matches the Demo resolution.
			if (dispMode.Width == Demo::s_xRes && dispMode.Height == Demo::s_yRes)
			{
				// Pick the highest refresh rate.
				if (iPreferredMode == -1 || dispMode.RefreshRate > dispAd.displayModes[iPreferredMode].RefreshRate)
				{
					iPreferredMode = iMode;
				}
				
				break;
			}
		}
	}

	// If no display mode is preferred, go with the first one in the list.
	if (iPreferredMode == -1)
		++iPreferredMode;

	// Select preferred display mode.
	SendDlgItemMessage(hDialog, IDC_DISPLAY_MODES, CB_SETCURSEL, iPreferredMode, 0);

	// Add default aspect ratio (adaptive) to combobox and select it.
	SendDlgItemMessageA(hDialog, IDC_ASPECT_RATIOS, CB_ADDSTRING, 0, (LPARAM) "Adapt from resolution.");
	SendDlgItemMessage(hDialog, IDC_ASPECT_RATIOS, CB_SETCURSEL, 0, 0);

	// Add standardized display aspect ratios.
	const unsigned int numAspectRatios = kNumDisplayAspectRatios;
	for (unsigned int iRatio = 0; iRatio < numAspectRatios; ++iRatio)
	{
		const AspectRatio &aspectRatio = kDisplayAspectRatios[iRatio];
		const std::string &description = aspectRatio.description;
		SendDlgItemMessageA(hDialog, IDC_ASPECT_RATIOS, CB_ADDSTRING, 0, (LPARAM) description.c_str());
	}
}

static BOOL CALLBACK DialogProc(HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			// Add default audio device to combobox and select it.
			SendDlgItemMessageA(hDialog, IDC_AUDIO_DEVICES, CB_ADDSTRING, 0, (LPARAM) "Default audio device.");
			SendDlgItemMessage(hDialog, IDC_AUDIO_DEVICES, CB_SETCURSEL, 0, 0);
			
			// Enumerate *actual* audio devices and add them to combobox.
			const unsigned int audioDevCount = Audio_GetDeviceCount();
			for (unsigned int iAudioDev = 0; iAudioDev < audioDevCount; ++iAudioDev)
			{
				SendDlgItemMessageA(hDialog, IDC_AUDIO_DEVICES, CB_ADDSTRING, 0, (LPARAM) Audio_GetDeviceDescription(iAudioDev));
			}

			// Add display adapter(s) to combobox.
			const unsigned int numDisplayAdapters = s_displayAdapters.size();
			for (unsigned int iAdapter = 0; iAdapter < numDisplayAdapters; ++iAdapter)
			{
				SendDlgItemMessage(hDialog, IDC_DISPLAY_ADAPTERS, CB_ADDSTRING, 0, (LPARAM) s_displayAdapters[iAdapter].name.c_str());
			}

			// Select default display adapter and add it's choices to comboboxes.
			SendDlgItemMessage(hDialog, IDC_DISPLAY_ADAPTERS, CB_SETCURSEL, 0, 0);
			s_config.D3DAdapter = D3DADAPTER_DEFAULT;
			OnDisplayAdapterSelect(hDialog, s_displayAdapters[D3DADAPTER_DEFAULT]);

			// Set windowed-checkbox to default and enable or disable comboboxes.
			s_config.windowed = kPrefWindowed;
			if (s_config.windowed)
			{
				SendDlgItemMessage(hDialog, IDC_WINDOWED, BM_SETCHECK, BST_CHECKED, 0);    
				EnableWindow(GetDlgItem(hDialog, IDC_DISPLAY_MODES), FALSE);
				EnableWindow(GetDlgItem(hDialog, IDC_ASPECT_RATIOS), FALSE);
			}
			else
			{
				SendDlgItemMessage(hDialog, IDC_WINDOWED, BM_SETCHECK, BST_UNCHECKED, 0);    
				EnableWindow(GetDlgItem(hDialog, IDC_DISPLAY_MODES), TRUE);
				EnableWindow(GetDlgItem(hDialog, IDC_ASPECT_RATIOS), TRUE);
			}
			
			// Vertical sync. enabled by default.
			SendDlgItemMessage(hDialog, IDC_VSYNC, BM_SETCHECK, BST_CHECKED, 0);
			
			// Set display mode hint.
			SendDlgItemMessageA(hDialog, IDC_DISP_MODE_HINT, WM_SETTEXT, 0, (LPARAM) Demo::s_dispModeHint.c_str());
		}

		return TRUE;

	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDOK:
				{
					// Grab selected audio device.
					const unsigned int iAudioDev = (unsigned int) SendDlgItemMessage(hDialog, IDC_AUDIO_DEVICES, CB_GETCURSEL, 0, 0);
					if (iAudioDev == 0)
					{
						// First choice (default) is system default.
						s_config.iAudioDevice = -1;
					}
					else
					{
						// Any other is an enumerated pick.
						s_config.iAudioDevice = iAudioDev - 1;
					}

					// Windowed display?
					if (s_config.windowed)
					{
						// Yes: use Demo resolution.
						s_config.xRes = Demo::s_xRes;
						s_config.yRes = Demo::s_yRes;
						s_config.displayAspectRatio = Fraction(Demo::s_xRes, Demo::s_yRes).GetRatio();
						s_config.refreshRate = 0;
					}
					else
					{
						// Grab selected display mode.
						const unsigned int iDisplayMode = (unsigned int) SendDlgItemMessage(hDialog, IDC_DISPLAY_MODES, CB_GETCURSEL, 0, 0);
						const D3DDISPLAYMODE &dispMode = s_displayAdapters[s_config.D3DAdapter].displayModes[iDisplayMode];
						s_config.xRes = dispMode.Width;
						s_config.yRes = dispMode.Height;
						s_config.refreshRate = dispMode.RefreshRate;

						// Grab selected aspect ratio.
						unsigned int iRatio = (unsigned int) SendDlgItemMessage(hDialog, IDC_ASPECT_RATIOS, CB_GETCURSEL, 0, 0);
						if (iRatio == 0)
						{
							// Default: adapt from resolution.
							s_config.displayAspectRatio = Fraction(dispMode.Width, dispMode.Height).GetRatio();
						}
						else
						{
							// Selected from list.
							--iRatio;
							s_config.displayAspectRatio = kDisplayAspectRatios[iRatio].ratio;
						}
					}

					s_config.vSync = SendDlgItemMessage(hDialog, IDC_VSYNC, BM_GETCHECK, 0, 0) != 0;
				}

				EndDialog(hDialog, LOWORD(wParam));
				SetWindowLong(hDialog, DWL_MSGRESULT, 0);
				return TRUE;

			case IDCANCEL:
				EndDialog(hDialog, LOWORD(wParam));
				SetWindowLong(hDialog, DWL_MSGRESULT, 0);
				return TRUE;
				
			case IDC_DISPLAY_ADAPTERS:
				switch (HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					{
						const UINT Adapter = (UINT) SendDlgItemMessage(hDialog, IDC_DISPLAY_ADAPTERS, CB_GETCURSEL, 0, 0);
						TPB_ASSERT(Adapter < s_displayAdapters.size());
						if (s_config.D3DAdapter != Adapter)
						{
							// Selection changed: update!
							s_config.D3DAdapter = Adapter;
							OnDisplayAdapterSelect(hDialog, s_displayAdapters[Adapter]);
						}
					}
					
					SetWindowLong(hDialog, DWL_MSGRESULT, 0);
					return TRUE;
				}
				
				break;
			
			case IDC_WINDOWED:
				switch (HIWORD(wParam))
				{
				case BN_CLICKED:
					// Store state and toggle relevant comboboxes.
					s_config.windowed = IsDlgButtonChecked(hDialog, IDC_WINDOWED) == BST_CHECKED;				
					EnableWindow(GetDlgItem(hDialog, IDC_DISPLAY_MODES), !s_config.windowed);
					EnableWindow(GetDlgItem(hDialog, IDC_ASPECT_RATIOS), !s_config.windowed);

					SetWindowLong(hDialog, DWL_MSGRESULT, 0);
					return TRUE;
				}
				
				break;
			}
		}
		
		break;		
	}
	
	return FALSE;
}

// Predicate for sorting D3DDISPLAYMODE.
inline bool D3DDISPLAYMODE_SortPredicate(const D3DDISPLAYMODE &modeA, const D3DDISPLAYMODE &modeB)
{
	// Descending: refresh rate first, resolution second.
	if (modeA.Width == modeB.Width && modeA.Height == modeB.Height)
	{
		return modeA.RefreshRate > modeB.RefreshRate;
	}
	else
	{
		const UINT numPixelsA = modeA.Width * modeA.Height;
		const UINT numPixelsB = modeB.Width * modeB.Height;
		return numPixelsA > numPixelsB;
	}
}

static bool UserConfig(const Direct3D9 &d3d9, HINSTANCE hInstance)
{
	// Get display adapter(s) and allocate.
	const UINT numDisplayAdapters = d3d9.Get().GetAdapterCount();
	s_displayAdapters.resize(numDisplayAdapters);
	
	// Gather information.
	for (UINT iAdapter = D3DADAPTER_DEFAULT; iAdapter < numDisplayAdapters; ++iAdapter)
	{
		DisplayAdapter &dispAd = s_displayAdapters[iAdapter];

		// Get name.
		D3DADAPTER_IDENTIFIER9 d3dID;
		d3d9.Get().GetAdapterIdentifier(iAdapter, 0, &d3dID);
		dispAd.name = StringToWideString(d3dID.Description);

		// Add monitor ID to display adapter name.
		const HMONITOR hMonitor = d3d9.Get().GetAdapterMonitor(iAdapter);
		dispAd.name += L" on " + GetMonitorDescription(hMonitor);		

		// Get current display mode, if possible.
		dispAd.curDisplayModeValid = D3D_OK == d3d9.Get().GetAdapterDisplayMode(iAdapter, &dispAd.curDisplayMode);

		// Get available display modes.
		const UINT numDisplayModes = d3d9.Get().GetAdapterModeCount(iAdapter, D3DFMT_X8R8G8B8);
		for (UINT iMode = 0; iMode < numDisplayModes; ++iMode)
		{
			D3DDISPLAYMODE dispMode;
			d3d9.Get().EnumAdapterModes(iAdapter, kFrontBufferFormat, iMode, &dispMode);
			dispAd.displayModes.push_back(dispMode);
		}

		// Sort display modes (predicate declared above).
		std::sort(dispAd.displayModes.begin(), dispAd.displayModes.end(), D3DDISPLAYMODE_SortPredicate);
	}

	// Present dialog.
	switch (DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), 0, DialogProc))
	{
	case IDOK:
		return true;
	
	default:
		SetLastError("Can't present configuration dialog.");
	case IDCANCEL:
		return false;
	}

	return true;
}

#else

// -- development configuration(s) --

static bool UserConfig(const Direct3D9 &d3d9, HINSTANCE hInstance)
{
	s_config.iAudioDevice = -1; // System default.
	s_config.D3DAdapter = D3DADAPTER_DEFAULT;
	s_config.xRes = Demo::s_xRes / 2;
	s_config.yRes = Demo::s_yRes / 2;
	s_config.displayAspectRatio = Fraction(s_config.xRes, s_config.yRes).GetRatio();
	s_config.refreshRate = 0;
	s_config.windowed = true;
	s_config.vSync = true;

	return true;
}

#endif // SKIP_DIALOG

// -- render window --

// notes:
// - window is not WS_VISIBLE after Create()
// - window is positioned at SM_XVIRTUALSCREEN, SM_YVIRTUALSCREEN -- properly position it yourself
// - there is another trick to do pass the instance pointer (via GWL_USERDATA, fetched on WM_CREATE)

class RenderWindow
{
public:
	RenderWindow(HINSTANCE hInstance, const std::wstring &title, bool forWindowed) :
		m_hInstance(hInstance),
		m_title(title),
		m_forWindowed(forWindowed),
		m_classRegged(false),
		m_hWnd(NULL) {}
	
	~RenderWindow()
	{
		// Force WM_CLOSE (blocking) if it hasn't been done already.
		if (m_hWnd != NULL)
		{
			SendMessage(m_hWnd, WM_CLOSE, 0, 0);
			TPB_ASSERT(m_hWnd == NULL);
		}

		if (m_classRegged) 
		{
			UnregisterClass(m_title.c_str(), m_hInstance); 
		}
	}

	bool Create(int showCmd, unsigned int xRes, unsigned int yRes)
	{
		WNDCLASSEX wndClass;
		wndClass.cbSize = sizeof(WNDCLASSEX);
		wndClass.style = 0;
		wndClass.lpfnWndProc = GlobalWindowProc;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hInstance = m_hInstance;
		wndClass.hIcon = NULL;
		wndClass.hCursor = NULL;
		wndClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
		wndClass.lpszMenuName = NULL;
		wndClass.lpszClassName = m_title.c_str();
		wndClass.hIconSm = NULL;
		
		if (RegisterClassEx(&wndClass))
		{
			m_classRegged = true;

			// determine style
			DWORD windowStyle, exWindowStyle;
			if (m_forWindowed)
			{
				windowStyle =  WS_POPUP | WS_CAPTION | WS_SYSMENU;
				exWindowStyle = 0;
			}
			else
			{
				windowStyle = WS_POPUP;
				exWindowStyle = WS_EX_TOPMOST;
			}

			// calculate size needed to meet req. client area
			RECT wndRect = { 0, 0, xRes, yRes }; 
			AdjustWindowRectEx(&wndRect, windowStyle, FALSE, exWindowStyle);
			int wndWidth = wndRect.right - wndRect.left;
			int wndHeight = wndRect.bottom - wndRect.top;

			m_hWnd = CreateWindowEx(
				exWindowStyle,
				m_title.c_str(),
				m_title.c_str(),
				windowStyle,
				GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), 
				wndWidth, wndHeight,
				NULL,
				NULL,
				m_hInstance,
				NULL);
				
			if (m_hWnd != NULL)
			{
				SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG) this);
				return true;
			}
		}
		
		SetLastError("Unable to create a window.");
		return false;	
	}

	bool Update(bool &renderFrame) const
	{
		if (m_hWnd != NULL)
		{
			if (!m_forWindowed && IsActive())
			{
				SetCursor(0);
			}
		
			MSG msg;
			if (PeekMessage(&msg, m_hWnd, 0, 0, PM_REMOVE))
			{
				// skip frame -- proceed
				renderFrame = false;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				return true;
			}			

			// render a frame
			renderFrame = !m_isMinimized;
			return true;
		}

		// terminate		
		return false;
	}

	bool IsActive() const { return m_isActive; }
	bool IsMinimized() const { return m_isMinimized; }

	HWND GetHandle() const { return m_hWnd; }
	
private:
	static LRESULT CALLBACK GlobalWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		RenderWindow *pInstance = reinterpret_cast<RenderWindow *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		return (pInstance != NULL)
			? pInstance->WindowProc(hWnd, uMsg, wParam, lParam)
			: DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND:
			return 1; // DefWindowProc() would normally handle this because hbrBackground != NULL
		
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_ESCAPE:
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return TRUE;

#if AUDIO_STEP
			case VK_UP:
				Audio_BeatSeek(false, kAudioStepNumBeats);
				break;
			
			case VK_DOWN:
				Audio_BeatSeek(true, kAudioStepNumBeats);
				break;
#endif
			}
			
			break;

		case WM_CLOSE:
			// DestroyWindow() will follow shortly
			m_hWnd = NULL;
			break;

		case WM_SYSCOMMAND:
			FIX_ME // This doesn't always work properly...
			switch (wParam & 0xffff)
			{
			// prevent screensaver and energy conservation
			case SC_SCREENSAVE:
			case SC_MONITORPOWER:
				return 0;
			}
			
			break;
		
		case WM_ACTIVATE:
			// keep track of status and control WS_EX_TOPMOST for non-windowed RenderWindow
			switch (LOWORD(wParam))
			{
			case WA_ACTIVE:
			case WA_CLICKACTIVE:
				if (!m_forWindowed) SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				m_isActive = true;
				break;
			
			case WA_INACTIVE:
				if (!m_forWindowed) SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				m_isActive = false;
				break;
			}

			m_isMinimized = !!HIWORD(wParam);
			break;
		};
	
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	const HINSTANCE m_hInstance;
	const std::wstring m_title;
	const bool m_forWindowed;

	bool m_classRegged;
	HWND m_hWnd;

	bool m_isActive;
	bool m_isMinimized;
};

// -- Direct3D 9 device --

class Direct3DDevice9 : public NoCopy
{
public:
	Direct3DDevice9() :
		m_pD3DDev(NULL) {}
	
	~Direct3DDevice9()
	{
		SAFE_RELEASE(m_pD3DDev);
	}
	
	bool Create(const Config &config, const Direct3D9 &d3d9, HWND hRenderWnd)
	{
		TPB_ASSERT(m_pD3DDev == NULL);

		// -- checks --

		d3d9.Get().GetDeviceCaps(config.D3DAdapter, D3DDEVTYPE_HAL, &m_caps);

		// figure out if the shader version requirement is met
		bool shaderSupport = false;
		if (m_caps.VertexShaderVersion >= D3DVS_VERSION(kReqShaderVerMajor, kReqShaderVerMinor))
		{
			if (m_caps.PixelShaderVersion >= D3DPS_VERSION(kReqShaderVerMajor, kReqShaderVerMinor))
			{
				shaderSupport = true;
			}
		}

		if (!shaderSupport)
		{
			SetLastError("Display adapter does not support shader model " + ToString(kReqShaderVerMajor) + '.' + ToString(kReqShaderVerMinor) + ".");
			return false;
		}

		// shader version requirement is met
		// based on this, it is safe to assume the display adapter can perform all basic tasks
		// testing for specific requirements is done by a DoCompatibilityChecks() on the Renderer and Demo instances
		
		// test if the surface formats match -- if so, non-exotic lower quality surfaces will probably also work
		switch (d3d9.Get().CheckDepthStencilMatch(config.D3DAdapter, D3DDEVTYPE_HAL, kFrontBufferFormat, kBackBufferFormat, kDepthStencilFormat))
		{
		case D3D_OK:
			break;
		
		case D3DERR_INVALIDCALL:
			TPB_ASSERT(0);	
		case D3DERR_NOTAVAILABLE:
			// keep this string in accordance to kFrontBufferFormat/kBackBufferFormat/kDepthStencilFormat
			SetLastError("Can not use a 24-bit depth buffer and 8-bit stencil with 32-bit render targets.");
			return false;
		}		

		const bool hardwareTNL = (m_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;
		const bool pureDevice = (m_caps.DevCaps & D3DDEVCAPS_PUREDEVICE) != 0;

		// -- device creation -- 

		m_presParms.BackBufferWidth = config.xRes;
		m_presParms.BackBufferHeight = config.yRes;
		m_presParms.BackBufferFormat = kBackBufferFormat;
		m_presParms.BackBufferCount = 1;
		m_presParms.MultiSampleType = D3DMULTISAMPLE_NONE;
		m_presParms.MultiSampleQuality = 0;
		m_presParms.SwapEffect = D3DSWAPEFFECT_DISCARD;
		m_presParms.hDeviceWindow = hRenderWnd;
		m_presParms.Windowed = config.windowed;
		m_presParms.EnableAutoDepthStencil = TRUE;
		m_presParms.AutoDepthStencilFormat = kDepthStencilFormat;
		m_presParms.Flags = 0;
		m_presParms.FullScreen_RefreshRateInHz = (config.windowed) ? 0 : config.refreshRate; // 0 is req. if windowed!
		m_presParms.PresentationInterval = (s_config.vSync) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

		DWORD behaviorFlags = D3DCREATE_NOWINDOWCHANGES;
		if (hardwareTNL)
		{
			behaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
			m_softVP = false;

			if (pureDevice)
			{
				// prohibits usage of the Get*() functions
				behaviorFlags |= D3DCREATE_PUREDEVICE;
			}
		}
		else
		{
			if (!kAllowSoftVP)
			{
				SetLastError("The display adapter does not support hardware TNL (required by the application). Or as Kusma would say, 'your GPU is from Poland'.");
				return false;
			}
						
			behaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
			m_softVP = true;
		}

		switch (d3d9.Get().CreateDevice(config.D3DAdapter, D3DDEVTYPE_HAL, hRenderWnd, behaviorFlags, &m_presParms, &m_pD3DDev))
		{
		case D3D_OK:
			break;
		
		case D3DERR_INVALIDCALL:
			TPB_ASSERT(0);
		case D3DERR_DEVICELOST:
		case D3DERR_NOTAVAILABLE:
		case D3DERR_OUTOFVIDEOMEMORY:
			SetLastError("Unable to create a Direct3D device.");
			return false;
		}

		return true;
	}

	bool Reset()
	{
		TPB_ASSERT(m_pD3DDev != NULL);

		if (m_pD3DDev->Reset(&m_presParms) != D3D_OK)
		{
			SetLastError("Unable to reset Direct3D device.");
			return false;
		}
		
		return true;
	}

	const D3DCAPS9 &GetCaps() const { return m_caps; }
	IDirect3DDevice9 &Get() const { TPB_ASSERT(m_pD3DDev != NULL); return *m_pD3DDev; }
	bool GetSoftVP() const { return m_softVP; }

private:
	D3DCAPS9 m_caps;
	D3DPRESENT_PARAMETERS m_presParms;
	IDirect3DDevice9 *m_pD3DDev;
	bool m_softVP;
};

// -- crash guard --

namespace CrashGuard
{
	HWND hWnd = NULL;
	IDirect3DDevice9 *pD3DDev = NULL;

	// Does what is necessary to get the desktop back in sight, let Windows handle the rest.
	static void OnUnhandledException()
	{
		SAFE_RELEASE(pD3DDev);
		DestroyWindow(hWnd);
	}
};	

// -- main --

int WINAPI Main(HINSTANCE hInstance, HINSTANCE, LPSTR cmdLine, int nCmdShow)
{
#if !FINAL_BUILD
	// Change path to project root.
	TPB_VERIFY(SetCurrentDirectoryA("../") != 0);
#endif

	// Check for SSE3.
	int cpuInfo[4];
	__cpuid(cpuInfo, 1);
	if (!(cpuInfo[2] & 1))
	{
		MessageBox(NULL, L"Processor does not support SSE3 instructions.", Demo::s_demoName.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	// Go!
 	Direct3D9 d3d9;
	if (d3d9.Load())
	{
		if (UserConfig(d3d9, hInstance))
		{
			RenderWindow wnd(hInstance, Demo::s_demoName, s_config.windowed);
			if (wnd.Create(nCmdShow, s_config.xRes, s_config.yRes))
			{
				Direct3DDevice9 d3d9Dev;
				if (d3d9Dev.Create(s_config, d3d9, wnd.GetHandle()))
				{
					// This practically enables the crash guard.
					// Whether or not it is used is decided in WinMain(), not here.
					CrashGuard::hWnd = wnd.GetHandle();
					CrashGuard::pD3DDev = &d3d9Dev.Get();
				
					if (Audio_Create(s_config.iAudioDevice, Demo::s_mp3Path, wnd.GetHandle(), Demo::s_soundtrackBPM, Demo::s_rocketRowsPerBeat))
					{
						if (kMuteAudio)
							Audio_Mute();

						if (Renderer::DoCompatibilityTests(s_config.D3DAdapter, d3d9.Get(), d3d9Dev.GetCaps()))
						{
							Renderer renderer(d3d9Dev.Get(), d3d9Dev.GetSoftVP(), Fraction(Demo::s_xRes, Demo::s_yRes).GetRatio());
							if (renderer.Initialize(s_config.xRes, s_config.yRes, s_config.displayAspectRatio))
							{
								if (Demo::DoCompatibilityChecks(s_config.D3DAdapter, d3d9.Get(), d3d9Dev.GetCaps()))
								{
									ScopedPtr<Demo> pDemo(Demo::Create(renderer));

									// now move the window to the top-left corner of the selected display adapter's monitor
									// if this information is not available, it stays where it is
									const HMONITOR hMonitor = d3d9.Get().GetAdapterMonitor(s_config.D3DAdapter);
									MONITORINFO monInfo;
									monInfo.cbSize = sizeof(MONITORINFO);
									if (GetMonitorInfo(hMonitor, &monInfo))
									{
										SetWindowPos(
											wnd.GetHandle(), 
											NULL, // ignored (SWP_NOZORDER)
											monInfo.rcWork.left, monInfo.rcWork.top, 
											0, 0, // ignored (SWP_NOSIZE)
											SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOSIZE | SWP_NOZORDER);
									}
									
									// done -- show the window
									ShowWindow(wnd.GetHandle(), (s_config.windowed) ? nCmdShow : SW_SHOWNORMAL);
									
									bool demoStarted = false, renderFrame;
									Timer timer;
									while (wnd.Update(renderFrame))
									{
										if (renderFrame)
										{
											const HRESULT hRes = d3d9Dev.Get().TestCooperativeLevel();
											if (hRes == D3D_OK)
											{
												// Update audio system.
												const float audioTime = Audio_Update();

												if (!demoStarted && pDemo->IsLoaded())
												{
													Audio_Start(AUDIO_STEP != 0); // Start playback.
													timer.Reset();                // Reset system timer.
													demoStarted = true;
												}
												
												// Load or render?
												if (!pDemo->IsLoaded())
												{
													// Load cycle.
													if (!pDemo->Load())
													{
														break;
													}
												}
												else
												{
													// Prepare parameters.
													// Note: the FPS is a per-frame extrapolation, not a walking average or
													// a sliding window. The last option would best the smoothest!
													const float systemTime = timer.Get();
													static float prevSystemTime = 0.f;
													const float frameTime = systemTime - prevSystemTime;
													const float FPS = floorf(1.f / frameTime);
													prevSystemTime = systemTime;
													const float timeInBeats = (Demo::s_soundtrackBPM == -1.f)
														? 0.f
														: audioTime / (60.f / Demo::s_soundtrackBPM);
													
													// Render.
													if (!pDemo->RenderFrame(audioTime, timeInBeats, FPS, systemTime))
													{
														break;
													}
												}
											}
											else
											{
												if (hRes == D3DERR_DRIVERINTERNALERROR)
												{
													SetLastError("Direct3D device signaled a D3DERR_DRIVERINTERNALERROR.");
													break;
												}
												else if (hRes == D3DERR_DEVICENOTRESET)
												{
													renderer.OnDeviceLost();									
													if (!(d3d9Dev.Reset() && renderer.OnDeviceReset()))
													{
//														SetLastError("Can not recover Direct3D device!");
														// ^ I commented this out because OnDeviceReset() creates the source from scratch,
														// so in case of failure probably a more useful message has already been set.
														break;
													}
												}
												else
												{
													// Stops our process from eating CPU.
													Sleep(10);
												}
											}
										}
									}
								}
							}
						}
					}
					
					Audio_Destroy();
				}
			}
		}
	}

	// anything bad to report?
	if (!s_lastErr.empty())
	{
		MessageBox(NULL, StringToWideString(s_lastErr).c_str(), Demo::s_demoName.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int nCmdShow)
{
#if CRASH_GUARD
	__try 
	{
#endif

	return Main(hInstance, hPrevInstance, cmdLine, nCmdShow);

#if CRASH_GUARD	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ShowWindow(s_hWnd, SW_HIDE);
		CrashGuard::OnUnhandledException();
		MessageBox(NULL, L"Demo crashed (unhandled exception). Now quickly: http://www.pouet.net!", Demo::s_demoName.c_str(), MB_OK | MB_ICONEXCLAMATION);
		_exit(1); // Better do as little as possible past this point.
	}
#endif

	return 0;
}
