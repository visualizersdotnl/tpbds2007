
// tpbds -- texture flags (sampler state)

#ifndef _TEXTUREFLAGS_H_
#define _TEXTUREFLAGS_H_

// sampling mode
#define kTexFlagLinearSamplingTri 0
#define kTexFlagLinearSamplingBi  1
#define kTexFlagAnisoSamplingTri  2
#define kTexFlagPointSamplingBi   3
#define kTexFlagSamplingMask      3

// addressing mode
#define kTexFlagAddressWrap   (0 << 3)
#define kTexFlagAddressClamp  (1 << 3)
#define kTexFlagAddressBorder (2 << 3)
#define kTexFlagAddressMirror (3 << 3)
#define kTexFlagAddressMask   (7 << 3)

// sampler will try to convert SRGB (exp. 2.2) to linear space
#define kTexFlagSRGBToLinear  (1 << 8)

// abbrev.
#define kTexFlagDef         (kTexFlagLinearSamplingTri | kTexFlagAddressWrap)
#define kTexFlagClamp       (kTexFlagLinearSamplingTri | kTexFlagAddressClamp)
#define kTexFlagMirror      (kTexFlagLinearSamplingTri | kTexFlagAddressMirror)
#define kTexFlagTile        (kTexFlagLinearSamplingTri | kTexFlagAddressTile)
#define kTexFlagImageClamp  (kTexFlagLinearSamplingBi  | kTexFlagAddressClamp)
#define kTexFlagImageMirror (kTexFlagLinearSamplingBi  | kTexFlagAddressMirror)
#define kTexFlagImageTile   (kTexFlagLinearSamplingBi  | kTexFlagAddressWrap)

#endif // _TEXTUREFLAGS_H_
