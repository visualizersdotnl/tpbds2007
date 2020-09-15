
// tpbds -- main header

// This is a VC++ precompiled header (see pch.cpp) and serves as the main hub for the project's core.
// I'm not happy with this choice but if you read this I still haven't found a reason to fix it!

#ifndef _MAIN_H_
#define _MAIN_H_

// Disable warnings about non-unwindable objects in case C++ exceptions are disabled.
#pragma warning(disable:4530)

// Keep Windows header from defining "min" and "max", which interfere with std::min() and std::max().
#define NOMINMAX

// Force Unicode.
#ifndef _UNICODE
	#define _UNICODE
#endif

// Enable for Direct3D debugging.
// #define D3D_DEBUG_INFO 1

// -- external headers --

// OS/API headers
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

// for intrinsics
#include <intrin.h>
#include <xmmintrin.h> // SSE

// STL & CRT headers (add as needed, but keep it to a minimum)
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <string>
#include <sstream>
#include <math.h>

// C99: stdint.h/inttypes.h (msinttypes-r26)
#define __STDC_LIMIT_MACROS
#include "../../3rdparty/msinttypes-r26/stdint.h"

// -- types, macros, defines & pragmas --

// custom types (don't forget: use C99's uint32_t et al. for such types)
typedef uint8_t Byte;
typedef std::vector<std::string> StringArray;

// misc. macro(s)
#define SAFE_RELEASE(pX) if (pX != NULL) pX->Release(); pX = NULL;

// assert/verify macros
#ifdef _DEBUG
	#define TPB_ASSERT(condition) { if (!(condition)) __debugbreak(); }
	#define TPB_VERIFY(condition) TPB_ASSERT(condition)
#else
	#define TPB_ASSERT(condition)
	#define TPB_VERIFY(condition) (condition)
#endif

// Use to flag pieces of code that need (further) implementation.
#define TPB_IMPLEMENT TPB_ASSERT(0)

// Use this to flag pieces of code that need to be either fixed or removed.
#define FIX_ME // TPB_ASSERT(0);

// -- utility functions --

// inherit from NoCopy to prohibit assignment and copy-construction (to prevent ownership issues)
class NoCopy
{
protected:
	NoCopy() {}	
	virtual ~NoCopy() {}

private:
	NoCopy(const NoCopy &noCopy) {}
	
	NoCopy & operator =(const NoCopy &RHS) { return *this; }
};

// serialize constant value T to std::string
template<typename T> inline std::string ToString(const T &X)
{
	std::stringstream stream;
	TPB_VERIFY((stream << X));
	return stream.str();
}

// serialize constant value T to std::wstring
template<typename T> inline std::wstring ToWideString(const T &X)
{
	std::wstringstream stream;
	TPB_VERIFY((stream << X));
	return stream.str();
}

// ASCII to wide string conversion.
// Do not expand on this; if more conversion is needed, write a set of helper classes.
inline const std::wstring StringToWideString(const std::string &string)
{
	std::wstring wideString(string.length(), L'');
	std::copy(string.begin(), string.end(), wideString.begin());
	return wideString;
}

// Use this function to print debug messages; please use them sparingly.
inline void DebugPrint(const std::string &message)
{
#ifdef _DEBUG
	OutputDebugStringA((message + "\n").c_str());
#endif
}

// -- core headers --

#include "alignedalloc.h"
#include "scopedptr.h"
#include "scopedarr.h"
#include "opaquedata.h"
#include "crc32.h"
#include "math.h"

#include "timer.h"
#include "fileio.h"
#include "audio.h"
#include "renderer.h" // Automatically includes many other relevant headers.
#include "mesh.h"
#include "view.h"

#include "demo.h"

// -- main.cpp --

// in case of failure, set a fitting description
// it will be displayed right before the application terminates
void SetLastError(const std::string &desc); 

#endif // _MAIN_H_
