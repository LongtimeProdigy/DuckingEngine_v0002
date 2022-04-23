#include "stdafx.h"
#include "RenderModuleDX12.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "d3dx12.h"

#include <wrl.h>
#pragma endregion

#include "IDevice.h"
#include "ICommandQueue.h"
#include "ISwapChain.h"
#include "IDescriptorHeap.h"
#include "ICommandAllocator.h"
#include "ICommandList.h"
#include "IFence.h"
#include "IResource.h"
#include "IPipelineStateObject.h"
#include "IRootSignature.h"
#include "IShader.h"
#include "ITexture.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
static IDescriptorHeap g_pd3dSrvDescHeap = NULL;
#endif

#include "DuckingEngine.h"
#include "ResourceManager.h"

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"
#include "Material.h"
#include "BufferView.h"

// #todo- 나중에 IViewport, IRect만들어서 분리할 것..
static D3D12_VIEWPORT _viewport;
static D3D12_RECT _scissorRect;

static DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
{
	*ppAdapter = nullptr;
	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		IDXGIAdapter1* pAdapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
		{
			// No more adapters to enumerate.
			break;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			*ppAdapter = pAdapter;
			return;
		}
		pAdapter->Release();
	}
}

bool CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	using Microsoft::WRL::ComPtr;
	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}

RenderModuleDX12::RenderModuleDX12()
{

}
RenderModuleDX12::~RenderModuleDX12()
{
	dk_delete _device;
	dk_delete _commandQueue;
	dk_delete _swapChain;

	for (uint i = 0; i < _commandAllocators.size(); ++i)
	{
		dk_delete _commandAllocators[i];
	}
	_commandAllocators.clear();

	dk_delete _commandList;

	for (uint i = 0; i < _fences.size(); ++i)
	{
		dk_delete _fences[i];
	}
	_fences.clear();
	_fenceValues.clear();
	CloseHandle(_fenceEvent);

	for (uint i = 0; i < _renderTargetResources.size(); ++i)
	{
		dk_delete _renderTargetResources[i];
	}
	_renderTargetResources.clear();
	dk_delete _renderTargetDescriptorHeap;

	dk_delete _depthStencilDescriptorHeap;
}

bool RenderModuleDX12::Initialize(HWND hwnd, uint width, uint height)
{
	HRESULT hr;

#pragma region Create Factory
	IDXGIFactory4* factory;
	UINT factoryFlags = 0;
#if defined(DK_DEBUG)
	factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory));
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));
#pragma endregion

#pragma region Create Device
	ID3D12Device8* device;
	if (_useWarpDevice == true)
	{
		IDXGIAdapter* warpAdapter;
		CHECK_BOOL_AND_RETURN(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))));
		CHECK_BOOL_AND_RETURN(SUCCEEDED(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))));
	}
	else
	{
		IDXGIAdapter1* hardwareAdapter;
		GetHardwareAdapter(factory, &hardwareAdapter);

		CHECK_BOOL_AND_RETURN(SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))));
	}

	_device = dk_new IDevice(device);
#pragma endregion

#pragma region Create CommandQueue
	ID3D12CommandQueue* commandQueue;
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CHECK_BOOL_AND_RETURN(SUCCEEDED(device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue))));

	_commandQueue = dk_new ICommandQueue(commandQueue);
#pragma endregion

#pragma region Create SwapChain
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = kFrameCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	IDXGISwapChain4* swapChain = nullptr;
	IDXGISwapChain1* swapChain1 = static_cast<IDXGISwapChain1*>(swapChain);
	hr = factory->CreateSwapChainForHwnd(
		commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1
	);
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	swapChain = static_cast<IDXGISwapChain4*>(swapChain1);

	_swapChain = dk_new ISwapChain(swapChain);
	_currentFrame = swapChain->GetCurrentBackBufferIndex();
#pragma endregion

	// Release tempObject
	{
		factory->Release();
	}

#pragma region Create CommandAllocator
	std::vector<ID3D12CommandAllocator*> commandAllocator;
	commandAllocator.resize(kFrameCount);
	_commandAllocators.resize(kFrameCount);
	for (uint i = 0; i < kFrameCount; ++i)
	{
		hr = device->CreateCommandAllocator(cqDesc.Type, IID_PPV_ARGS(&commandAllocator[i]));
		CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

		_commandAllocators[i] = dk_new ICommandAllocator(commandAllocator[i]);
	}
