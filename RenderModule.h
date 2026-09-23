#pragma once

struct ID3D12Device8;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList4;
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
struct D3D12_DISPATCH_RAYS_DESC;

namespace DK
{
	struct IBuffer;
	struct DKCommandList;
	class ShaderCompiler;
	struct ShaderResourceReflection;

	using TextureResourceViewType = uint32;

	struct RootConstant32BitParameter
	{
		DKString _bufferName;
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
		RaytracingAccelerationStructure, 
		Count
	};

	struct ShaderParameter
	{
		ShaderParameterType _type = ShaderParameterType::Count;
		uint32 _register = -1;
		uint32 _space = 0;

		uint32 _rootParameterIndex = uint32 (-1);		// createRenderPass 시점에 설정 (나머지는 Resource로부터)
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

			DKString _primitiveTopologyType;
			bool _depthEnable;
			FillMode _fillMode;
			CullMode _cullMode;
			DKString _vertexShaderPath;
			DKString _vertexShaderEntry;
			DKString _pixelShaderPath;
			DKString _pixelShaderEntry;

			DKString _computeShaderPath;
			DKString _computeShaderEntry;

			DKString _raygenShaderPath;
			DKString _raygenEntry;
			DKString _missShaderPath;
			DKString _missEntry;
			DKString _closestShaderPath;
			DKString _closestEntry;

			DKVector<LayoutInfo> _layout;
			DKVector<RootConstant32BitParameter> _rootConstant32BitParameter;
		};
		enum class Type : uint8
		{
			COMPUTE,
			GRAPHIC,
			RAYTRACING, 
			COUNT
		};

#if defined(_DK_DEBUG_)
		CreateInfo _createInfo;
#endif

		Type _type = Type::COUNT;

		// common
		RenderResourcePtr<ID3D12RootSignature> _rootSignature;
		// graphics/compute
		RenderResourcePtr<ID3D12PipelineState> _pipelineStateObject;
		// compute
		uint32 _threadGroupSize[3] = { 0, 0, 0 };
		// Raytraincg
		RenderResourcePtr<ID3D12StateObject> _rtStateObject;
		RenderResourcePtr<ID3D12StateObjectProperties> _rtStateObjectProperties;

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
			DKVector<DKPair<DKString, Pipeline::CreateInfo>> _pipelineArr;
		};

		DKHashMap<DKString, Pipeline> _pipelineMap;

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

	extern RenderPass* gCurrentBindedRenderPass;
	extern Pipeline* gCurrentBindedPipeline;

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

#define startRenderPass(renderModule, renderPassName, rtvPrevSlot, rtvSlot, bindDSV, clearTarget, isRaytracing) \
do{ \
	RenderModule& currentRenderModule = renderModule; \
	RENDERING_ALREADY_BIND(gCurrentBindedRenderPass, renderPassName); \
	static RenderPass* findRenderPass = currentRenderModule.getRenderPass(renderPassName); \
	gCurrentBindedRenderPass = findRenderPass; \
	RENDERING_VERIFY(gCurrentBindedRenderPass, renderPassName); \
	if(isRaytracing == false) \
		currentRenderModule.bindRenderPass(rtvPrevSlot, rtvSlot, bindDSV, clearTarget)

#define endRenderPass() \
	gCurrentBindedRenderPass = nullptr; \
}while(false)

#define startPipeline(pipelineName) \
do{ \
	RENDERING_ALREADY_BIND(gCurrentBindedPipeline, pipelineName); \
	static Pipeline* findPipeline = gCurrentBindedRenderPass->getPipeline(pipelineName); \
	gCurrentBindedPipeline = findPipeline; \
	RENDERING_VERIFY(gCurrentBindedPipeline, pipelineName); \
	currentRenderModule.bindPipeline(*gCurrentBindedPipeline, gCurrentBindedPipeline->_type)

#define endPipeline() \
	gCurrentBindedPipeline = nullptr; \
}while(false)

#define setRootConstantParameter(name, value) \
{ \
	static_assert(sizeof(value) == 4, "Root constant value must be 4 bytes."); \
	static RootConstant32BitParameterBindingInfo* bindingInfo = gCurrentBindedPipeline->getRootConstantParameter(name); \
	RENDERING_VERIFY(bindingInfo, name); \
	DK::memcpy((static_cast<uint8*>(bindingInfo->_buffer) + bindingInfo->_offset), &value, 4); \
	currentRenderModule.setRoot32BitConstants(bindingInfo->_rootParameterIndex, 1, &value, bindingInfo->_offset, gCurrentBindedPipeline->_type); \
}

