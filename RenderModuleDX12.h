#pragma once

#include "RenderModuleDX.h"

#define USE_D3DX12_HELPER

class IDevice;
class ICommandQueue;
class ISwapChain;
class IDescriptorHeap;
class IResource;
class ICommandAllocator;
class ICommandList;
class IFence;
class IRootSignature;
class IPipelineStateObject;
class IShader;
class ITexture;

enum class MaterialType : uint8;
class Material;
class SkinnedMeshComponent;

class RenderModuleDX12 : public RenderModuleDX
{
public:
	RenderModuleDX12();
	virtual ~RenderModuleDX12() override final;

	virtual bool Initialize(HWND hwnd, uint width, uint height) override final;

	virtual Material* CreateMaterial(const MaterialType materialType, const SceneObject* sceneObject) override;
	virtual IResource* CreateBuffer(void* data, uint size) const override;
	virtual IResource* CreateUploadBuffer(void* data, uint size) const override;
	virtual const bool LoadVertexBuffer(const void* data, const uint strideSize, const uint bufferSize, VertexBufferViewRef& outView) override;
	virtual const bool LoadIndexBuffer(const void* data, const uint bufferSize, IndexBufferViewRef& outView) override;
	virtual ITexture CreateTexture(const char* texturePath, const D3D12_CPU_DESCRIPTOR_HANDLE& handle) override;

	virtual void UpdateUploadResource(IResource* resource, void* data, uint size) const noexcept override;

	virtual void PreRender() const noexcept override;
	virtual bool RenderSkinnedMeshComponent(const SkinnedMeshComponent* skinnedMeshComponent) const override;
	virtual void EndRender() const noexcept override;

	virtual void RenderUI() const noexcept override;

	const bool LoadRootSignature(const MaterialType materialType, IRootSignatureRef& outRootSignature);
	const bool LoadPipelineStateObject(const MaterialType materialType, const IRootSignature& rootSignature, IPipelineStateObjectRef& outIPipelineStateObject);
	const bool CreateDescriptorHeap(const MaterialType materialType, IDescriptorHeap& outDescriptorHeap);

private:
	IResource* CreateBufferInternal(const void* data, const uint size) const;
	const bool LoadBufferInternal(std::unordered_map<const void*, IResourceRef>& container, const void* data, const uint size, IResourceRef& outBuffer);
	const bool LoadVertexShader(const char* path, IShaderRef& outShader);
	const bool LoadPixelShader(const char* path, IShaderRef& outShader);
	//bool CreateSRVFromTexture(
	//	D3D12_RESOURCE_DESC& textureDesc, void* data, int imageBytesPerRow, uint bufferSize, uint descriptorHeapIndex, uint offset
	//);

	bool WaitForPreviousFrame() const;
	bool ExcuteCommandList() const;

public:
#pragma region Initialize Variable
	// #todo- ...흠... 굳이 포인터로 가지고 있어야할까?
	bool _useWarpDevice = false;
	mutable uint _currentFrame = 0;

	IDevice* _device = nullptr;
	ICommandQueue* _commandQueue = nullptr;
	ISwapChain* _swapChain = nullptr;

	std::vector<ICommandAllocator*> _commandAllocators;
	ICommandList* _commandList;

	std::vector<IFence*> _fences;
	mutable std::vector<uint> _fenceValues;
	HANDLE _fenceEvent;

	std::vector<IResource*> _renderTargetResources;
	IDescriptorHeap* _renderTargetDescriptorHeap = nullptr;

	IDescriptorHeap* _depthStencilDescriptorHeap = nullptr;
#pragma endregion

	std::unordered_map<MaterialType, IRootSignatureRef> _rootSignatureContainer;
	std::unordered_map<MaterialType, IPipelineStateObjectRef> _pipelineStateObjectContainer;
	// note(220313): vertexBufferContainer, pixelBufferContainer 모두 현재 해당 data의 주소값을 키값으로 사용합니다.
	// 추후 고유 ID로 변경하는 게 좋아보임..
	std::unordered_map<const void*, IResourceRef> _vertexBufferContainer;
	std::unordered_map<const void*, VertexBufferViewRef> _vertexBufferViewContainer;
	std::unordered_map<const void*, IResourceRef> _indexBufferContainer;
	std::unordered_map<const void*, IndexBufferViewRef> _indexBufferViewContainer;
	std::unordered_map<const char*, IShaderRef> _vertexShaderContainer;
	std::unordered_map<const char*, IShaderRef> _pixelShaderContainer;
};