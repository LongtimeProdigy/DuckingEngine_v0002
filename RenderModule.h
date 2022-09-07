#pragma once

#include "Matrix4x4.h"

struct ID3D12Device8;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
enum D3D12_RESOURCE_STATES;
enum D3D12_HEAP_TYPE;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

struct IDXGISwapChain4;

enum class MaterialParameterType : uint8;
struct MaterialParameterDefinition;
class Material;
struct MaterialDefinition;
class TextureRaw;

struct RenderPass
{
	~RenderPass();

	// definition
	DKVector<MaterialParameterDefinition> _parameters;
	MaterialDefinition _materialDefinition;

	// pipeline
	ID3D12RootSignature* _rootSignature;
	ID3D12PipelineState* _pipelineStateObject;
};

struct DKCommandList
{
	dk_inline DKCommandList(ID3D12CommandAllocator* commandAllocator, ID3D12GraphicsCommandList* commandList)
		: _commandAllocator(commandAllocator)
		, _commandList(commandList)
	{}

	~DKCommandList();

	ID3D12CommandAllocator* _commandAllocator;
	ID3D12GraphicsCommandList* _commandList;
};

class RenderModule
{
public:
	constexpr static uint kFrameCount = 3;
	static uint kCurrentFrameIndex;

public:
	~RenderModule();

	bool initialize(const HWND hwnd, const uint width, const uint height);

	ID3D12Resource* createDefaultBuffer(const void* data, const uint size, const D3D12_RESOURCE_STATES state) const;
	ID3D12Resource* createUploadBuffer(const void* data, const uint size, const D3D12_RESOURCE_STATES state) const;
	const bool loadVertexBuffer(const char* modelPath, const uint subMeshIndex, const void* data, const uint strideSize, const uint bufferSize, VertexBufferViewRef& outView);
	const bool loadIndexBuffer(const void* data, const uint bufferSize, IndexBufferViewRef& outView);

	// material 관련 (SceneRenderer로 옮길 필요가 있어보임)
	const MaterialDefinition* findRenderPassByMaterialName(const DKString& materialName) const;

	// Texture Manager 전용함수!
	// 사용시 주의할것!
	const bool createTextureBindlessDescriptorHeap(ID3D12DescriptorHeap* outDescriptorHeap) const;
	bool createTexture(const TextureRaw& textureRaw, const D3D12_CPU_DESCRIPTOR_HANDLE& handle) const;

private:
	bool initialize_createDeviceAndCommandQueueAndSwapChain(const HWND hwnd, const uint width, const uint height);
	bool initialize_loadRenderPass();
	DKCommandList* createCommandList();
	bool createMaterialDefinition(const char* materialPath, MaterialDefinition& outMaterialDefinition) const;
	bool createRootSignature(RenderPass& inoutRenderPass) const;
	bool createPipelineObjectState(const char* vsPath, const char* vsEntry, const char* psPath, const char* psEntry, RenderPass& inoutRenderPass) const;
	bool initialize_createFence();
	bool initialize_createFenceEvent();

	ID3D12Resource* createBufferInternal(const void* data, const uint size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state) const;

	void waitFenceAndResetCommandList();
	void execute();
	void executeImmediately();

private:
	bool _useWarpDevice = false;
	ID3D12Device8* _device = nullptr;
	ID3D12CommandQueue* _commandQueue;
	IDXGISwapChain4* _swapChain = nullptr;
	ID3D12Resource* _renderTargetResources[kFrameCount];
	ID3D12DescriptorHeap* _renderTargetDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* _depthStencilDescriptorHeap = nullptr;
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

#if defined(USE_IMGUI)
	ID3D12DescriptorHeap* _pd3dSrvDescHeap;
#endif

	DKCommandList* _commandList = nullptr;

	uint _fenceValues[kFrameCount] = { 0, };
	ID3D12Fence* _fences[kFrameCount];
	HANDLE _fenceEvent;

	DKHashMap<const char*, RenderPass> _renderPassMap;
};