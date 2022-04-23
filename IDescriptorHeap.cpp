#include "stdafx.h"
#include "IDescriptorHeap.h"

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

IDescriptorHeap::~IDescriptorHeap()
{
	_descriptorHeap->Release();
}

D3D12_CPU_DESCRIPTOR_HANDLE IDescriptorHeap::GetCPUDescriptorHandleForHeapStart() const noexcept
{
	return _descriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE IDescriptorHeap::GetGPUDescriptorHandleForHeapStart() const noexcept
{
	return _descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}