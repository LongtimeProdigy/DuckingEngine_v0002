#pragma once

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
struct D3D12_VIEWPORT;
typedef RECT D3D12_RECT;
typedef UINT64 D3D12_GPU_VIRTUAL_ADDRESS;
struct IDXGISwapChain4;
struct D3D12Resource;
enum D3D12_PRIMITIVE_TOPOLOGY_TYPE;

struct IDxcBlob;

namespace DK
{
	struct IBuffer;
	struct DKCommandList;

	// MaterialParameter로 쓰이는 Type은 POD를 유지해야합니다. (memcpy를 하기때문)
	class ITexture
	{
	public:
		using TextureSRVType = uint32;
		static constexpr TextureSRVType kErrorTextureSRVIndex = 0xffffffff;
	public:
		ITexture(const DKString& path, const TextureSRVType& textureSRV)
			: _path(path)
			, _textureSRVIndex(textureSRV)
		{}

		dk_inline const DKString& getPath() const
		{
			return _path;
		}

		dk_inline const TextureSRVType& getSRV() const noexcept
		{
			return _textureSRVIndex;
		}

	private:
		DKString _path;
		TextureSRVType _textureSRVIndex = kErrorTextureSRVIndex;
	};

	enum class ShaderParameterType
	{
		Buffer,
		StructuredBuffer,
		Count
	};

	struct ShaderParameter
	{
		ShaderParameterType _type = ShaderParameterType::Count;
		uint32 _register = -1;
		uint32 _rootParameterIndex = -1;		// createRenderPass 시점에 설정 (나머지는 Resource로부터)
	};

	struct Pipeline
	{
	public:
		struct CreateInfo
		{
		public:
			struct LayoutInfo
			{
			public:
				enum class Type
				{
					UINT4, 
					FLOAT2, 
					FLOAT3, 
					FLOAT4, 
				};

				Type _type;
				DKString _name;
			};

			const char* _primitiveTopologyType;
			const char* _depthEnable;
			const char* _vertexShaderPath;
			const char* _vertexShaderEntry;
			const char* _pixelShaderPath;
			const char* _pixelShaderEntry;

			DKVector<LayoutInfo> _layout;
			DKHashMap<DKString, ShaderParameter> _shaderParameterMap;
		};

		RenderResourcePtr<ID3D12RootSignature> _rootSignature;
		RenderResourcePtr<ID3D12PipelineState> _pipelineStateObject;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE _primitiveTopologyType;
		DKHashMap<DKString, ShaderParameter> _shaderParameterMap;

		dk_inline ShaderParameter* getShaderParameter(const DKString& name)
		{
			DKHashMap<DKString, ShaderParameter>::iterator iter = _shaderParameterMap.find(name);
			DK_ASSERT_LOG(iter != _shaderParameterMap.end(), "존재하지 않는 ShaderParameter을 찾습니다.\nShaderParameterName: %s", name.c_str());
#ifdef _DK_DEBUG_
			if (iter == _shaderParameterMap.end())
				return nullptr;
#endif

			return &iter->second;
		}
	};

	struct RenderPass
	{
		struct CreateInfo
		{
			DKHashMap<DKString, ShaderParameter> _shaderParameterMap;
			DKVector<DKPair<DKString, Pipeline::CreateInfo>> _pipelineArr;
		};

		DKHashMap<DKString, ShaderParameter> _shaderParameterMap;
		DKHashMap<DKString, Pipeline> _pipelineMap;

		dk_inline ShaderParameter* getShaderParameter(const DKString& name)
		{
			DKHashMap<DKString, ShaderParameter>::iterator iter = _shaderParameterMap.find(name);
			// RenderPass에서 없다면 Pipeline에서 찾아야함
			//DK_ASSERT_LOG(iter != _shaderParameterMap.end(), "존재하지 않는 ShaderParameter을 찾습니다.\nShaderParameterName: %s", name.c_str());
#ifdef _DK_DEBUG_
			if (iter == _shaderParameterMap.end())
				return nullptr;
#endif

			return &iter->second;
		}
		dk_inline Pipeline* getPipeline(const DKString& pipelineName)
		{
			DKHashMap<DKString, Pipeline>::iterator iter = _pipelineMap.find(pipelineName);
			DK_ASSERT_LOG(iter != _pipelineMap.end(), "존재하지 않는 Pipeline을 찾습니다.\nPipelineName: %s", pipelineName.c_str());
#ifdef _DK_DEBUG_
			if (iter == _pipelineMap.end())
				return nullptr;
#endif

			return &iter->second;
		}
	};

