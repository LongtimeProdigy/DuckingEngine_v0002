#ifndef ASSERT_HLSL
#define ASSERT_HLSL

#if defined(DK_GPU_ASSERT_ENABLED)
RWByteAddressBuffer GpuAssertBuffer : register(u0, space0);

// Record the first failure, then let the submission finish for CPU readback.
void GPUAssertRecordImpl(uint fileId, uint sourceLine, bool condition, uint4 data, uint dataType)
{
    if (!condition)
    {
        uint previous;
        GpuAssertBuffer.InterlockedCompareExchange(0, 0u, 1u, previous);
        if (previous == 0u)
        {
            GpuAssertBuffer.Store3(4, uint3(fileId, sourceLine, dataType));
            GpuAssertBuffer.Store4(16, data);
        }
    }
}

// dataType matches GpuAssertDataType on the CPU. Preserve the payload bits.
void GPUAssertImpl(uint fileId, uint sourceLine, bool condition, uint4 data = uint4(0, 0, 0, 0))
{
    GPUAssertRecordImpl(fileId, sourceLine, condition, data, 0u);
}

void GPUAssertImpl(uint fileId, uint sourceLine, bool condition, float4 data)
{
    GPUAssertRecordImpl(fileId, sourceLine, condition, asuint(data), 1u);
}

#define GPUAssert(...) GPUAssertImpl(GpuAssertFileId(__FILE__), __LINE__, __VA_ARGS__)
#define GPUASSERT(...) GPUAssert(__VA_ARGS__)
#else
#define GPUAssert(...)
#define GPUASSERT(...)
#endif

#endif
