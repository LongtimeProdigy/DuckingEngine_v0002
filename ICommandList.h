#pragma once

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

struct ID3D12GraphicsCommandList;
struct D3D12_RESOURCE_BARRIER;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
//struct D3D12_RECT;
enum D3D12_CLEAR_FLAGS;
struct ID3D12DescriptorHeap;
//struct D3D12_GPU_VIRTUAL_ADDRESS;
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;

class ICommandAllocator;
class IPipelineStateObject;
class IRootSignature;
class IDescriptorHeap;

class ICommandList
{
public:
	ICommandList(ID3D12GraphicsCommandList* commandList)
		: _commandList(commandList)
	{}
	~ICommandList();

	dk_inline const ID3D12GraphicsCommandList* GetCommandList() const noexcept
	{
		return _commandList;
	}
	dk_inline ID3D12GraphicsCommandList* GetCommandListWritable() const noexcept
	{
		return _commandList;
	}

	// #todo- 현재 Reset은 PSO가 무조건 nullptr로 설정됩니다. 구조적 문제.. 해결해야함
	// Reset하고 SetPipelineStateObject 차이가 얼마나 나는지 조사도하고
	bool Reset(const ICommandAllocator* commandAllocator, const IPipelineStateObject* pso) const;
	bool Close() const;
	void ResourceBarrier(const uint barriorCount, const D3D12_RESOURCE_BARRIER* barriers) const noexcept;
	void OMSetRenderTargets(
		const uint renderTargetDescriptorCount, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetDescriptors,
		const bool RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilDescriptor) const noexcept;
	void ClearRenderTargetView(
		const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, const float* colorRGBA, const uint rectCount, const D3D12_RECT* rect
	) const noexcept;
	void ClearDepthStencilView(
		const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, const D3D12_CLEAR_FLAGS flag, const float depth, const uint8 stencil, const uint rectCount, const D3D12_RECT* rect
	) const noexcept;
	//void SetDescriptorHeaps(const uint descriptorHeapCount, ID3D12DescriptorHeap* const* descriptors) const noexcept;
	void SetGraphicsRootSignature(const IRootSignature* rootSignature) const noexcept;
	void SetGraphicsRoot32BitConstant(const uint rootPatameterIndex, const uint data, const uint destOffsetIn32BitValues) const noexcept;
	void SetGraphicsRootConstantBufferView(const uint rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS bufferAdress) const noexcept;
	void RSSetViewports(const uint viewportCount, const D3D12_VIEWPORT* viewports) const noexcept;
	void RSSetScissorRects(const uint scissorCount, const D3D12_RECT* scissors) const noexcept;
	void IASetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) const noexcept;
	void IASetVertexBuffers(const uint startSlot, const uint viewCount, const D3D12_VERTEX_BUFFER_VIEW* views) const noexcept;
	void IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* views) const noexcept;
	void SetPipelineState(const IPipelineStateObject* pipelineStateObject) const noexcept;
	void SetDescriptorHeaps(const std::vector<ID3D12DescriptorHeap*>& descriptorHeaps) const noexcept;
	void SetGraphicsRootDescriptorTable(const uint rootParameterIndex, const D3D12_GPU_DESCRIPTOR_HANDLE& handle) const noexcept;
	void DrawIndexedInstanced(const uint indexCountPerInstance, const uint instanceCount, const uint startIndexLocation, const uint baseVertexLocation, const uint startInstanceLocation) const noexcept;

private:
	ID3D12GraphicsCommandList* _commandList;
};