	static RenderPass* currentRenderPass = nullptr;
	static Pipeline* currentPipeline = nullptr;

#ifdef _DK_DEBUG_
#define RENDERING_ALREADY_BIND(object, name) \
if(object != nullptr) \
{ \
	DK_ASSERT_LOG(false, "이미 Bind되어 있습니다. Bind를 건너뜁니다. Name: %s", name); \
	break; \
}
#define RENDERING_VERIFY(object, name) \
if (object == nullptr) \
{ \
	DK_ASSERT_LOG(false, "Object를 찾지 못했습니다. Bind를 건너뜁니다. Name: %s", name); \
	break; \
}
#else
#define RENDERING_ALREADY_BIND(object)
#define RENDERING_VERIFY(object)
#endif

#define startRenderPass(renderModule, renderPassName, renderTargetBuffer) \
do{ \
	RenderModule& currentRenderModule = renderModule; \
	RENDERING_ALREADY_BIND(currentRenderPass, renderPassName); \
	currentRenderPass = currentRenderModule.getRenderPass(renderPassName); \
	RENDERING_VERIFY(currentRenderPass, renderPassName); \
	currentRenderModule.bindRenderPass(renderTargetBuffer);

#define endRenderPass() \
	currentRenderPass = nullptr; \
}while(false)

#define startPipeline(pipelineName) \
do{ \
	RENDERING_ALREADY_BIND(currentPipeline, pipelineName); \
	currentPipeline = currentRenderPass->getPipeline(pipelineName); \
	RENDERING_VERIFY(currentPipeline, pipelineName); \
	currentRenderModule.bindPipeline(*currentPipeline);

#define endPipeline() \
	currentPipeline = nullptr; \
}while(false)

#define setConstantBuffer(name, address) \
{ \
	static const ShaderParameter* shaderParameter = currentRenderPass->getShaderParameter(name) != nullptr ? currentRenderPass->getShaderParameter(name) : currentPipeline->getShaderParameter(name); \
	RENDERING_VERIFY(shaderParameter, name); \
	currentRenderModule.bindConstantBuffer(shaderParameter->_rootParameterIndex, address); \
}
#define setShaderResourceView(name, address) \
{ \
	static const ShaderParameter* shaderParameter = currentRenderPass->getShaderParameter(name) != nullptr ? currentRenderPass->getShaderParameter(name) : currentPipeline->getShaderParameter(name); \
	RENDERING_VERIFY(shaderParameter, name); \
	currentRenderModule.bindShaderResourceView(shaderParameter->_rootParameterIndex, address); \
}

	class RenderModule
	{
	public:
		static constexpr uint32 kFrameCount = 2;
		static uint32 kCurrentFrameIndex;

	public:
		~RenderModule();

		bool initialize(const HWND hwnd, const uint32 width, const uint32 height);

		bool createRenderPass(const DKString& renderPassName, RenderPass::CreateInfo&& renderPassCreateInfo);
		// #todo- Container 이용해도될듯?
		IBuffer* createUploadBuffer(const uint32 size, const DKStringW& debugName);
		const bool createVertexBuffer(const void* data, const uint32 strideSize, const uint32 vertexCount, VertexBufferViewRef& outView, const DKStringW& debugName);
		const bool createIndexBuffer(const void* data, const uint32 bufferSize, IndexBufferViewRef& outView, const DKStringW& debugName);

		// SceneRenderer 전용 함수
		void preRender();			// RenderTaget 등을 설정하는데.. 이건 RenderPass Set으로 옮겨야할듯. 지금은 RenderTarget이 하나니 하나로 빼둠
		void bindRenderPass(bool renderTargetBuffer);
		bool bindPipeline(Pipeline& pipeline);
		void bindConstantBuffer(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress);
		void bindShaderResourceView(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress);
		void setVertexBuffers(const uint32 startSlot, const uint32 numViews, const D3D12_VERTEX_BUFFER_VIEW* view);
		void setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view);
		void drawIndexedInstanced(const uint32 indexCountPerInstance, const uint32 instanceCount, const uint32 startIndexLocation, const int baseVertexLocation, const uint32 startInstanceLocation);
		void endRender();