#define setConstantBuffer(name, buffer) \
{ \
	static const ShaderParameter* shaderParameter = gCurrentBindedPipeline->getShaderParameter(name); \
	RENDERING_VERIFY(shaderParameter, name); \
	currentRenderModule.bindConstantBuffer(shaderParameter->_rootParameterIndex, buffer, gCurrentBindedPipeline->_type); \
}
#define setShaderResourceView(name, buffer) \
{ \
	static const ShaderParameter* shaderParameter = gCurrentBindedPipeline->getShaderParameter(name); \
	RENDERING_VERIFY(shaderParameter, name); \
	currentRenderModule.bindShaderResourceView(shaderParameter->_rootParameterIndex, buffer, gCurrentBindedPipeline->_type); \
}

	class RaytracingRenderer;

	struct IBuffer : public std::enable_shared_from_this<IBuffer>
	{
		friend class RenderModule;

		enum class Type
		{
			BACKBUFFER, DEFAULT, UPLOAD, TEXTURE, COUNT
		};

	public:
		IBuffer()
		{}
		IBuffer(IBuffer&& rhs)
		{
			this->operator=(DK::move(rhs));
		}
		IBuffer(const Type type, RenderResourcePtr<ID3D12Resource>&& buffer, const uint32 bufferSize, const D3D12_RESOURCE_STATES state)
			: _type(type)
			, _bufferSize(bufferSize)
			, _currentState(state)
		{
			_buffer = DK::move(buffer);
		}
		IBuffer(RenderResourcePtr<ID3D12Resource>&& buffer, const uint32 width, const uint32 height, const uint32 bitPerPixel, const uint32 mipLevelCount, const D3D12_RESOURCE_STATES state)
			: _type(Type::TEXTURE)
			, _bufferSize(width* height* bitPerPixel)
			, _currentState(state)
		{
			_buffer = DK::move(buffer);
		}

		IBuffer& operator=(IBuffer&& rhs)
		{
			_type = rhs._type;
			_buffer = DK::move(rhs._buffer);
			_bufferSize = rhs._bufferSize;
			_currentState = rhs._currentState;

			return *this;
		}

		ID3D12Resource* operator->()
		{
			DK_ASSERT_LOG(_type != Type::COUNT && isValid(), "");
			return _buffer.get();
		}
		const ID3D12Resource* operator->() const
		{
			DK_ASSERT_LOG(_type != Type::COUNT && isValid(), "");
			return _buffer.get();
		}

		const bool isValid() const
		{
			DK_ASSERT_LOG(_type != Type::COUNT, "");
			return _buffer.get() != nullptr;
		}

		void upload(const void* data);

	private:
#if defined(_DK_DEBUG_)
		Type _type = Type::COUNT;
#endif

		/*const*/ RenderResourcePtr<ID3D12Resource> _buffer = nullptr;
		/*const*/ uint32 _bufferSize = 0;

		D3D12_RESOURCE_STATES _currentState;
	};
	using IBufferRef = std::shared_ptr<IBuffer>;

	struct DKCommandList
	{
		friend class RenderModule;
		friend void IBuffer::upload(const void* data);

		dk_inline DKCommandList()
		{
		}
		dk_inline DKCommandList(RenderResourcePtr<ID3D12CommandAllocator>(&& commandAllocators)[2], RenderResourcePtr<ID3D12GraphicsCommandList4>&& commandList)
			: _commandList(DK::move(commandList))
		{
			for (uint32 i = 0; i < 2; ++i)
				_commandAllocators[i] = DK::move(commandAllocators[i]);
		}

	private:
		ID3D12GraphicsCommandList4* operator->()
		{
			return _commandList.get();
		}
		const ID3D12GraphicsCommandList4* operator->() const
		{
			return _commandList.get();
		}

		operator ID3D12GraphicsCommandList4* ()
		{
			return _commandList.get();
		}

	private:
		RenderResourcePtr<ID3D12CommandAllocator> _commandAllocators[2];
		RenderResourcePtr<ID3D12GraphicsCommandList4> _commandList;
		uint32 _lastResetIndex = 0;

		struct UploadResourcePendingData
		{
			IBufferRef _target = nullptr;
			DKVector<uint8> _data;
			void* _dataPtr = nullptr;

			const void* getData() const { return _dataPtr == nullptr ? _data.data() : _dataPtr; }
		};
		DKVector<UploadResourcePendingData> _uploadResourcePendingArray;
		DKVector<UploadResourcePendingData> _prevUploadResourcePendingArray;

		struct CopyResourcePendingData
		{
			static constexpr const uint32 INVALID_AFTER_STATE = 0xFFFFFFFF;

			IBufferRef _source;
			IBufferRef _target;
			D3D12_RESOURCE_STATES _afterState = static_cast<D3D12_RESOURCE_STATES>(INVALID_AFTER_STATE);
		};
		DKVector<CopyResourcePendingData> _copyResourcePendingArray;
		DKVector<CopyResourcePendingData> _prevCopyResourcePendingArray;
	};

	class RenderModule
	{
		friend void IBuffer::upload(const void* data);

	public:
		static constexpr uint32 kFrameCount = 2;
		static uint32 kCurrentFrameIndex;
		static uint32 kCurrentBackBufferIndex;
		static uint32 kWidth;
		static uint32 kHeight;

	public:
		~RenderModule();

		bool initialize(const HWND hwnd, const uint32 width, const uint32 height);

		void destroy();

		bool createRenderPass(ShaderCompiler& shaderCompiler, const DKString& renderPassName, RenderPass::CreateInfo&& renderPassCreateInfo);
#if defined(_DK_DEBUG_)
		const bool reloadShader();
		void ReportLiveObjects() const;
#endif

		// SceneRenderer 전용 함수
		void preRender();
		void bindRenderPass(const uint32 rtvReadSlot, const uint32 rtvSlot, const bool bindDSV, const bool clearTarget);
		bool bindPipeline(Pipeline& pipeline, const Pipeline::Type type);
		void setRoot32BitConstants(const uint32 rootParameterIndex, const uint32 count, const void* data, uint32 offset, const Pipeline::Type type);
		void bindConstantBuffer(const uint32 rootParameterIndex, const IBufferRef& buffer, const Pipeline::Type type);
		void bindShaderResourceView(const uint32 rootParameterIndex, const IBufferRef& buffer, const Pipeline::Type type);
		void setVertexBuffers(const uint32 startSlot, const uint32 numViews, const D3D12_VERTEX_BUFFER_VIEW* view);
		void setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view);
		void drawIndexedInstanced(const uint32 indexCountPerInstance, const uint32 instanceCount, const uint32 startIndexLocation, const int baseVertexLocation, const uint32 startInstanceLocation);
		void dispatch(const uint32 threadGroupCountX, const uint32 threadGroupCountY, const uint32 threadGroupCountZ);
		void dispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc);
		void endRender();

		// helper 함수
		IBufferRef createConstantBuffer(const uint32 size, const wchar_t* debugName);
		IBufferRef createUploadBuffer(const uint32 size, const wchar_t* debugName);
		IBufferRef createVertexBuffer(const void* data, const uint32 strideSize, const uint32 vertexCount, VertexBufferViewRef& outView, const wchar_t* debugName);
		IBufferRef createIndexBuffer(const uint32* data, const uint32 indexCount, IndexBufferViewRef& outView, const wchar_t* debugName);
		ITextureRef createTexture(const DKString& path, const uint32 width, const uint32 height, const byte* data, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES state, const bool createSRV, const bool createUAV);
		ITextureRef createTexture(const DKString& path, const uint32 width, const uint32 height, const D3D12_SUBRESOURCE_DATA* initialData, const uint8 mipLevelCount, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES state, const bool createSRV, const bool createUAV);
		ITextureRef loadAndCreateTexture(const DKString& path);
		void deleteTexture(ITexture* texture);
		void deallocateTextureSRV(const TextureResourceViewType index);
		void deallocateTextureUAV(const TextureResourceViewType index);
		void copyResource(const IBufferRef& targetBuffer, const IBufferRef& sourceBuffer, const D3D12_RESOURCE_STATES afterState);

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

		void waitAllGPU();

	private:
		bool initialize_createDeviceAndCommandQueueAndSwapChain(const HWND hwnd, const uint32 width, const uint32 height);
		const bool createCommandList(DKCommandList& outCommandList);
		bool initialize_createFence();
		bool createRootSignature(const Pipeline::CreateInfo& createInfo, const DKVector<ShaderResourceReflection>& resources, Pipeline& inoutPipeline);
		bool createPipelineObjectState(ShaderCompiler& shaderCompiler, const Pipeline::CreateInfo& pipelineCreateInfo, Pipeline& inoutPipeline);

		const bool allocateTextureSRV(ITexture* texture);
		const bool allocateTextureUAV(ITexture* texture);

		IBufferRef createBuffer2DInternal(const uint32 width, const uint32 height, const uint32 mipLevelCount, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue, const wchar_t* debugName);
		IBufferRef createDefaultBuffer(const void* data, const uint32 bufferSize, const D3D12_RESOURCE_STATES state, const wchar_t* debugName);

		void execute(const bool present = false);

		void resourceBarrierTransition(IBufferRef& buffer, const D3D12_RESOURCE_STATES afterState);
		void resourceBarrierTransition(IBuffer& buffer, const D3D12_RESOURCE_STATES afterState);

		void waitFenceAndResetCommandList();