#pragma endregion

#pragma region Create CommandList
	ID3D12GraphicsCommandList* commandList;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[_currentFrame], NULL, IID_PPV_ARGS(&commandList));
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));
	commandList->Close();

	_commandList = dk_new ICommandList(commandList);
#pragma endregion

#pragma region Create Fence
	std::vector<ID3D12Fence*> fences;
	fences.resize(kFrameCount);
	_fences.resize(kFrameCount);
	_fenceValues.resize(kFrameCount);
	for (uint i = 0; i < kFrameCount; ++i)
	{
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fences[i]));
		CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

		_fences[i] = dk_new IFence(fences[i]);

		_fenceValues[i] = 0;
	}
	// create Fence Event
	_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == _fenceEvent) return false;
#pragma endregion

#pragma region Create DescriptorHeap, BackBuffers(RTV)
	ID3D12DescriptorHeap* rtvDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = kFrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	std::vector<ID3D12Resource*> rtvResource;
	rtvResource.resize(kFrameCount);
	_renderTargetResources.resize(kFrameCount);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (uint i = 0; i < kFrameCount; ++i)
	{
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvResource[i]));
		CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

		_renderTargetResources[i] = dk_new IResource(rtvResource[i]);

		device->CreateRenderTargetView(rtvResource[i], nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}

	_renderTargetDescriptorHeap = dk_new IDescriptorHeap(rtvDescriptorHeap);
#pragma endregion

#pragma region Create Depth/Stencil View
	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = dsvFormat;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES dsHeapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC dsDesc = CD3DX12_RESOURCE_DESC::Tex2D(dsvFormat, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ID3D12Resource2* depthStencilBuffer;
	hr = device->CreateCommittedResource(
		&dsHeapProperty,
		D3D12_HEAP_FLAG_NONE,
		&dsDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer)
	);
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));
	depthStencilBuffer->SetName(L"Depth/Stencil Resource Heap");

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ID3D12DescriptorHeap* dsvDescriptorHeap;
	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
	CHECK_BOOL_AND_RETURN(SUCCEEDED(hr));

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = dsvFormat;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
	device->CreateDepthStencilView(
		depthStencilBuffer, &depthStencilViewDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
	);

	_depthStencilDescriptorHeap = dk_new IDescriptorHeap(dsvDescriptorHeap);
#pragma endregion

	_viewport.TopLeftX = 0;
	_viewport.TopLeftY = 0;
	_viewport.Width = static_cast<FLOAT>(width);
	_viewport.Height = static_cast<FLOAT>(height);
	_viewport.MinDepth = 0.0f;
	_viewport.MaxDepth = 1.0f;

	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = static_cast<LONG>(width);
	_scissorRect.bottom = static_cast<LONG>(height);

#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	{
		ID3D12DescriptorHeap* descriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)) != S_OK)
			return false;

		g_pd3dSrvDescHeap.SetDescriptorHeap(descriptorHeap);

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(device, kFrameCount,
			DXGI_FORMAT_R8G8B8A8_UNORM, descriptorHeap,
			descriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			descriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
#endif // USE_IMGUI

	return true;
}

bool RenderModuleDX12::WaitForPreviousFrame() const
{
	_currentFrame = _swapChain->GetCurrentBackBufferIndex();

	uint temp = static_cast<uint>(_fences[_currentFrame]->GetCompletedValue());
	if (temp < _fenceValues[_currentFrame])
	{
		bool success = _fences[_currentFrame]->SetEventOnCompletion(_fenceValues[_currentFrame], _fenceEvent);
		if (success == false)
		{
			_isRunning = false;
		}

		WaitForSingleObject(_fenceEvent, INFINITE);
	}

	++_fenceValues[_currentFrame];
	
	bool success = _commandAllocators[_currentFrame]->Reset();
	CHECK_BOOL_AND_RETURN(success);

	success = _commandList->Reset(_commandAllocators[_currentFrame], nullptr);
	CHECK_BOOL_AND_RETURN(success);

	return true;
}