		// helper 함수
		ITextureRef createTexture(const DKString& path);
		ITextureRef& getDepthStencilTextureWritable() { return _depthStencilTexture; }
		dk_inline RenderPass* getRenderPass(const DKString& renderPassName)
		{
			using FindResult = DKHashMap<DKString, RenderPass>::iterator;
			FindResult find = _renderPassMap.find(renderPassName);
			DK_ASSERT_LOG(find != _renderPassMap.end(), "존재하지 않는 RenderPass를 Bind시도합니다.\nName: %s", renderPassName.c_str());
#ifdef _DK_DEBUG_
			if (find == _renderPassMap.end())
				return nullptr;
#endif

			return &find->second;
		}

	private:
		bool initialize_createDeviceAndCommandQueueAndSwapChain(const HWND hwnd, const uint32 width, const uint32 height);
		DKCommandList* createCommandList();
		bool initialize_createFence();
		bool createRootSignature(RenderPass& renderPass, Pipeline& inoutPipeline);
		bool createPipelineObjectState(const Pipeline::CreateInfo& pipelineCreateInfo, Pipeline& inoutPipeline);

		ITextureRef createTextureSRV(const DKString& name, ID3D12Resource* textureBuffer, const DXGI_FORMAT format);

		ID3D12Resource* createBufferInternal(const uint32 size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state, const DKStringW& debugName);
		ID3D12Resource* createDefaultBuffer(const uint32 size, const D3D12_RESOURCE_STATES state, const DKStringW& debugName);
		ID3D12Resource* createInitializedDefaultBuffer(const void* data, const uint32 bufferSize, const DKStringW& debugName);

		void waitFenceAndResetCommandList();
		void execute();

	private:
		bool _useWarpDevice = false;
		RenderResourcePtr<ID3D12Device8> _device = nullptr;
		RenderResourcePtr<ID3D12CommandQueue> _commandQueue;
		Ptr<DKCommandList> _commandList = nullptr;
		uint32 _fenceValues[kFrameCount] = { 0, };
		RenderResourcePtr<ID3D12Fence> _fences[kFrameCount];
		HANDLE _fenceEvent;
		Ptr<D3D12_VIEWPORT> _viewport;
		Ptr<D3D12_RECT> _scissorRect;
		RenderResourcePtr<IDXGISwapChain4> _swapChain = nullptr;

		RenderResourcePtr<ID3D12Resource> _renderTargetResourceArr[kFrameCount];	// Deffered
		RenderResourcePtr<ID3D12Resource> _backBufferResource[kFrameCount];			// BackBuffer
		RenderResourcePtr<ID3D12DescriptorHeap> _renderTargetViewHeap = nullptr;	// FrameCount * 2개수 (앞은 Deffered Rendering RTV, 뒤는 SwapChain BackBuffer RTV)
		ITextureRef _renderTargetTextureArr[kFrameCount];
		RenderResourcePtr<ID3D12Resource2> _depthStencilBuffer;
		RenderResourcePtr<ID3D12DescriptorHeap> _depthStencilDescriptorHeap = nullptr;
		ITextureRef _depthStencilTexture;


		// SwapChain

#if defined(USE_IMGUI)
		RenderResourcePtr<ID3D12DescriptorHeap> _pd3dSrvDescHeap;
#endif

		// Texture
		DKVector<ITexture::TextureSRVType> _deletedTextureSRVArr;
		DKHashMap<DKString, ITextureRef> _textureContainer;
		RenderResourcePtr<ID3D12DescriptorHeap> _textureDescriptorHeap;

		DKHashMap<DKString, RenderPass> _renderPassMap;
	};

	struct IBuffer
	{
	public:
		IBuffer(RenderResourcePtr<ID3D12Resource> buffers[RenderModule::kFrameCount], const uint32 bufferSize)
			: _bufferSize(bufferSize)
		{
			for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
				_buffers[i] = buffers[i];
		}

		void upload(const void* data);
		D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress();

	private:
		RenderResourcePtr<ID3D12Resource> _buffers[RenderModule::kFrameCount];
		const uint32 _bufferSize;
	};

	struct DKCommandList
	{
		dk_inline DKCommandList(RenderResourcePtr<ID3D12CommandAllocator> commandAllocators[RenderModule::kFrameCount], RenderResourcePtr<ID3D12GraphicsCommandList>& commandList)
			: _commandList(commandList)
		{
			for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
			{
				_commandAllocators[i] = commandAllocators[i];
			}
		}

		bool reset();

		RenderResourcePtr<ID3D12CommandAllocator> _commandAllocators[RenderModule::kFrameCount];
		RenderResourcePtr<ID3D12GraphicsCommandList> _commandList;
	};
}