#ifndef ASSERT_HLSL
#define ASSERT_HLSL

#if defined(__INTELLISENSE__)

void GPUAssert(bool condition) {}
void GPUAssert(bool condition, uint4 data) {}
void GPUAssert(bool condition, float4 data) {}

#define GPUASSERT GPUAssert

#elif defined(DK_GPU_ASSERT_ENABLED)

static bool gGpuAssertAttempted = false;
RWByteAddressBuffer GpuAssertBuffer : register(u0, space0);

void GPUAssertRecordImpl(uint fileId, uint sourceLine, bool condition, uint4 data, uint dataType)
{
    if (condition)
        return;

    if (gGpuAssertAttempted)
        return;

    // 이 invocation은 atomic 시도를 한 번만 수행합니다.
    gGpuAssertAttempted = true;

    uint previous;
    GpuAssertBuffer.InterlockedCompareExchange(0, 0u, 1u, previous);
    if (previous == 0u)
    {
        GpuAssertBuffer.Store3(4, uint3(fileId, sourceLine, dataType));
        GpuAssertBuffer.Store4(16, data);
    }
}
void GPUAssertImpl(uint fileId, uint sourceLine, bool condition, uint4 data = uint4(0, 0, 0, 0))
{
    GPUAssertRecordImpl(fileId, sourceLine, condition, data, 0u);
}
void GPUAssertImpl(uint fileId, uint sourceLine, bool condition, float4 data)
{
    GPUAssertRecordImpl(fileId, sourceLine, condition, asuint(data), 1u);
}

// 조건만 전달한 경우
#define GPUAssertCondition(condition) \
    do \
    { \
        [branch] \
        if (!(condition)) \
        { \
            GPUAssertImpl( \
                GpuAssertFileId(__FILE__), \
                __LINE__, \
                false); \
        } \
    } while (0)

// 조건 + 진단값을 전달한 경우
// data 계산은 실패 분기 안에 위치합니다.
#define GPUAssertWithData(condition, data) \
    do \
    { \
        [branch] \
        if (!(condition)) \
        { \
            GPUAssertImpl( \
                GpuAssertFileId(__FILE__), \
                __LINE__, \
                false, \
                (data)); \
        } \
    } while (0)

// 인자 개수에 따라 위 두 매크로 중 하나를 선택합니다.
#define GPUAssertSelect(_1, _2, NAME, ...) NAME

#define GPUAssert(...) \
    GPUAssertSelect( \
        __VA_ARGS__, \
        GPUAssertWithData, \
        GPUAssertCondition, \
        unused)(__VA_ARGS__)

#define GPUASSERT(...) GPUAssert(__VA_ARGS__)

#else   // defined(DK_SHADERTOOLS), defined(DK_GPU_ASSERT_ENABLED)

#define GPUAssert(...)
#define GPUASSERT(...)

#endif  // defined(DK_SHADERTOOLS), defined(DK_GPU_ASSERT_ENABLED)

#endif
