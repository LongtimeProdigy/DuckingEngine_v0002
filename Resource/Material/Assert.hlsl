#ifndef ASSERT_HLSL
#define ASSERT_HLSL

// SM 6.10 이상 + 해당 기능을 지원하는 컴파일러/런타임/드라이버
// void GPUAssert(bool condition)
// {
//     if (!condition)
//         DebugBreak();
// }

RWByteAddressBuffer DebugHangBuffer : register(u0, space0);
void GPUAssert(bool condition)
{
    if (!condition)
    {
        uint previous;

        [allow_uav_condition]
        while (DebugHangBuffer.Load(0) == 0u)
        {
            DebugHangBuffer.InterlockedAdd(4, 1u, previous);
        }
    }
}

#define GPUASSERT(condition)

#endif
