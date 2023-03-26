#ifndef __DEFINE_COMMON_HLSL__
#define __DEFINE_COMMON_HLSL__

#define TextureParameter uint
#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D gBindlessTextureArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);

SamplerState normalSampler : register(s0);

#endif