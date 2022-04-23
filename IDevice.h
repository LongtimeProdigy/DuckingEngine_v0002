#pragma once

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

struct ID3D12Device8;
struct D3D12_GRAPHICS_PIPELINE_STATE_DESC;
struct D3D12_DESCRIPTOR_HEAP_DESC;
struct D3D12_SHADER_RESOURCE_VIEW_DESC;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
enum D3D12_DESCRIPTOR_HEAP_TYPE;

struct ID3D12RootSignature;
struct ID3D12PipelineState;
//struct ID3D12DescriptorHeap;

class IResource;
class IDescriptorHeap;
class ICommandList;

class IDevice
{
public:
	IDevice(ID3D12Device8* device)
		: _device(device)
	{}
	~IDevice();

	IResource* CreateUploadResource(ICommandList* commandList, const void* data, const uint width, const uint height);
	IResource* CreateDefaultResource(ICommandList* commandList, const void* data, const uint width, const uint height);
	IResource* CreateTextureResource(ICommandList* commandList, DXGI_FORMAT format, const void* data, const uint width, const uint height, const uint bytePerPixel);
	const uint GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept;
	bool CreateRootSignature(const uint nodeMask, const void* blobWithRootSignature, const uint blobLengthInBytes, _OUT_ ID3D12RootSignature*& outSignature) const;
	bool CreateGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, _OUT_ ID3D12PipelineState*& outPipelineStateObject) const;
	bool CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC& desc, IDescriptorHeap& outDescriptorHeap) const;
	void CreateShaderResourceView(const IResource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc, const D3D12_CPU_DESCRIPTOR_HANDLE handle) const noexcept;
	void CreateTexutre() const;

private:
	ID3D12Device8* _device;
};