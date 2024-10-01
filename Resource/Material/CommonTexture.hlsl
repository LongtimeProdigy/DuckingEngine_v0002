#ifndef __DEFINE_COMMON_HLSL__
#define __DEFINE_COMMON_HLSL__

SamplerState normalSampler : register(s0);

#define TextureParameter uint
#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D gBindlessTextureArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);

#define DEPTHSTENCILTEXTUREINDEX 0  // 이거 바꾸면 C++코드도 바꿔야함
Texture2D getDepthStencilTexture(const uint frameIndex, const uint offset)
{
    return gBindlessTextureArray[frameIndex * 2 + offset];
}
Texture2D getRenderTargetTexture(const uint frameIndex, const uint offset)
{
    return gBindlessTextureArray[4 + frameIndex * 2 + offset]; // DepthStencil때문에 +4
}
Texture2D getTexture(const TextureParameter index)
{
    return gBindlessTextureArray[index];
}

#endif