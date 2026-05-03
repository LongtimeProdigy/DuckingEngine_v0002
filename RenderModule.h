#pragma once

struct ID3D12Device8;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
enum D3D12_RESOURCE_STATES;
enum D3D12_RESOURCE_BARRIER_TYPE;
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

	using TextureResourceViewType = uint32;

	struct RootConstant32BitParameter
	{
		DKVector<DKString> _parameters;
		uint32 _register = -1;

		uint32 _offset;
		uint32 _rootParameterIndex = -1;		// createRenderPass 시점에 설정 (나머지는 Resource로부터)
	};

	struct RootConstant32BitParameterBindingInfo
	{
		uint32 _rootParameterIndex;
		uint32 _offset;
		void* _buffer;
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

			enum class FillMode
			{
				WIREFRAME,
				SOLID,
				COUNT
			};
			enum class CullMode
			{
				NONE,
				FRONT,
				BACK,
				COUNT
			};

			const char* _primitiveTopologyType;
			bool _depthEnable;
			FillMode _fillMode;
			CullMode _cullMode;
			const char* _vertexShaderPath = nullptr;
			const char* _vertexShaderEntry = nullptr;
			const char* _pixelShaderPath = nullptr;
			const char* _pixelShaderEntry = nullptr;
			const char* _computeShaderPath = nullptr;
			const char* _computeShaderEntry = nullptr;

			DKVector<LayoutInfo> _layout;
			DKVector<RootConstant32BitParameter> _rootConstant32BitParameter;
			DKHashMap<DKString, ShaderParameter> _shaderParameterMap;
		};
		enum class Type : uint8
		{
			COMPUTE,
			GRAPHIC,
			COUNT
		};

		Type _type = Type::COUNT;

		RenderResourcePtr<ID3D12RootSignature> _rootSignature;
		RenderResourcePtr<ID3D12PipelineState> _pipelineStateObject;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE _primitiveTopologyType;
		DKVector<DKVector<char>> _rootConstant32BitParameterBuffer;
		DKHashMap<DKString, RootConstant32BitParameterBindingInfo> _rootConstant32BitParameterMap;
		DKHashMap<DKString, ShaderParameter> _shaderParameterMap;

		// static하게만 호출해야합니다.
		dk_inline RootConstant32BitParameterBindingInfo* getRootConstantParameter(const DKString& name)
		{
			DKHashMap<DKString, RootConstant32BitParameterBindingInfo>::iterator iter = _rootConstant32BitParameterMap.find(name);
			DK_ASSERT_LOG(iter != _rootConstant32BitParameterMap.end(), "존재하지 않는 RootConstantParameter을 찾습니다.\nRootConstantParameterName: %s", name.c_str());
#ifdef _DK_DEBUG_
			if (iter == _rootConstant32BitParameterMap.end())
				return nullptr;
#endif

			return &iter->second;
		}
		// static하게만 호출해야합니다.
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

#define startRenderPass(renderModule, renderPassName, rtvPrevSlot, rtvSlot, bindDSV, clearTarget) \
do{ \
	RenderModule& currentRenderModule = renderModule; \
	RENDERING_ALREADY_BIND(currentRenderPass, renderPassName); \
	static RenderPass* findRenderPass = currentRenderModule.getRenderPass(renderPassName); \
	currentRenderPass = findRenderPass; \
	RENDERING_VERIFY(currentRenderPass, renderPassName); \
	currentRenderModule.bindRenderPass(rtvPrevSlot, rtvSlot, bindDSV, clearTarget)

#define endRenderPass() \
	currentRenderPass = nullptr; \
}while(false)

#define startPipeline(pipelineName) \
do{ \
	RENDERING_ALREADY_BIND(currentPipeline, pipelineName); \
	static Pipeline* findPipeline = currentRenderPass->getPipeline(pipelineName); \
	currentPipeline = findPipeline; \
	RENDERING_VERIFY(currentPipeline, pipelineName); \
	currentRenderModule.bindPipeline(*currentPipeline, currentPipeline->_type == Pipeline::Type::COMPUTE)

#define endPipeline() \
	currentPipeline = nullptr; \
}while(false)

#define setRootConstantParameter(name, value) \
{ \
	DK_ASSERT_LOG(sizeof(value), "현재 RootConstant는 4byte만 지원합니다.\nName: %s", name); \
	static RootConstant32BitParameterBindingInfo* bindingInfo = currentPipeline->getRootConstantParameter(name); \
	RENDERING_VERIFY(bindingInfo, name); \
	DK::memcpy(((uint8*)bindingInfo->_buffer + bindingInfo->_offset), &value, 4); \
	currentRenderModule.setRoot32BitConstants(bindingInfo->_rootParameterIndex, 1, &value, bindingInfo->_offset, currentPipeline->_type == Pipeline::Type::COMPUTE); \
}