bool RenderModuleDX12::ExcuteCommandList() const
{
	bool success = _commandList->Close();
	DK_ASSERT_LOG(success, "계속 왜 실패하는거야!");
	CHECK_BOOL_AND_RETURN(success);

	std::vector<ICommandList*> commandLists;
	commandLists.resize(1);
	commandLists[0] = _commandList;
	_commandQueue->ExecuteCommandLists(static_cast<uint>(commandLists.size()), commandLists);

	success = _commandQueue->Signal(_fences[_currentFrame], _fenceValues[_currentFrame]);
	CHECK_BOOL_AND_RETURN(success);

	return true;
}

const bool RenderModuleDX12::LoadRootSignature(const MaterialType materialType, IRootSignatureRef& outRootSignature)
{
	if (materialType == MaterialType::COUNT)
	{
		DK_ASSERT_LOG(false, "MaterialType이 올바르지 않습니다.");
		return false;
	}

	std::unordered_map<const MaterialType, IRootSignatureRef>::iterator found = _rootSignatureContainer.find(materialType);
	if (found != _rootSignatureContainer.end())
	{
		outRootSignature = found->second;
		return true;
	}

	ID3DBlob* signature = nullptr;
	switch (materialType)
	{
	case MaterialType::SKINNEDMESH: 
	{
		//D3D12_ROOT_CONSTANTS root32BitConstant = {};
		//root32BitConstant.RegisterSpace = 0;
		//root32BitConstant.ShaderRegister = 0;
		//root32BitConstant.Num32BitValues = 1;

		D3D12_ROOT_DESCRIPTOR rootCameraConstantBufferDescriptor = {};
		rootCameraConstantBufferDescriptor.RegisterSpace = 0;
		rootCameraConstantBufferDescriptor.ShaderRegister = 0;

		D3D12_ROOT_DESCRIPTOR rootSceneObjectConstantBufferDescriptor = {};
		rootSceneObjectConstantBufferDescriptor.RegisterSpace = 0;
		rootSceneObjectConstantBufferDescriptor.ShaderRegister = 1;

		D3D12_ROOT_DESCRIPTOR skeletonConstantBufferDescriptor = {};
		skeletonConstantBufferDescriptor.RegisterSpace = 0;
		skeletonConstantBufferDescriptor.ShaderRegister = 2;

		D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1];
		descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorTableRanges[0].NumDescriptors = 1;
		descriptorTableRanges[0].BaseShaderRegister = 0;
		descriptorTableRanges[0].RegisterSpace = 0;
		descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
		descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
		descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

		D3D12_ROOT_PARAMETER rootParameters[4];
		//{
		//	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		//	rootParameters[0].Constants = root32BitConstant;
		//	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		//}
		{	// For CameraConstantBuffer
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[0].Descriptor = rootCameraConstantBufferDescriptor;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		}
		{	// For SceneObjectConstantBuffer
			rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[1].Descriptor = rootSceneObjectConstantBufferDescriptor;
			rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		}
		{	// For SkeletonConstantBuffer
			rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[2].Descriptor = skeletonConstantBufferDescriptor;
			rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		}
		{	// For TextureTable
			rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[3].DescriptorTable = descriptorTable;
			rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &sampler;
		rootSignatureDesc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		ID3DBlob* errorBuffer;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuffer);
		if (FAILED(hr) == true)
		{
			OutputDebugStringA(static_cast<char*>(errorBuffer->GetBufferPointer()));
			return false;
		}
	}
		break;
	default:
		break;
	}

	ID3D12RootSignature* rootSignature = nullptr;
	bool success = _device->CreateRootSignature(
		0, 
		signature->GetBufferPointer(), 
		static_cast<uint>(signature->GetBufferSize()), 
		rootSignature
	);
	CHECK_BOOL_AND_RETURN(success);

	typedef std::pair<std::unordered_map<const MaterialType, IRootSignatureRef>::iterator, bool> InsertResult;
	InsertResult insertSuccess = _rootSignatureContainer.insert(
		std::pair<const MaterialType, IRootSignatureRef>(materialType, std::make_shared<IRootSignature>(rootSignature))
	);
	if (insertSuccess.second == false)
	{
		return false;
	}

	outRootSignature = insertSuccess.first->second;
	return true;
}

