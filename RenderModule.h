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
	enum class MaterialParameterType : uint8;
	struct MaterialParameterDefinition;
	class Material;
	struct MaterialDefinition;
	class TextureRaw;

	struct DKCommandList;
	struct IBuffer;

	static constexpr bool gAssertRenderPassPipelineExist = false;
}

namespace DK
{
	enum class ShaderVariableType
	{
		Buffer,
		StructuredBuffer,
		Count
	};

	struct ShaderVariable
	{
		DKString _name;
		ShaderVariableType _type;
		uint32 _register;
		uint32 _rootParameterIndex;		// createRenderPass 시점에 설정 (나머지는 Resource로부터)
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
			const char* _vertexShaderPath;
			const char* _vertexShaderEntry;
			const char* _pixelShaderPath;
			const char* _pixelShaderEntry;

			DKVector<LayoutInfo> _layout;
			DKVector<ShaderVariable> _variableArr;
		};

		RenderResourcePtr<ID3D12RootSignature> _rootSignature;
		RenderResourcePtr<ID3D12PipelineState> _pipelineStateObject;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE _primitiveTopologyType;
		DKVector<ShaderVariable> _shaderVariables;

		dk_inline const ShaderVariable* getShaderVariable(const DKString& name) const
		{
			for (uint i = 0; i < _shaderVariables.size(); ++i)
			{
				if (name == _shaderVariables[i]._name)
				{
					return &_shaderVariables[i];
				}
			}

			DK_ASSERT_LOG(false, "존재하지 않는 ShaderVariable입니다.");
			return nullptr;
		}
	};

	struct RenderPass
	{
		struct CreateInfo
		{
			DKVector<DKPair<DKString, Pipeline::CreateInfo>> _pipelines;
		};

		DKHashMap<DKString, Pipeline> _pipelines;

		dk_inline Pipeline* getPipeline(const DKString& pipelineName)
		{
			DKHashMap<DKString, Pipeline>::iterator iter = _pipelines.find(pipelineName);
			if (iter == _pipelines.end())
			{
				DK_ASSERT_LOG(!gAssertRenderPassPipelineExist, "존재하지 않는 Pipeline을 찾습니다.\nPipelineName: %s", pipelineName.c_str());
				return nullptr;
			}

			return &iter->second;
		}
	};

