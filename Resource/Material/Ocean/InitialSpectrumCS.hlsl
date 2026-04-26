RWTexture2D<float2> H0 : register(u0);

cbuffer OceanParams : register(b0)
{
    float2 _windDir;
    float _A;
    float _L;
    uint _N;

    TextureParameter _h0;
}

float Phillips(float2 k)
{
    float kLen = length(k);
    if (kLen < 1e-6) return 0.0;

    float k2 = kLen * kLen;
    float k4 = k2 * k2;

    float2 kNorm = k / kLen;
    float kw = dot(kNorm, normalize(_windDir));

    float damping = exp(-1.0 / (k2 * _L * _L));
    return _A * damping * kw * kw / k4;
}

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    int x = id.x;
    int y = id.y;

    float2 k = float2(
        (x - _N/2) * 2.0 * 3.14159 / _L,
        (y - _N/2) * 2.0 * 3.14159 / _L
    );

    float P = Phillips(k);

    float2 gaussian = float2(
        rand(x, y),
        rand(y, x)
    );

    H0[id.xy] = gaussian * sqrt(P * 0.5);
}
