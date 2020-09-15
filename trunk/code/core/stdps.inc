
// tpbds -- std. pixel shader constants

float4 vTimers : register(c0); // application-defined, do not use in generic code
float4 vConstants : register(c1);
float4 vFogParams : register(c2);
float4 vFogRGB1 : register(c3);

// abbrev.
#define vAspectRatio vConstants.x
#define vFogNear vFogParams.x
#define vFogRange vFogParams.y
#define vFogEnabled vFogParams.z

// macros
float sign(float x) { return (x < 0) ? -1 : 1; }
float sqr(float x) { return x * x; }

// constants
static float kPI = 3.1415926535897932384626433832795f;
static float kEpsilon = 5.96e-08f; // max. error for 32-bit floats

// linear fog function
float4 ApplyLinearFog(float viewZ, float4 RGBA) { return lerp(RGBA, vFogRGB1, vFogEnabled * saturate((viewZ - vFogNear) / vFogRange)); }

// first user constant index
#define kFirstUserReg c4
