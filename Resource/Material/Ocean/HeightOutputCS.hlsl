Texture2D<float2> FFTResult : register(t0);
RWTexture2D<float> Height : register(u0);

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float2 h = FFTResult[id.xy];
    Height[id.xy] = h.x;
}
