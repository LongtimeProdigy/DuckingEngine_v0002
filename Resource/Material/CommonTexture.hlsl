#ifndef __DEFINE_COMMON_HLSL__
#define __DEFINE_COMMON_HLSL__

SamplerState normalSampler : register(s0);

#define TextureParameter uint
#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D<float4> gBindlessTextureSRVArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);
RWTexture2D<float4> gBindlessTextureUAVArray[] : register(u0, BINDLESSTEXTUREARRAY_SPACE);

Texture2D<float4> getDepthStencilTexture(const uint frameIndex, const uint offset)
{
    return gBindlessTextureSRVArray[frameIndex * 2 + offset];
}
Texture2D<float4> getRenderTargetTexture(const uint frameIndex, const uint offset)
{
    return gBindlessTextureSRVArray[4 + frameIndex * 2 + offset]; // DepthStencil때문에 +4
}
Texture2D<float4> getTexture(const TextureParameter index)
{
    return gBindlessTextureSRVArray[index];
}

RWTexture2D<float4> getTextureRW(const TextureParameter index)
{
    return gBindlessTextureUAVArray[index];
}

#endif