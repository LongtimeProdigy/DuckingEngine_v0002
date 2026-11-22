#ifndef __DEFINE_COMMON_HLSL__
#define __DEFINE_COMMON_HLSL__

SamplerState pointSampler : register(s0);
SamplerState bilinearRepeatSampler : register(s1);

#define TextureParameter uint
#include "ShaderBindings.h"
#define BINDLESSTEXTUREARRAY_SPACE DK_REGISTER_SPACE(DK_BINDLESS_TEXTURE_SPACE)
Texture2D<float4> gBindlessTextureSRVArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);
RWTexture2D<float4> gBindlessTextureUAVArray[] : register(u0, BINDLESSTEXTUREARRAY_SPACE);
Texture3D<float4> gBindlessTexture3DSRVArray[] : register(t0, DK_REGISTER_SPACE(DK_BINDLESS_TEXTURE3D_SPACE));
RWTexture3D<float4> gBindlessTexture3DUAVArray[] : register(u0, DK_REGISTER_SPACE(DK_BINDLESS_TEXTURE3D_SPACE));

Texture2D<float4> getTexture(const TextureParameter index)
{
    return gBindlessTextureSRVArray[index];
}

RWTexture2D<float4> getTextureRW(const TextureParameter index)
{
    return gBindlessTextureUAVArray[index];
}

Texture3D<float4> getTexture3D(const TextureParameter index)
{
    return gBindlessTexture3DSRVArray[index];
}

RWTexture3D<float4> getTexture3DRW(const TextureParameter index)
{
    return gBindlessTexture3DUAVArray[index];
}

#endif
