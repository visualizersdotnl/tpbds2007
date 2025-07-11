#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 9.29.952.3111
//
//   fxc /E core_ss_lit3D /T vs_3_0 /Fh
//    h:\Documents and Settings\Administrator\Desktop\tpbds_Refactor\trunk\code\core\stockshaders\lit3D.vsh
//    h:\Documents and Settings\Administrator\Desktop\tpbds_Refactor\trunk\code\core\stockshaders\lit3D.vs
//
//
// Parameters:
//
//   float4x4 mCustom;
//   float4x4 mFull;
//   float3 vStockLightColor;
//   float3 vStockLightPos;
//
//
// Registers:
//
//   Name             Reg   Size
//   ---------------- ----- ----
//   mFull            c12      4
//   mCustom          c16      3
//   vStockLightPos   c20      1
//   vStockLightColor c21      1
//

    vs_3_0
    def c0, 1, 0, 0, 0
    dcl_position v0
    dcl_texcoord v1
    dcl_normal v2
    dcl_position o0
    dcl_color o1
    dcl_texcoord o2.xy
    dcl_texcoord1 o3.x
    mad r0, v0.xyzx, c0.xxxy, c0.yyyx
    dp4 o0.x, r0, c12
    dp4 o0.y, r0, c13
    dp4 o0.z, r0, c14
    dp4 r0.x, r0, c15
    mov r1.xyz, c20
    dp3 r2.x, r1, c16
    dp3 r2.y, r1, c17
    dp3 r2.z, r1, c18
    add r0.yzw, r2.xxyz, -v0.xxyz
    nrm r1.xyz, r0.yzww
    dp3 r0.y, v2, r1
    max r0.y, r0.y, c0.y
    mul o1.xyz, r0.y, c21
    mov o0.w, r0.x
    mov o3.x, r0.x
    mov o1.w, c0.x
    mov o2.xy, v1

// approximately 20 instruction slots used
#endif

const BYTE g_vs30_core_ss_lit3D[] =
{
      0,   3, 254, 255, 254, 255, 
     62,   0,  67,  84,  65,  66, 
     28,   0,   0,   0, 194,   0, 
      0,   0,   0,   3, 254, 255, 
      4,   0,   0,   0,  28,   0, 
      0,   0,   0,   1,   0,   0, 
    187,   0,   0,   0, 108,   0, 
      0,   0,   2,   0,  16,   0, 
      3,   0,  66,   0, 116,   0, 
      0,   0,   0,   0,   0,   0, 
    132,   0,   0,   0,   2,   0, 
     12,   0,   4,   0,  50,   0, 
    116,   0,   0,   0,   0,   0, 
      0,   0, 138,   0,   0,   0, 
      2,   0,  21,   0,   1,   0, 
     86,   0, 156,   0,   0,   0, 
      0,   0,   0,   0, 172,   0, 
      0,   0,   2,   0,  20,   0, 
      1,   0,  82,   0, 156,   0, 
      0,   0,   0,   0,   0,   0, 
    109,  67, 117, 115, 116, 111, 
    109,   0,   3,   0,   3,   0, 
      4,   0,   4,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    109,  70, 117, 108, 108,   0, 
    118,  83, 116, 111,  99, 107, 
     76, 105, 103, 104, 116,  67, 
    111, 108, 111, 114,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      3,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 118,  83, 
    116, 111,  99, 107,  76, 105, 
    103, 104, 116,  80, 111, 115, 
      0, 118, 115,  95,  51,  95, 
     48,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  57,  46,  50,  57,  46, 
     57,  53,  50,  46,  51,  49, 
     49,  49,   0, 171,  81,   0, 
      0,   5,   0,   0,  15, 160, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      0,   0,  15, 144,  31,   0, 
      0,   2,   5,   0,   0, 128, 
      1,   0,  15, 144,  31,   0, 
      0,   2,   3,   0,   0, 128, 
      2,   0,  15, 144,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      0,   0,  15, 224,  31,   0, 
      0,   2,  10,   0,   0, 128, 
      1,   0,  15, 224,  31,   0, 
      0,   2,   5,   0,   0, 128, 
      2,   0,   3, 224,  31,   0, 
      0,   2,   5,   0,   1, 128, 
      3,   0,   1, 224,   4,   0, 
      0,   4,   0,   0,  15, 128, 
      0,   0,  36, 144,   0,   0, 
     64, 160,   0,   0,  21, 160, 
      9,   0,   0,   3,   0,   0, 
      1, 224,   0,   0, 228, 128, 
     12,   0, 228, 160,   9,   0, 
      0,   3,   0,   0,   2, 224, 
      0,   0, 228, 128,  13,   0, 
    228, 160,   9,   0,   0,   3, 
      0,   0,   4, 224,   0,   0, 
    228, 128,  14,   0, 228, 160, 
      9,   0,   0,   3,   0,   0, 
      1, 128,   0,   0, 228, 128, 
     15,   0, 228, 160,   1,   0, 
      0,   2,   1,   0,   7, 128, 
     20,   0, 228, 160,   8,   0, 
      0,   3,   2,   0,   1, 128, 
      1,   0, 228, 128,  16,   0, 
    228, 160,   8,   0,   0,   3, 
      2,   0,   2, 128,   1,   0, 
    228, 128,  17,   0, 228, 160, 
      8,   0,   0,   3,   2,   0, 
      4, 128,   1,   0, 228, 128, 
     18,   0, 228, 160,   2,   0, 
      0,   3,   0,   0,  14, 128, 
      2,   0, 144, 128,   0,   0, 
    144, 145,  36,   0,   0,   2, 
      1,   0,   7, 128,   0,   0, 
    249, 128,   8,   0,   0,   3, 
      0,   0,   2, 128,   2,   0, 
    228, 144,   1,   0, 228, 128, 
     11,   0,   0,   3,   0,   0, 
      2, 128,   0,   0,  85, 128, 
      0,   0,  85, 160,   5,   0, 
      0,   3,   1,   0,   7, 224, 
      0,   0,  85, 128,  21,   0, 
    228, 160,   1,   0,   0,   2, 
      0,   0,   8, 224,   0,   0, 
      0, 128,   1,   0,   0,   2, 
      3,   0,   1, 224,   0,   0, 
      0, 128,   1,   0,   0,   2, 
      1,   0,   8, 224,   0,   0, 
      0, 160,   1,   0,   0,   2, 
      2,   0,   3, 224,   1,   0, 
    228, 144, 255, 255,   0,   0
};
