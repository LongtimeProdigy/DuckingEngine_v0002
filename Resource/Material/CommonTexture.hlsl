#ifndef __DEFINE_COMMON_HLSL__
#define __DEFINE_COMMON_HLSL__

SamplerState normalSampler : register(s0);

#define TextureParameter uint
#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D gBindlessTextureArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);

#define DEPTHSTENCILTEXTUREINDEX 0  // 이거 바꾸면 C++코드도 바꿔야함
Texture2D getDepthStencilTexture()
{
    return gBindlessTextureArray[DEPTHSTENCILTEXTUREINDEX];
}
Texture2D getRenderTargetTexture(const uint frameIndex)
{
    return gBindlessTextureArray[frameIndex + 1];   // DepthStencil때문에 +1
}
Texture2D getTexture(const TextureParameter index)
{
    return gBindlessTextureArray[index];
}

#endif