#include "stdafx.h"
#include "IDevice.h"

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#include "d3dx12.h"

#include "IResource.h"
#include "ICommandList.h"
#include "IDescriptorHeap.h"

IDevice::~IDevice()
{
	_device->Release();
}

IResource* IDevice::CreateUploadResource(ICommandList* commandList, const void* data, const uint width, const uint height)
{
	D3D12_RESOURCE_DESC bufferDesc;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = width;
	bufferDesc.Height = height;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProperty;
	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	ID3D12Resource* uploadBuffer;
	// #todo- createcommittedresource가 실패할 수 있습니다. 예외 처리가 필요함!
	HRESULT hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
	if (FAILED(hr) == true)
	{
		return nullptr;
	}
	uploadBuffer->SetName(L"UploadBuffer");

	//CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(uploadBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	//commandList->ResourceBarrier(1, &barrier);

	IResource* returnResource = dk_new IResource(uploadBuffer);

	return returnResource;
}

IResource* IDevice::CreateDefaultResource(ICommandList* commandList, const void* data, const uint width, const uint height)
{
	D3D12_RESOURCE_DESC bufferDesc;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = width;
	bufferDesc.Height = height;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProperty;
	heapProperty.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	ID3D12Resource* defaultBuffer;
	// #todo- createcommittedresource가 실패할 수 있습니다. 예외 처리가 필요함!
	HRESULT hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer));
	if (FAILED(hr) == true)
	{
		return nullptr;
	}
	defaultBuffer->SetName(L"DefaultBuffer");

	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	ID3D12Resource* uploadBuffer;
	// #todo- createcommittedresource가 실패할 수 있습니다. 예외 처리가 필요함!
	hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
	if (FAILED(hr) == true)
	{
		return nullptr;
	}
	uploadBuffer->SetName(L"UploadBuffer");

	D3D12_SUBRESOURCE_DATA resourceData = {};
	resourceData.pData = reinterpret_cast<const BYTE*>(data);
	resourceData.RowPitch = width * height;
	resourceData.SlicePitch = width * height;

#if 1
	UpdateSubresources(commandList->GetCommandListWritable(), defaultBuffer, uploadBuffer, 0, 0, 1, &resourceData);
#else
	// Copy the triangle data to the vertex buffer.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	m_vertexBuffer->Unmap(0, nullptr);
#endif // USE_D3DX12_HELPER

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &barrier);

	IResource* returnResource = dk_new IResource(defaultBuffer);

	return returnResource;
}

IResource* IDevice::CreateTextureResource(ICommandList* commandList, DXGI_FORMAT format, const void* data, const uint width, const uint height, const uint bytePerPixel)
{
	D3D12_RESOURCE_DESC resourceDescription = {};
	// now describe the texture with the information we have obtained from the image
	resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	resourceDescription.Width = width; // width of the texture
	resourceDescription.Height = height; // height of the texture
	resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	resourceDescription.Format = format; // This is the dxgi format of the image (format of the pixels)
	resourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	resourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

	D3D12_HEAP_PROPERTIES heapProperty;
	heapProperty.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	ID3D12Resource* defaultBuffer;
	// #todo- createcommittedresource가 실패할 수 있습니다. 예외 처리가 필요함!
	HRESULT hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDescription,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer));
	if (FAILED(hr) == true)
	{
		return nullptr;
	}
	defaultBuffer->SetName(L"TextureDefaultBuffer");

	UINT64 textureUploadBufferSize;
	// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
	// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
	// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
	//textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
	_device->GetCopyableFootprints(&resourceDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	ID3D12Resource* uploadBuffer;
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
	hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
	if (FAILED(hr) == true)
	{
		return nullptr;
	}
	uploadBuffer->SetName(L"TextureUploadBuffer");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = data; // pointer to our image data
	textureData.RowPitch = width * bytePerPixel; // size of all our triangle vertex data
	textureData.SlicePitch = height; // also the size of our triangle vertex data

	UpdateSubresources(commandList->GetCommandListWritable(), defaultBuffer, uploadBuffer, 0, 0, 1, &textureData);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &barrier);

	IResource* returnResource = dk_new IResource(defaultBuffer);
	return returnResource;
}

const uint IDevice::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept
{
	return _device->GetDescriptorHandleIncrementSize(type);
}

bool IDevice::CreateRootSignature(const uint nodeMask, const void* blobWithRootSignature, const uint blobLengthInBytes, _OUT_ ID3D12RootSignature*& outSignature) const
{
	HRESULT hr = _device->CreateRootSignature(nodeMask, blobWithRootSignature, blobLengthInBytes, IID_PPV_ARGS(&outSignature));
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	return true;
}

bool IDevice::CreateGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, _OUT_ ID3D12PipelineState*& outPipelineStateObject) const
{
	HRESULT hr = _device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&outPipelineStateObject));
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	return true;
}

bool IDevice::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC& desc, IDescriptorHeap& outDescriptorHeap) const
{
	ID3D12DescriptorHeap* descriptorHeap;
	HRESULT hr = _device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	outDescriptorHeap.SetDescriptorHeap(descriptorHeap);

	return true;
}

void IDevice::CreateShaderResourceView(
	const IResource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc, const D3D12_CPU_DESCRIPTOR_HANDLE handle
) const noexcept
{
	_device->CreateShaderResourceView(resource->GetResourceWritable(), desc, handle);
}