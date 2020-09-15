
// tpbds -- std. vertex shader constant indices

#ifndef _STDVS_H_
#define _STDVS_H_

// 4x4 matrices for rigid geometry
#define VS_MWORLD_CI  0  // local->world
#define VS_MVIEW_CI   4  // world->view
#define VS_MPROJ_CI   8  // view->projection
#define VS_MFULL_CI   12 // local->projection
#define VS_MCUSTOM_CI 16 // custom or world->local (used by SS_LIT3D)

// world space position and RGB color of single directional light (SS_LIT3D)
#define VS_STOCK_LIGHT_POS_CI   20 
#define VS_STOCK_LIGHT_COLOR_CI 21 

// offset values to compensate for Direct3D 9 texel alignment artifact
// automatically uploaded by Renderer::SetRenderTarget()
#define VS_TEXEL_ALIGN_D3D9_CI 22

// user constants start here
#define VS_USER_CI 23

#endif // _STDVS_H_