#define startRenderPass(renderModule, renderPassName) \
do{ \
	RenderModule& currentRenderModule = renderModule; \
	RenderPass* currentRenderPass = currentRenderModule.getRenderPass(renderPassName); \
	if(currentRenderPass == nullptr) \
		break

#define endRenderPass() }while(false)

#define startPipeline(pipelineName) \
do{ \
	Pipeline* currentPipeline = currentRenderPass->getPipeline(pipelineName); \
	if(currentPipeline == nullptr) break; \
	currentRenderModule.bindPipeline(*currentPipeline);

#define endPipeline() }while(false)

#define setConstantBuffer(name, address) \
{ \
	static const ShaderVariable* shaderVariable = currentPipeline->getShaderVariable(name); \
	if(shaderVariable == nullptr) break; \
	currentRenderModule.bindConstantBuffer(shaderVariable->_rootParameterIndex, address); \
}
#define setShaderResourceView(name, address) \
{ \
	static const ShaderVariable* shaderVariable = currentPipeline->getShaderVariable(name); \
	if(shaderVariable == nullptr) break; \
	currentRenderModule.bindShaderResourceView(shaderVariable->_rootParameterIndex, address); \
}

	class RenderModule
	{
	public:
		constexpr static uint kFrameCount = 3;
		static uint kCurrentFrameIndex;

	public:
		~RenderModule();

		bool initialize(const HWND hwnd, const uint32 width, const uint32 height);

		bool createRenderPass(const DKString& renderPassName, RenderPass::CreateInfo&& renderPassCreateInfo);
		// #todo- Container 이용해도될듯?
		IBuffer* createUploadBuffer(const uint32 size);
		const bool createVertexBuffer(const void* data, const uint32 strideSize, const uint32 vertexCount, VertexBufferViewRef& outView);
		const bool createIndexBuffer(const void* data, const uint32 bufferSize, IndexBufferViewRef& outView);

		// Texture Manager 전용함수! 사용시 주의할것!
		const bool createTextureBindlessDescriptorHeap(RenderResourcePtr<ID3D12DescriptorHeap>& outDescriptorHeap);
		bool createTexture(const TextureRaw& textureRaw, D3D12_CPU_DESCRIPTOR_HANDLE& handleStart, const uint32 index);

		// SceneRenderer 전용 함수
		void preRender();			// RenderTaget 등을 설정하는데.. 이건 RenderPass Set으로 옮겨야할듯. 지금은 RenderTarget이 하나니 하나로 빼둠
		bool bindPipeline(Pipeline& pipeline);
		void bindConstantBuffer(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress);
		void bindShaderResourceView(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress);
		void setVertexBuffers(const uint32 startSlot, const uint32 numViews, const D3D12_VERTEX_BUFFER_VIEW* view);
		void setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view);
		void drawIndexedInstanced(const uint32 indexCountPerInstance, const uint32 instanceCount, const uint32 startIndexLocation, const int baseVertexLocation, const uint32 startInstanceLocation);
		void endRender();

		// helper 함수
		dk_inline RenderPass* getRenderPass(const DKString& renderPassName)
		{
			using FindResult = DKHashMap<DKString, RenderPass>::iterator;
			FindResult find = _renderPassMap.find(renderPassName);

			if (find == _renderPassMap.end())
			{
				DK_ASSERT_LOG(!gAssertRenderPassPipelineExist, "존재하지 않는 RenderPass를 Bind시도합니다.\nName: %s", renderPassName.c_str());
				return nullptr;
			}

			return &find->second;
		}

	private:
		bool initialize_createDeviceAndCommandQueueAndSwapChain(const HWND hwnd, const uint width, const uint height);
		DKCommandList* createCommandList();
		bool initialize_createFence();
		bool initialize_createFenceEvent();
		bool createRootSignature(Pipeline& inoutPipeline);
		bool createPipelineObjectState(const Pipeline::CreateInfo& pipelineCreateInfo, Pipeline& inoutPipeline);

		ID3D12Resource* createBufferInternal(const uint32 size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state);
		ID3D12Resource* createDefaultBuffer(const uint size, const D3D12_RESOURCE_STATES state);
		ID3D12Resource* createInitializedDefaultBuffer(const void* data, const uint bufferSize);

		void waitFenceAndResetCommandList();
		void execute();

	private:
		bool _useWarpDevice = false;
		RenderResourcePtr<ID3D12Device8> _device = nullptr;
		RenderResourcePtr<ID3D12CommandQueue> _commandQueue;
		RenderResourcePtr<IDXGISwapChain4> _swapChain = nullptr;
		RenderResourcePtr<ID3D12Resource> _renderTargetResources[kFrameCount];
		RenderResourcePtr<ID3D12DescriptorHeap> _renderTargetDescriptorHeap = nullptr;
		RenderResourcePtr<ID3D12DescriptorHeap> _depthStencilDescriptorHeap = nullptr;
		Ptr<D3D12_VIEWPORT> _viewport;
		Ptr<D3D12_RECT> _scissorRect;

#if defined(USE_IMGUI)
		RenderResourcePtr<ID3D12DescriptorHeap> _pd3dSrvDescHeap;
#endif

		Ptr<DKCommandList> _commandList = nullptr;
		uint _fenceValues[kFrameCount] = { 0, };
		RenderResourcePtr<ID3D12Fence> _fences[kFrameCount];
		HANDLE _fenceEvent;

		DKHashMap<DKString, RenderPass> _renderPassMap;
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
}