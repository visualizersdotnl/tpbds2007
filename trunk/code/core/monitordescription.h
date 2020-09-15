
// tpbds -- Windows monitor description.

// Notes:
// - Link wbemidl.lib (already hinted through a #pragma).
// - This function initializes and uses COM, so if you do too, beware of mixups.

#ifndef _MONITOR_DESCRIPTION_H_
#define _MONITOR_DESCRIPTION_H_

const std::wstring GetMonitorDescription(HMONITOR hMonitor);

#endif // _MONITOR_DESCRIPTION_H_
