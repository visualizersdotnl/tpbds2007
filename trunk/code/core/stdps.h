
// tpbds -- std. pixel shader constant indices

#ifndef _STDPS_H_
#define _STDPS_H_

#define PS_VTIMERS_CI    0 // container for arbitrary timers
#define PS_VCONSTANTS_CI 1 // constants set by Renderer
#define PS_VFOG_SETUP_CI 2 // fog parameters: float4(nearViewSpaceZ, viewSpaceRange, (isEnabled) ? 1 : 0, 0)
#define PS_VFOG_COLOR_CI 3 // fog color (RGB)

// user constants start here
#define PS_USER_CI 4 

#endif // _STDPS_H_
