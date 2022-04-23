#include "stdafx.h"
#include "ICommandList.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
//#pragma comment(lib, "d3dcompiler.lib")
//#include <d3dcompiler.h>
#include "d3dx12.h"

#include "ICommandAllocator.h"
#include "IPipelineStateObject.h"
#include "IResource.h"
#include "IDescriptorHeap.h"
#include "IRootSignature.h"

ICommandList::~ICommandList()
{
	_commandList->Release();
}

bool ICommandList::Reset(const ICommandAllocator* commandAllocator, const IPipelineStateObject* pso) const
{
	return SUCCEEDED(_commandList->Reset(commandAllocator->GetCommnadAllocatorWritable(), nullptr /*pso->GetPipelineStateObjectWritable()*/));
}

bool ICommandList::Close() const
{
	HRESULT hr = _commandList->Close();
	return SUCCEEDED(hr);
}

void ICommandList::ResourceBarrier(const uint barrierCount, const D3D12_RESOURCE_BARRIER* barriers) const noexcept
{
	_commandList->ResourceBarrier(barrierCount, barriers);
}

void ICommandList::OMSetRenderTargets(
	const uint renderTargetDescriptorCount, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetDescriptors, 
	const bool RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilDescriptor) const noexcept
{
	_commandList->OMSetRenderTargets(renderTargetDescriptorCount, renderTargetDescriptors, RTsSingleHandleToDescriptorRange, depthStencilDescriptor);
}

void ICommandList::ClearRenderTargetView(
	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, const float* colorRGBA, const uint rectCount, const D3D12_RECT* rect
) const noexcept
{
	_commandList->ClearRenderTargetView(renderTargetView, colorRGBA, rectCount, rect);
}

void ICommandList::ClearDepthStencilView(
	const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, const D3D12_CLEAR_FLAGS flag, const float depth, const uint8 stencil, const uint rectCount, const D3D12_RECT* rect
) const noexcept
{
	_commandList->ClearDepthStencilView(depthStencilView, flag, depth, stencil, rectCount, rect);
}

//void ICommandList::SetDescriptorHeaps(const uint descriptorHeapCount, ID3D12DescriptorHeap* const* descriptors) const noexcept
//{
//	_commandList->SetDescriptorHeaps(descriptorHeapCount, descriptors);
//}

void ICommandList::SetGraphicsRootSignature(const IRootSignature* rootSignature) const noexcept
{
	_commandList->SetGraphicsRootSignature(rootSignature->GetRootSignatureWritable());
}

void ICommandList::SetGraphicsRoot32BitConstant(const uint rootPatameterIndex, const uint data, const uint destOffsetIn32BitValues) const noexcept
{
	_commandList->SetGraphicsRoot32BitConstant(rootPatameterIndex, data, destOffsetIn32BitValues);
}

void ICommandList::SetGraphicsRootConstantBufferView(const uint rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS bufferAdress) const noexcept
{
	_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, bufferAdress);
}

void ICommandList::RSSetViewports(const uint viewportCount, const D3D12_VIEWPORT* viewports) const noexcept
{
	_commandList->RSSetViewports(viewportCount, viewports);
}

void ICommandList::RSSetScissorRects(const uint scissorCount, const D3D12_RECT* scissors) const noexcept
{
	_commandList->RSSetScissorRects(scissorCount, scissors);
}

void ICommandList::IASetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) const noexcept
{
	_commandList->IASetPrimitiveTopology(primitiveTopology);
}

void ICommandList::IASetVertexBuffers(const uint startSlot, const uint viewCount, const D3D12_VERTEX_BUFFER_VIEW* views) const noexcept
{
	_commandList->IASetVertexBuffers(startSlot, viewCount, views);
}

void ICommandList::IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* views) const noexcept
{
	_commandList->IASetIndexBuffer(views);
}

void ICommandList::SetPipelineState(const IPipelineStateObject* pipelineStateObject) const noexcept
{
	_commandList->SetPipelineState(pipelineStateObject->GetPipelineStateObjectWritable());
}

void ICommandList::SetDescriptorHeaps(const std::vector<ID3D12DescriptorHeap*>& descriptorHeaps) const noexcept
{
	_commandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), &descriptorHeaps[0]);
}

void ICommandList::SetGraphicsRootDescriptorTable(const uint rootParameterIndex, const D3D12_GPU_DESCRIPTOR_HANDLE& handle) const noexcept
{
	_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, handle);
}

void ICommandList::DrawIndexedInstanced(const uint indexCountPerInstance, const uint instanceCount, const uint startIndexLocation, const uint baseVertexLocation, const uint startInstanceLocation) const noexcept
{
	_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startInstanceLocation, baseVertexLocation, startInstanceLocation);
}