#if defined(_DK_DEBUG_)
	public:
		bool _isDestroyed = false;
		RenderResourcePtr<ID3D12Debug> _debugController;
#endif

	private:
		bool _useWarpDevice = false;
		RenderResourcePtr<ID3D12Device8> _device = nullptr;
		RenderResourcePtr<ID3D12CommandQueue> _commandQueue;
		DKCommandList _commandList;
		uint32 _fenceValues[kFrameCount] = { 0, };
		RenderResourcePtr<ID3D12Fence> _fences[kFrameCount];
		HANDLE _fenceEvent;
		Ptr<D3D12_VIEWPORT> _viewport;
		Ptr<D3D12_RECT> _scissorRect;
		RenderResourcePtr<IDXGISwapChain4> _swapChain = nullptr;

		uint32 _renderTargetViewSize = 0;
		uint32 _depthStencilViewSize = 0;
		uint32 _bindlessViewSize = 0;

		// RenderTarget + BackBuffer
		RenderResourcePtr<ID3D12DescriptorHeap> _renderTargetViewHeap = nullptr;
		// RenderTarget
		static constexpr const uint32 kRenderTargetTextureCount = 4;					// Deffered: 0, 2 / Gbuffer: 1, 3
		ITextureRef _renderTargetTextureArr[kRenderTargetTextureCount];
		// BackBuffer
		IBuffer _backBufferResourceArr[kFrameCount];			// BackBuffer

		// DepthStencil
		RenderResourcePtr<ID3D12DescriptorHeap> _depthStencilDescriptorHeap = nullptr;	// Deffered: 0, 2 / Gbuffer: 1, 3
		ITextureRef _depthStencilTextureArr[kRenderTargetTextureCount];

		// SwapChain
