#ifndef __DEFINE_COMMONCOMPLEX_HLSL__
#define __DEFINE_COMMONCOMPLEX_HLSL__

float2 ComplexMul(float2 a, float2 b)
{
    return float2(
        a.x * b.x - a.y * b.y, // 결과 실수부: ac - bd
        a.x * b.y + a.y * b.x  // 결과 허수부: ad + bc
    );
}

#endif