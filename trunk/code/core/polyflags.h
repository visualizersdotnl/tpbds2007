
// tpbds -- "polyflags" (basic geometry batch properties)

// These flags are used in a lot of places so get to know them by heart.
// They're passed around in 32-bit unsigned integers for now!

#ifndef _POLYFLAGS_H_
#define _POLYFLAGS_H_

// blend mode
#define kPolyFlagOpaque       0 // 1:0
#define kPolyFlagAdditive     1 // SrcAlpha:1
#define kPolyFlagSubtractive  2 // SrcAlpha:1
#define kPolyFlagAlpha        3 // SrcAlpha:InvSrcAlpha
#define kPolyFlagColorMask    4 // 0:SrcColor
#define kPolyFlagInvColorMask 5 // 0:InvSrcColor
#define kPolyFlagAlphaMod     6 // SrcAlpha:0
#define kPolyFlagBlendMask    7

// toggles
#define kPolyFlagNoCull      (1 << 4)  
#define kPolyFlagNoZTest     (1 << 5)
#define kPolyFlagNoZWrite    (1 << 6)
#define kPolyFlagLambert     (1 << 7)
#define kPolyFlagWires       (1 << 8)
#define kPolyFlagFog         (1 << 9)
#define kPolyFlagToSRGB      (1 << 10) // If possible, convert from linear to SRGB (exp. 2.2) space on write.

// channel masking (matches D3DCOLORWRITEENABLE_* in d3d9types.h)
#define kPolyFlagMaskBitOffs 12
#define kPolyFlagMaskR       (1 << (kPolyFlagMaskBitOffs + 0))
#define kPolyFlagMaskG       (1 << (kPolyFlagMaskBitOffs + 1))
#define kPolyFlagMaskB       (1 << (kPolyFlagMaskBitOffs + 2))
#define kPolyFlagMaskA       (1 << (kPolyFlagMaskBitOffs + 3))

// abbrev.
#define kPolyFlagNoZBuffer   (kPolyFlagNoZTest | kPolyFlagNoZWrite)
#define kPolyFlagMaskRGB     (kPolyFlagMaskR | kPolyFlagMaskG | kPolyFlagMaskB)
#define kPolyFlagMaskRGBA    (kPolyFlagMaskRGB | kPolyFlagMaskA)

#endif // _POLYFLAGS_H_