const bool RenderModuleDX12::LoadPipelineStateObject(const MaterialType materialType, const IRootSignature& rootSignature, IPipelineStateObjectRef& outPipelineStateObject)
{
	if (materialType == MaterialType::COUNT)
	{
		DK_ASSERT_LOG(false, "MaterialType이 올바르지 않습니다.");
		return false;
	}

	std::unordered_map<const MaterialType, IPipelineStateObjectRef>::iterator found = _pipelineStateObjectContainer.find(materialType);
	if (found != _pipelineStateObjectContainer.end())
	{
		outPipelineStateObject = found->second;
		return true;
	}

	// #todo- ModelProperty로부터 얻어온 Shader를 사용해야할 것입니다.
	static const char* vertexShaderFilePath = "Shader/VertexShader.hlsl";
	IShaderRef vertexShader;
	bool success = LoadVertexShader(vertexShaderFilePath, vertexShader);
	CHECK_BOOL_AND_RETURN(success);
	static const char* pixelShaderFilePath = "Shader/PixelShader.hlsl";
	IShaderRef pixelShader;
	success = LoadPixelShader(pixelShaderFilePath, pixelShader);
	CHECK_BOOL_AND_RETURN(success);

	static D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
		{ "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	depthStencilDesc.FrontFace = defaultStencilOp;
	depthStencilDesc.BackFace = defaultStencilOp;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = rootSignature.GetRootSignatureWritable();
	psoDesc.VS = vertexShader->GetShaderByteCode();
	psoDesc.PS = pixelShader->GetShaderByteCode();
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = dsvFormat;
	
	ID3D12PipelineState* createdPipelineStateObject = nullptr;
	success = _device->CreateGraphicsPipelineState(psoDesc, createdPipelineStateObject);
	if (success == false)
	{
		_vertexShaderContainer.erase(vertexShaderFilePath);
		_pixelShaderContainer.erase(pixelShaderFilePath);
		return false;
	}

	typedef std::pair<std::unordered_map<const MaterialType, IPipelineStateObjectRef>::iterator, bool> InsertResult;
	InsertResult insertSuccess = _pipelineStateObjectContainer.insert(
		std::pair<const MaterialType, IPipelineStateObjectRef>(
			materialType, 
			std::make_shared<IPipelineStateObject>(createdPipelineStateObject)
			)
	);
	if (insertSuccess.second == false)
	{
		_vertexShaderContainer.erase(vertexShaderFilePath);
		_pixelShaderContainer.erase(pixelShaderFilePath);
		createdPipelineStateObject->Release();
		return false;
	}

	outPipelineStateObject = insertSuccess.first->second;
	return true;
}

const bool RenderModuleDX12::CreateDescriptorHeap(const MaterialType materialType, IDescriptorHeap& outDescriptorHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	switch (materialType)
	{
	case MaterialType::SKINNEDMESH:
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	default:
		break;
	}
	bool success = _device->CreateDescriptorHeap(heapDesc, outDescriptorHeap);
	CHECK_BOOL_AND_RETURN(success);

	return true;
}

Material* RenderModuleDX12::CreateMaterial(const MaterialType materialType, const SceneObject* sceneObject)
{
	if (materialType == MaterialType::COUNT)
	{
		DK_ASSERT_LOG(false, "올바르지 않은 MaterialType입니다.");
		return nullptr;
	}

	Material* returnMaterial = dk_new Material(materialType);
	returnMaterial->UpdateTechnique(sceneObject);

	return returnMaterial;
}

IResource* RenderModuleDX12::CreateUploadBuffer(void* data, uint size) const
{
	//bool success = WaitForPreviousFrame();
	//if (success == false)
	//{
	//	return nullptr;
	//}

	IResource* buffer = _device->CreateUploadResource(_commandList, data, size, 1);

	//success = ExcuteCommandList();
	//if (success == false)
	//{
	//	return nullptr;
	//}

	return buffer;
}

void RenderModuleDX12::UpdateUploadResource(IResource* resource, void* data, uint size) const noexcept
{
	//UpdateSubresources(_commandList->GetCommandListWritable(), defaultBuffer, uploadBuffer, 0, 0, 1, &resourceData);
}

IResource* RenderModuleDX12::CreateBufferInternal(const void* data, const uint size) const
{
	bool success = WaitForPreviousFrame();
	if (success == false)
	{
		return nullptr;
	}

	IResource* buffer = _device->CreateDefaultResource(_commandList, data, size, 1);
	if (buffer == nullptr)
	{
		return nullptr;
	}
	success = ExcuteCommandList();
	if (success == false)
	{
		return nullptr;
	}

	return buffer;
}
IResource* RenderModuleDX12::CreateBuffer(void* data, uint size) const
{
	return CreateBufferInternal(data, size);
}
const bool RenderModuleDX12::LoadBufferInternal(std::unordered_map<const void*, IResourceRef>& container, const void* data, const uint size, IResourceRef& outBuffer)
{
	std::unordered_map<const void*, IResourceRef>::iterator found = container.find(data);
	if (found != container.end())
	{
		outBuffer = found->second;
		return true;
	}

	IResource* buffer = CreateBufferInternal(data, size);
	if (buffer == nullptr)
	{
		return false;
	}

	typedef std::pair<std::unordered_map<const void*, IResourceRef>::iterator, bool> InsertResult;
	InsertResult insertSuccess = container.insert(std::pair<const void*, IResourceRef>(data, buffer));
	if (insertSuccess.second == false)
	{
		dk_delete buffer;
		return false;
	}
	outBuffer = insertSuccess.first->second;

	return true;
}
const bool RenderModuleDX12::LoadVertexBuffer(const void* data, const uint strideSize, const uint bufferSize, VertexBufferViewRef& outView)
{
	auto found = _vertexBufferViewContainer.find(data);
	if (found != _vertexBufferViewContainer.end())
	{
		outView = found->second;
		return true;
	}

	IResourceRef outrResource = nullptr;
	const bool success = LoadBufferInternal(_vertexBufferContainer, data, bufferSize, outrResource);
	if (success == false)
	{
		return false;
	}

	D3D12_VERTEX_BUFFER_VIEW d3dView;
	d3dView.BufferLocation = outrResource.get()->GetResourceWritable()->GetGPUVirtualAddress();
	d3dView.StrideInBytes = strideSize;
	d3dView.SizeInBytes = bufferSize;

	auto insertSuccess = _vertexBufferViewContainer.insert(std::pair<const void*, VertexBufferViewRef>(data, dk_new VertexBufferView(d3dView)));
	if (insertSuccess.second == false)
	{
		return false;
	}

	outView = insertSuccess.first->second;
	return true;
}
const bool RenderModuleDX12::LoadIndexBuffer(const void* data, const uint bufferSize, IndexBufferViewRef& outView)
{
	auto found = _indexBufferViewContainer.find(data);
	if (found != _indexBufferViewContainer.end())
	{
		outView = found->second;
		return true;
	}

	IResourceRef outrResource = nullptr;
	const bool success = LoadBufferInternal(_indexBufferContainer, data, bufferSize, outrResource);
	if (success == false)
	{
		return false;
	}

	D3D12_INDEX_BUFFER_VIEW d3dView;
	d3dView.BufferLocation = outrResource.get()->GetResourceWritable()->GetGPUVirtualAddress();
	d3dView.Format = DXGI_FORMAT_R32_UINT;
	d3dView.SizeInBytes = bufferSize;

	auto insertSuccess = _indexBufferViewContainer.insert(std::pair<const void*, IndexBufferViewRef>(data, dk_new IndexBufferView(d3dView)));
	if (insertSuccess.second == false)
	{
		return false;
	}
	outView = insertSuccess.first->second;

	return true;
}

// #todo- 실패가능성은?
ITexture RenderModuleDX12::CreateTexture(const char* texturePath, const D3D12_CPU_DESCRIPTOR_HANDLE& handle)
{
	ResourceManager* resourceManager = DuckingEngine::GetResourceManagerWritable();
	const TextureRawRef textureRaw = resourceManager->LoadTextureRaw(texturePath);

	WaitForPreviousFrame();

	IResource* textureBuffer = _device->CreateTextureResource(
		_commandList, textureRaw->_format, 
		textureRaw->_data, 
		textureRaw->_width, 
		textureRaw->_height, 
		textureRaw->_bitsPerPixel / 8
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureRaw->_format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	_device->CreateShaderResourceView(textureBuffer, &srvDesc, handle);

	ExcuteCommandList();

	return ITexture(texturePath, textureRaw, handle);
}

enum class ShaderType
{
	VERTEX, 
	PIXEL, 
	COUNT
};
wchar_t* CharToWChar2(const char* pstrSrc)
{
	int nLen = static_cast<int>(strlen(pstrSrc)) + 1;

	wchar_t* pwstr = (LPWSTR)malloc(sizeof(wchar_t) * nLen);

	size_t cn;
	mbstowcs_s(&cn, pwstr, nLen, pstrSrc, nLen);

	return pwstr;
}
const bool LoadShaderInternal(ShaderType type, std::unordered_map<const char*, IShaderRef>& container, const char* path, IShaderRef& outShader)
{
	std::unordered_map<const char*, IShaderRef>::const_iterator iter = container.find(path);
	if (iter != container.end())
	{
		outShader = iter->second;
		return true;
	}

	ID3DBlob* errorBuffer;
	ID3DBlob* shader;
	const wchar_t* wPath = CharToWChar2(path);
	HRESULT hr = D3DCompileFromFile(
		wPath, nullptr, nullptr, "main", 
		type == ShaderType::VERTEX ? "vs_5_0" : "ps_5_0", 
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 
		0, &shader, &errorBuffer
	);
	dk_delete_array(wPath);
	if (FAILED(hr) == true)
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE shaderByteCode;
	shaderByteCode.BytecodeLength = shader->GetBufferSize();
	shaderByteCode.pShaderBytecode = shader->GetBufferPointer();

	typedef std::pair<std::unordered_map<const char*, IShaderRef>::iterator, bool> InsertResult;
	InsertResult insertSuccess = container.insert(
		std::pair<const char*, IShaderRef>(path, std::make_shared<IShader>(shaderByteCode))
	);
	if (insertSuccess.second == false)
	{
		return false;
	}

	outShader = insertSuccess.first->second;
	return true;
}
const bool RenderModuleDX12::LoadVertexShader(const char* path, IShaderRef& outShader)
{
	return LoadShaderInternal(ShaderType::VERTEX, _vertexShaderContainer, path, outShader);
}
const bool RenderModuleDX12::LoadPixelShader(const char* path, IShaderRef& outShader)
{
	return LoadShaderInternal(ShaderType::PIXEL, _pixelShaderContainer, path, outShader);
}

void RenderModuleDX12::PreRender() const noexcept
{
#if defined(USE_IMGUI)
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	{
		static float f = 0.0f;
		static int counter = 0;
		static char buf[200] = {};

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

#define MAX_BUFFER_LENGTH 200
		ImGui::Text("MainCameraPosition");

		char cameraPosBuffer[MAX_BUFFER_LENGTH];
		const float3& cameraPosition = Camera::gMainCamera->GetWorldTransform().GetPosition();
		sprintf_s(cameraPosBuffer, MAX_BUFFER_LENGTH, "Position: x: %f, y: %f, z: %f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
		ImGui::Text(cameraPosBuffer);

		char cameraRotationBuffer[MAX_BUFFER_LENGTH];
		const float3 cameraRotation = Camera::gMainCamera->GetWorldTransform().GetRotation();
		sprintf_s(cameraRotationBuffer, MAX_BUFFER_LENGTH, "Rotation: x: %f, y: %f, z: %f", cameraRotation.x, cameraRotation.y, cameraRotation.z);
		ImGui::Text(cameraRotationBuffer);

		char cameraRotationMatrixBuffer[MAX_BUFFER_LENGTH];
		Matrix3x3 rotationMatrix;
		Camera::gMainCamera->GetWorldTransform().GetRoationMatrix(rotationMatrix);
		sprintf_s(cameraRotationMatrixBuffer, MAX_BUFFER_LENGTH, "%f, %f, %f\n%f, %f, %f\n%f, %f, %f",
			rotationMatrix._11, rotationMatrix._12, rotationMatrix._13,
			rotationMatrix._21, rotationMatrix._22, rotationMatrix._23,
			rotationMatrix._31, rotationMatrix._32, rotationMatrix._33
		);
		ImGui::Text(cameraRotationMatrixBuffer);

		char cameraForwardBuffer[MAX_BUFFER_LENGTH];
		float3 cameraForward = Camera::gMainCamera->GetWorldTransform().GetForward();
		sprintf_s(cameraForwardBuffer, MAX_BUFFER_LENGTH, "Forward: x: %f, y: %f, z: %f", cameraForward.x, cameraForward.y, cameraForward.z);
		ImGui::Text(cameraForwardBuffer);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	ImGui::Render();
#endif

	WaitForPreviousFrame();

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _renderTargetResources[_currentFrame]->GetResourceWritable();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &barrier);

	const UINT rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += rtvDescriptorSize * _currentFrame;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float clearColor[] = { 0.5f, 0.5f, 0.9f, 1.0f };
	_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	_commandList->RSSetViewports(1, &_viewport);
	_commandList->RSSetScissorRects(1, &_scissorRect);
	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

bool RenderModuleDX12::RenderSkinnedMeshComponent(const SkinnedMeshComponent* skinnedMeshComponent) const
{
	if (skinnedMeshComponent == nullptr)
	{
		return true;
	}

	const std::vector<SubMesh>& subMeshes = skinnedMeshComponent->GetModel()->GetSubMeshes();
	for (uint i = 0; i < subMeshes.size(); ++i)
	{
		const SubMesh& subMesh = subMeshes[i];

		_commandList->SetGraphicsRootSignature(subMesh._material->_rootSignature.get());
		_commandList->SetPipelineState(subMesh._material->_pipelineStateObject.get());

		for (uint i = 0; i < subMesh._material->_constant32BitParamters.size(); ++i)
		{
			_commandList->SetGraphicsRoot32BitConstant(
				subMesh._material->_constant32BitParamters[i]._rootParameterIndex, 
				subMesh._material->_constant32BitParamters[i]._constanceData, 
				0
			);
		}
		for (uint i = 0; i < subMesh._material->_constantBufferParamters.size(); ++i)
		{
			if (subMesh._material->_constantBufferParamters[i]._constantBuffer == nullptr)
			{
				continue;
			}

			_commandList->SetGraphicsRootConstantBufferView(
				subMesh._material->_constantBufferParamters[i]._rootParameterIndex,
				subMesh._material->_constantBufferParamters[i]._constantBuffer->GetResourceWritable()->GetGPUVirtualAddress()
			);
		}

		std::vector<ID3D12DescriptorHeap*> descriptorArr;
		descriptorArr.resize(subMesh._material->_descriptorHeapParameters.size());
		for (uint i = 0; i < subMesh._material->_descriptorHeapParameters.size(); ++i)
		{
			descriptorArr[i] = subMesh._material->_descriptorHeapParameters[i]._descriptor->GetDescriptorHeapWritable();
		}
		_commandList->SetDescriptorHeaps(descriptorArr);
		for (uint i = 0; i < subMesh._material->_descriptorHeapParameters.size(); ++i)
		{
			_commandList->SetGraphicsRootDescriptorTable(
				subMesh._material->_descriptorHeapParameters[i]._rootParameterIndex, 
				subMesh._material->_descriptorHeapParameters[i]._descriptor->GetGPUDescriptorHandleForHeapStart()
			);
		}

		_commandList->IASetVertexBuffers(0, 1, &subMesh._vertexBufferView.get()->GetViewWritable());
		_commandList->IASetIndexBuffer(&subMesh._indexBufferView.get()->GetViewWritable());
		uint maxValue = 0;
		for (uint i = 0; i < subMesh._indices.size(); ++i)
		{
			if (subMesh._indices[i] > maxValue)
			{
				maxValue = subMesh._indices[i];
			}
		}
		_commandList->DrawIndexedInstanced(static_cast<UINT>(subMesh._indices.size()), 1, 0, 0, 0);
	}

	return true;
}

void RenderModuleDX12::EndRender() const noexcept
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _renderTargetResources[_currentFrame]->GetResourceWritable();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &barrier);

	ExcuteCommandList();

	UINT syncInterval = 0; //gVSync ? 1 : 0;
	UINT presentFlags = 0; // CheckTearingSupport() && !gVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	bool success = _swapChain->Present(syncInterval, presentFlags);
}

void RenderModuleDX12::RenderUI() const noexcept
{
#ifdef USE_IMGUI
	std::vector<ID3D12DescriptorHeap*> descriptorHeaps;
	descriptorHeaps.push_back(g_pd3dSrvDescHeap.GetDescriptorHeapWritable());
	_commandList->SetDescriptorHeaps(descriptorHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList->GetCommandListWritable());
#endif // USE_IMGUI
}