#define setConstantBuffer(name, address) \
{ \
	static const ShaderParameter* shaderParameter = currentRenderPass->getShaderParameter(name) != nullptr ? currentRenderPass->getShaderParameter(name) : currentPipeline->getShaderParameter(name); \
	RENDERING_VERIFY(shaderParameter, name); \
	currentRenderModule.bindConstantBuffer(shaderParameter->_rootParameterIndex, address, currentPipeline->_type == Pipeline::Type::COMPUTE); \
}
#define setShaderResourceView(name, address) \
{ \
	static const ShaderParameter* shaderParameter = currentRenderPass->getShaderParameter(name) != nullptr ? currentRenderPass->getShaderParameter(name) : currentPipeline->getShaderParameter(name); \
	RENDERING_VERIFY(shaderParameter, name); \
	currentRenderModule.bindShaderResourceView(shaderParameter->_rootParameterIndex, address, currentPipeline->_type == Pipeline::Type::COMPUTE); \
}

	class RenderModule
	{
	public:
		static constexpr uint32 kFrameCount = 2;
		static uint32 kCurrentFrameIndex;
		static uint32 kWidth;
		static uint32 kHeight;

	public:
		~RenderModule();

		bool initialize(const HWND hwnd, const uint32 width, const uint32 height);
		bool postInitialize();

		bool createRenderPass(const DKString& renderPassName, RenderPass::CreateInfo&& renderPassCreateInfo);
		// #todo- Container 이용해도될듯?
		IBuffer* createUploadBuffer(const uint32 size, const DKStringW& debugName);
		const bool createVertexBuffer(const void* data, const uint32 strideSize, const uint32 vertexCount, VertexBufferViewRef& outView, const DKStringW& debugName);
		const bool createIndexBuffer(const void* data, const uint32 bufferSize, IndexBufferViewRef& outView, const DKStringW& debugName);

		// SceneRenderer 전용 함수
		void resourceBarrierTransition(const ITextureRef& texture, const D3D12_RESOURCE_STATES beforeState, const D3D12_RESOURCE_STATES afterState);
		void resourceBarrier(ID3D12Resource* resource, const D3D12_RESOURCE_BARRIER_TYPE barrierType);

		void preRender();			// RenderTaget 등을 설정하는데.. 이건 RenderPass Set으로 옮겨야할듯. 지금은 RenderTarget이 하나니 하나로 빼둠
		void bindRenderPass(const uint32 rtvReadSlot, const uint32 rtvSlot, const bool bindDSV, const bool clearTarget);
		bool bindPipeline(Pipeline& pipeline, const bool isCompute);
		void setRoot32BitConstants(const uint32 rootParameterIndex, const uint32 count, const void* data, uint32 offset, const bool isCompute);
		void bindConstantBuffer(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress, const bool isCompute);
		void bindShaderResourceView(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress, const bool isCompute);
		void setVertexBuffers(const uint32 startSlot, const uint32 numViews, const D3D12_VERTEX_BUFFER_VIEW* view);
		void setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view);
		void drawIndexedInstanced(const uint32 indexCountPerInstance, const uint32 instanceCount, const uint32 startIndexLocation, const int baseVertexLocation, const uint32 startInstanceLocation);
		void dispatch(const uint32 threadGroupCountX, const uint32 threadGroupCountY, const uint32 threadGroupCountZ);
		void endRender();

		// helper 함수
		ITextureRef createTexture(const DKString& path, const uint32 width, const uint32 height, const byte* data, const uint8 mipLevelCount, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES state, const bool createSRV, const bool createUAV);
		ITextureRef loadAndCreateTexture(const DKString& path);
		void deleteTexture(ITexture* texture);
		void deallocateTextureSRV(const TextureResourceViewType index);
		void deallocateTextureUAV(const TextureResourceViewType index);

		dk_inline RenderPass* getRenderPass(const DKString& renderPassName)
		{
			using FindResult = DKHashMap<DKString, RenderPass>::iterator;
			FindResult find = _renderPassMap.find(renderPassName);
			DK_ASSERT_LOG(find != _renderPassMap.end(), "Can't not find RenderPass named %s", renderPassName.c_str());
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
		bool createRootSignature(RenderPass& renderPass, const DKVector<RootConstant32BitParameter>& rootConstant32BitParameters, Pipeline& inoutPipeline);
		bool createPipelineObjectState(const Pipeline::CreateInfo& pipelineCreateInfo, Pipeline& inoutPipeline);

		const bool allocateTextureSRV(ITexture* texture);
		const bool allocateTextureUAV(ITexture* texture);

		ID3D12Resource* createBufferInternal(const uint32 size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state, const DKStringW& debugName);
		ID3D12Resource* createInitializedDefaultBuffer(const void* data, const uint32 bufferSize, const D3D12_RESOURCE_STATES state, const DKStringW& debugName);

		void resourceBarrierTransition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES beforeState, const D3D12_RESOURCE_STATES afterState);

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

		// RenderTarget + BackBuffer
		RenderResourcePtr<ID3D12DescriptorHeap> _renderTargetViewHeap = nullptr;
		// RenderTarget
		static constexpr const uint32 kRenderTargetTextureCount = 4;					// Deffered: 0, 2 / Gbuffer: 1, 3
		ITextureRef _renderTargetTextureArr[kRenderTargetTextureCount];
		// BackBuffer
		RenderResourcePtr<ID3D12Resource> _backBufferResourceArr[kFrameCount];			// BackBuffer

		// DepthStencil
		RenderResourcePtr<ID3D12DescriptorHeap> _depthStencilDescriptorHeap = nullptr;	// Deffered: 0, 2 / Gbuffer: 1, 3
		ITextureRef _depthStencilTextureArr[kRenderTargetTextureCount];

		// SwapChain
