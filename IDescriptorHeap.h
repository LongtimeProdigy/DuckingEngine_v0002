#pragma once

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12DescriptorHeap;

class IDescriptorHeap
{
public:
	IDescriptorHeap() {}
	IDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
		: _descriptorHeap(descriptorHeap)
	{}
	~IDescriptorHeap();

	dk_inline const ID3D12DescriptorHeap* GetDescriptorHeap() const noexcept
	{
		return _descriptorHeap;
	}
	dk_inline ID3D12DescriptorHeap* GetDescriptorHeapWritable() const noexcept
	{
		return _descriptorHeap;
	}
	dk_inline void SetDescriptorHeap(ID3D12DescriptorHeap* value) noexcept
	{
		_descriptorHeap = value;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const noexcept;

private:
	ID3D12DescriptorHeap* _descriptorHeap;
};