#if defined(USE_IMGUI)
		RenderResourcePtr<ID3D12DescriptorHeap> _pd3dSrvDescHeap;
#endif

		// Texture
		static constexpr const uint32 kMaxTextureSRVCount = 1024;
		static constexpr const uint32 kMaxTextureUAVCount = 1024;
		uint32 _currentTextureSRV = 0;
		uint32 _currentTextureUAV = 0;
		DKVector<TextureResourceViewType> _deletedTextureSRVArr;
		DKVector<TextureResourceViewType> _deletedTextureUAVArr;
		DKHashMap<DKString, ITexture*> _textureContainer;
		RenderResourcePtr<ID3D12DescriptorHeap> _textureDescriptorHeap;

		DKHashMap<DKString, RenderPass> _renderPassMap;

#if defined(_DK_DEBUG_)
	public:
		bool _blockUpload = false;
		bool _blockCopy = false;
#endif
	};

	class ITexture : public std::enable_shared_from_this<ITexture>
	{
		friend class RenderModule;

	public:
		static constexpr TextureResourceViewType kErrorTextureResourceViewIndex = 0xffffffff;

	private:
		ITexture(const DKString& path, const uint8 mipLevelCount, const DXGI_FORMAT format, IBuffer&& textureBuffer)
			: _path(path)
			, _mipLevelCount(mipLevelCount)
			, _format(format)
			, _textureBuffer(DK::move(textureBuffer))
		{}
		ITexture(const DKString& path, const uint8 mipLevelCount, const DXGI_FORMAT format, IBufferRef&& textureBuffer)
			: _path(path)
			, _mipLevelCount(mipLevelCount)
			, _format(format)
			, _textureBuffer(DK::move(*textureBuffer.get()))
		{
			textureBuffer.reset();
		}

	public:
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
		dk_inline const IBuffer& getTextureBuffer() const
		{
			return _textureBuffer;
		}
		dk_inline IBuffer& getTextureBuffer()
		{
			return _textureBuffer;
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

	public:
#if defined(_DK_DEBUG_)
		bool _inContainer = false;
#endif

	private:
		const DKString _path = "";
		const uint8 _mipLevelCount = 1;
		const DXGI_FORMAT _format = DXGI_FORMAT_FORCE_UINT;
		/*const*/ IBuffer _textureBuffer;

		TextureResourceViewType _textureSRVIndex = kErrorTextureResourceViewIndex;
		TextureResourceViewType _textureUAVIndex = kErrorTextureResourceViewIndex;
	};

	//static_assert(DK_COUNT_OF(DKCommandLIst::_commandAllocators) == RenderModule::kFrameCount, "");
	static_assert(2 == RenderModule::kFrameCount, "");
}