#if defined(USE_IMGUI)
		RenderResourcePtr<ID3D12DescriptorHeap> _pd3dSrvDescHeap;
#endif

		// Texture
		static constexpr const uint32 kMaxTextureSRVCount = 1024;
		uint32 _currentTextureSRV = 0;
		uint32 _currentTextureUAV = 0;
		DKVector<TextureResourceViewType> _deletedTextureSRVArr;
		DKVector<TextureResourceViewType> _deletedTextureUAVArr;
		DKHashMap<DKString, ITextureRef> _textureContainer;
		RenderResourcePtr<ID3D12DescriptorHeap> _textureDescriptorHeap;

		DKHashMap<DKString, RenderPass> _renderPassMap;
	};

	struct IBuffer
	{
	public:
		IBuffer(RenderResourcePtr<ID3D12Resource> (&buffers)[RenderModule::kFrameCount], const uint32 bufferSize)
			: _bufferSize(bufferSize)
		{
			for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
				_buffers[i] = buffers[i];
		}

		void upload(const void* data);
		void uploadImmediately(const void* data);
		D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress();

	private:
		RenderResourcePtr<ID3D12Resource> _buffers[RenderModule::kFrameCount];
		const uint32 _bufferSize;
		uint32 _lastUploadIndex = 0;
	};

	class ITexture
	{
		friend class RenderModule;

	public:
		static constexpr TextureResourceViewType kErrorTextureResourceViewIndex = 0xffffffff;

	public:
		ITexture(const DKString& path, const uint8 mipLevelCount, RenderResourcePtr<ID3D12Resource>& textureBuffer, const DXGI_FORMAT format)
			: _path(path)
			, _mipLevelCount(mipLevelCount)
			, _textureBuffer(textureBuffer)
			, _format(format)
		{}
		~ITexture();

		dk_inline const DKString& getPath() const
		{
			return _path;
		}
		dk_inline const uint8 getMipLevelCount() const
		{
			return _mipLevelCount;
		}
		dk_inline const DXGI_FORMAT getFormat() const
		{
			return _format;
		}
		dk_inline ID3D12Resource* getTextureBuffer()
		{
			return _textureBuffer.get();
		}
		dk_inline const TextureResourceViewType& getSRV() const noexcept
		{
			DK_ASSERT_LOG(_textureSRVIndex != kErrorTextureResourceViewIndex, "유효하지 않은 TextureSRV입니다. Path: %s", _path.c_str());
			return _textureSRVIndex;
		}
		dk_inline const TextureResourceViewType& getUAV() const noexcept
		{
			DK_ASSERT_LOG(_textureUAVIndex != kErrorTextureResourceViewIndex, "유효하지 않은 TextureSRV입니다. Path: %s", _path.c_str());
			return _textureUAVIndex;
		}

	private:
		const DKString _path = "";
		const uint8 _mipLevelCount = 1;
		const DXGI_FORMAT _format = DXGI_FORMAT_FORCE_UINT;
		RenderResourcePtr<ID3D12Resource> _textureBuffer;

		TextureResourceViewType _textureSRVIndex = kErrorTextureResourceViewIndex;
		TextureResourceViewType _textureUAVIndex = kErrorTextureResourceViewIndex;
	};

	struct DKCommandList
	{
		dk_inline DKCommandList(RenderResourcePtr<ID3D12CommandAllocator> commandAllocators[RenderModule::kFrameCount], RenderResourcePtr<ID3D12GraphicsCommandList>& commandList)
			: _commandList(commandList)
		{
			for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
				_commandAllocators[i] = commandAllocators[i];
		}

		bool reset();

		RenderResourcePtr<ID3D12CommandAllocator> _commandAllocators[RenderModule::kFrameCount];
		RenderResourcePtr<ID3D12GraphicsCommandList> _commandList;
		uint32 _lastResetIndex = 0;
	};
}
