
// tpbds -- Windows monitor description.

// Disable warnings about non-unwindable objects (in case C++ exceptions are disabled).
#pragma warning(disable:4530)

// Force Unicode.
#ifndef _UNICODE
	#define _UNICODE
#endif

// Required library.
#pragma comment(lib, "wbemuuid.lib")

#include <windows.h>

#define _WIN32_DCOM // Multi-threaded COM.
#include <comdef.h>
#include <wbemidl.h>

#include <string>
#include <sstream>

#include "monitordescription.h"

// Macros.
#define FIX_ME
#define SAFE_RELEASE(pX) if (pX != NULL) pX->Release(); pX = NULL;

// Serialize constant value T to std::wstring.
template<typename T> inline std::wstring ToWideString(const T &X)
{
	std::wstringstream stream;
	stream << X;
	return stream.str();
}

static const std::wstring GetMonitorDescriptonFromWMI(DWORD iMonitor)
{
	// If anything fails down the line I just return an empty string and apply a fallback mechanism.
	// This function should never fail unless you're probing a non-existent piece of harwdare.

	// Initialize COM.
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
	{
		return L"";
	}

	// Set COM security levels.
	// Note: if you are using Windows 2000, you need to specify default authentication credentials 
	// for a user by using a SOLE_AUTHENTICATION_LIST structure as the pAuthList parameter.
	if (FAILED(CoInitializeSecurity(
		NULL,                        
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL, // pAuthList
		EOAC_NONE,
		NULL)))
	{
		CoUninitialize();
		return L"";
	}

	// Obtain initial locator to WMI.
	IWbemLocator *pLocator = NULL;
	if (FAILED(CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID *>(&pLocator))))
	{
		CoUninitialize();
		return L"";
	}

	// Connect to WMI.
	IWbemServices *pServices = NULL;
	if (FAILED(pLocator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, NULL, NULL, NULL, NULL, &pServices)))
	{
		pLocator->Release();
		CoUninitialize();
		return NULL;
	}

	// Set security levels on the proxy.
	if (FAILED(CoSetProxyBlanket(
		pServices,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE)))
	{
		pServices->Release();
		pLocator->Release();
		CoUninitialize();
		return L"";
	}
	
	// Request WMI data.
	IEnumWbemClassObject* pEnumerator = NULL;
	if (FAILED(pServices->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_DesktopMonitor"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator)))
	{
		pServices->Release();
		pLocator->Release();
		CoUninitialize();
		return L"";
	}
	
	// Try to derive a correct description.
	std::wstring description;
	DWORD iLoop = 1; // Monitor index is 1-based.
	IWbemClassObject *pClassObj = NULL;
	while (pEnumerator != NULL)
	{
		ULONG uReturn = 0;
		const HRESULT hRes = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObj, &uReturn);
		if (uReturn == 0)
		{
			// Done (pClassObj remains NULL).
			break;
		}
		
		// Is this the one we're looking for?
		if (iMonitor == iLoop) FIX_ME // Untested shortcut (assumes order is identical to that of EnumDisplayDevices).
		{
			FIX_ME // The value used needs to be tested on a true multi-monitor setup.
			VARIANT varProp;
			pClassObj->Get(L"Description", 0, &varProp, NULL, NULL); // Check MSDN for Win32_DesktopMonitor.
			description = varProp.bstrVal;
			description += L" #" + ToWideString(iMonitor);
			VariantClear(&varProp);
			SAFE_RELEASE(pClassObj);

			// Done.
			break;
		}
		else
			SAFE_RELEASE(pClassObj);
	}

	pServices->Release();
	pLocator->Release();
	CoUninitialize();

	// With a hint of luck this string was just built.
	return description;
}

const std::wstring GetMonitorDescription(HMONITOR hMonitor)
{
	MONITORINFOEX monInfoEx;
	monInfoEx.cbSize = sizeof(MONITORINFOEX);
	if (GetMonitorInfo(hMonitor, &monInfoEx))
	{
		// Get monitor index by matching ID.
		DWORD iDevNum = 0;
		DISPLAY_DEVICE dispDev;
		do
		{
			dispDev.cb = sizeof(DISPLAY_DEVICE);
			EnumDisplayDevices(NULL, iDevNum, &dispDev, 0);
			++iDevNum; // Incrementing here for a 1-based display index.
		}
		while (0 != wcscmp(dispDev.DeviceName, monInfoEx.szDevice));
		
		// Attempt to get description from WMI.
		// If it's empty, carry on.
		const std::wstring descriptionFromWMI = GetMonitorDescriptonFromWMI(iDevNum);
		if (!descriptionFromWMI.empty())
			return descriptionFromWMI;

		// Enumerate again, since doing it by string instead of index yields a different (more usable) DeviceString.
		dispDev.cb = sizeof(DISPLAY_DEVICE);
		EnumDisplayDevices(monInfoEx.szDevice, 0, &dispDev, 0);
		
		// WMI approach failed so we rely on EnumDisplayDevices() for an acceptable result.
		std::wstring description(dispDev.DeviceString);
		return description + L" #" + ToWideString(iDevNum);
	}
	else return L"Unknown monitor";
}
