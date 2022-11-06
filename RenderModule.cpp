#include "stdafx.h"
#include "RenderModule.h"

#pragma region Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
#include "d3dx12.h"
#include <wrl.h>
#pragma region DXC
#pragma comment(lib, "lib/dxc_2022_07_18/lib/x64/dxcompiler.lib")
#include "lib/dxc_2022_07_18/inc/dxcapi.h"
//#include "lib/dxc_2022_07_18/inc/d3d12shader.h"
//#include <atlbase.h>
#pragma endregion

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#endif


#include "DuckingEngine.h"
#include "ResourceManager.h"
#include "TextureManager.h"

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"

#include "Material.h"

uint RenderModule::kCurrentFrameIndex = 0;
DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

#define TEXTUREBINDLESS_MAX_COUNT 4096
#define TEXTUREBINDLESS_SPACE 10

template<typename T>
RenderResourcePtr<T>::~RenderResourcePtr()
{
	if(_ptr != nullptr)
		_ptr->Release();
}

RenderModule::~RenderModule()
{
	CloseHandle(_fenceEvent);
}

#pragma region Initialize
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
//bool CheckTearingSupport()
//{
//	BOOL allowTearing = FALSE;
//
//	using Microsoft::WRL::ComPtr;
//	// Rather than create the DXGI 1.5 factory interface directly, we create the
//	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
//	// graphics debugging tools which will not support the 1.5 factory interface 
//	// until a future update.
//	ComPtr<IDXGIFactory4> factory4;
//	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
//	{
//		ComPtr<IDXGIFactory5> factory5;
//		if (SUCCEEDED(factory4.As(&factory5)))
//		{
//			if (FAILED(factory5->CheckFeatureSupport(
//				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
//				&allowTearing, sizeof(allowTearing))))
//			{
//				allowTearing = FALSE;
//			}
//		}
//	}
//
//	return allowTearing == TRUE;
//}
bool RenderModule::initialize(const HWND hwnd, const uint width, const uint height)
{
	if (initialize_createDeviceAndCommandQueueAndSwapChain(hwnd, width, height) == false) return false;
	_commandList.assign(createCommandList());
	if (_commandList.get() == nullptr) return false;
	_commandList->_commandList->Close();
	if (initialize_createFence() == false) return false;
	if (initialize_createFenceEvent() == false) return false;

	return true;
}
bool CheckTearingSupport()
{
	Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));
	BOOL allowTearing = FALSE;
	if (SUCCEEDED(hr))
	{
		Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
		hr = factory4.As(&factory5);
		if (SUCCEEDED(hr))
		{
			hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		}
	}
	return SUCCEEDED(hr) && allowTearing;
}
bool RenderModule::initialize_createDeviceAndCommandQueueAndSwapChain(const HWND hwnd, const uint width, const uint height)
{
	HRESULT hr;

	// Create DirectX Factory
	IDXGIFactory4* factory;
	UINT factoryFlags = 0;
#if defined(_DK_DEBUG_)
	factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory));
	if (SUCCEEDED(hr) == false)
	{
		return false;
	}

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	if (SUCCEEDED(hr) == false)
	{
		return false;
	}

	static const GUID D3D12ExperimentalShaderModelsID = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
	0x76f5573e,
	0xf13a,
	0x40f5,
	{ 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
	};
	hr = D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr);
	if (FAILED(hr) == true)
	{
		return false;
	}

	// Create RenderDevice
	if (_useWarpDevice == true)
	{
		IDXGIAdapter* warpAdapter;
		if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))) == false)
		{
			return false;
		}
		if (SUCCEEDED(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(_device.getAddress()))) == false)
		{
			return false;
		}
	}
	else
	{
		IDXGIAdapter1* hardwareAdapter;
		GetHardwareAdapter(factory, &hardwareAdapter);
		if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(_device.getAddress()))) == false)
		{
			return false;
		}
	}

	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};

#if defined(NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB)
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
#elif defined(NTDDI_WIN10_19H1) && (NTDDI_VERSION >= NTDDI_WIN10_19H1)
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
#elif defined(NTDDI_WIN10_RS5) && (NTDDI_VERSION >= NTDDI_WIN10_RS5)
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_4;
#elif defined(NTDDI_WIN10_RS4) && (NTDDI_VERSION >= NTDDI_WIN10_RS4)
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_2;
#else
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
#endif

	hr = _device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
	while (hr == E_INVALIDARG && shaderModel.HighestShaderModel > D3D_SHADER_MODEL_6_0)
	{
		shaderModel.HighestShaderModel = static_cast<D3D_SHADER_MODEL>(static_cast<int>(shaderModel.HighestShaderModel) - 1);
		hr = _device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
	}

	if (FAILED(hr))
	{
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;
	}

	// Create CommandQueue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	if (FAILED(_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(_commandQueue.getAddress()))))
	{
		return false;
	}

	// Create SwapChain
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

	hr = factory->CreateSwapChainForHwnd(_commandQueue.get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(_swapChain.getAddress()));
	if (FAILED(hr) == true)
	{
		return false;
	}

	kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	// Create RenderTargetView and DescriptorHeap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = kFrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	hr = _device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(_renderTargetDescriptorHeap.getAddress()));
	if (FAILED(hr) == true)
	{
		return false;
	}

	UINT rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if (FAILED(hr) == true)
	{
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (uint i = 0; i < kFrameCount; ++i)
	{
		hr = _swapChain->GetBuffer(i, IID_PPV_ARGS(_renderTargetResources[i].getAddress()));
		if (FAILED(hr) == true)
		{
			return false;
		}

		_device->CreateRenderTargetView(_renderTargetResources[i].get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}

	// Create Depth/Stencil View
	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = dsvFormat;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES dsHeapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC dsDesc = CD3DX12_RESOURCE_DESC::Tex2D(dsvFormat, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ID3D12Resource2* depthStencilBuffer;
	hr = _device->CreateCommittedResource(
		&dsHeapProperty, D3D12_HEAP_FLAG_NONE, &dsDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, IID_PPV_ARGS(&depthStencilBuffer)
	);
	if (FAILED(hr) == true)
	{
		return false;
	}
	depthStencilBuffer->SetName(L"Depth/Stencil Resource Heap");

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = _device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_depthStencilDescriptorHeap.getAddress()));
	if (FAILED(hr) == true)
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = dsvFormat;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
	_device->CreateDepthStencilView(
		depthStencilBuffer, &depthStencilViewDesc, _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// etc
	_viewport = dk_new D3D12_VIEWPORT;
	_viewport->TopLeftX = 0;
	_viewport->TopLeftY = 0;
	_viewport->Width = static_cast<FLOAT>(width);
	_viewport->Height = static_cast<FLOAT>(height);
	_viewport->MinDepth = 0.0f;
	_viewport->MaxDepth = 1.0f;

	_scissorRect = dk_new D3D12_RECT;
	_scissorRect->left = 0;
	_scissorRect->top = 0;
	_scissorRect->right = static_cast<LONG>(width);
	_scissorRect->bottom = static_cast<LONG>(height);

#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(_pd3dSrvDescHeap.getAddress())) != S_OK)
		{
			return false;
		}

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(
			_device.get(), kFrameCount, DXGI_FORMAT_R8G8B8A8_UNORM, _pd3dSrvDescHeap.get(),
			_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(), _pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart()
		);
	}
#endif // USE_IMGUI

	return true;
}
DKCommandList* RenderModule::createCommandList()
{
	HRESULT hr;

	RenderResourcePtr<ID3D12CommandAllocator> outCommandAllocator[RenderModule::kFrameCount] = {nullptr,};
	RenderResourcePtr<ID3D12GraphicsCommandList> outCommandList = nullptr;
	for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
	{
		hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(outCommandAllocator[i].getAddress()));
		if (FAILED(hr))
		{
			return nullptr;
		}
	}

	hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, outCommandAllocator[0].get(), NULL, IID_PPV_ARGS(outCommandList.getAddress()));
	if (FAILED(hr))
	{
		return nullptr;
	}

	return dk_new DKCommandList(outCommandAllocator, outCommandList);
}
bool RenderModule::initialize_createFence()
{
	for (uint i = 0; i < kFrameCount; ++i)
	{
		HRESULT hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fences[i].getAddress()));
		if (FAILED(hr))
		{
			return false;
		}

		_fences[i]->Signal(_fenceValues[i]);
	}

	return true;
}
bool RenderModule::initialize_createFenceEvent()
{
	_fenceEvent = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
	return true;
}

bool RenderModule::createRootSignature(RenderPass& inoutRenderPass)
{
	const uint32 parameterCount = static_cast<uint32>(inoutRenderPass._shaderVariables.size()) + 1;	// Texture Bindless 전용 +1
	DKVector<D3D12_ROOT_PARAMETER> rootParameters;
	rootParameters.resize(parameterCount);
	uint rootParameterIndex = 0;

	// Bindless Texture 2D Table
	D3D12_DESCRIPTOR_RANGE  texture2DTableDescriptorRange[1];
	texture2DTableDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	texture2DTableDescriptorRange[0].NumDescriptors = TEXTUREBINDLESS_MAX_COUNT;
	texture2DTableDescriptorRange[0].RegisterSpace = TEXTUREBINDLESS_SPACE;			// TextureBindless 전용 space
	texture2DTableDescriptorRange[0].BaseShaderRegister = 0;
	texture2DTableDescriptorRange[0].OffsetInDescriptorsFromTableStart = 0; //D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	D3D12_ROOT_DESCRIPTOR_TABLE texture2DTableDescriptorTable;
	texture2DTableDescriptorTable.NumDescriptorRanges = _countof(texture2DTableDescriptorRange);
	texture2DTableDescriptorTable.pDescriptorRanges = &texture2DTableDescriptorRange[0];

	rootParameters[rootParameterIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[rootParameterIndex].DescriptorTable = texture2DTableDescriptorTable;
	rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	++rootParameterIndex;

	// RenderPass Parameters
	for (const ShaderVariable& parameterDefinition : inoutRenderPass._shaderVariables)
	{
		D3D12_ROOT_DESCRIPTOR constantBufferDescriptor = {};
		constantBufferDescriptor.RegisterSpace = 0;		// 현재 DuckingEngine은 0번 Space만 사용합니다.
		constantBufferDescriptor.ShaderRegister = parameterDefinition._register;

		rootParameters[rootParameterIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[rootParameterIndex].Descriptor = constantBufferDescriptor;
		rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		++rootParameterIndex;
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
	rootSignatureDesc.NumParameters = static_cast<uint32>(rootParameters.size());
	rootSignatureDesc.pParameters = rootParameters.data();
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;
	rootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	ID3DBlob* signature = nullptr;
	ID3DBlob* errorBuffer = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuffer);
	if (FAILED(hr) == true)
	{
		OutputDebugStringA(static_cast<char*>(errorBuffer->GetBufferPointer()));
		return false;
	}

	hr = _device->CreateRootSignature(
		0,
		signature->GetBufferPointer(), static_cast<uint>(signature->GetBufferSize()),
#if 0
		__uuidof(ID3D12RootSignature), &inoutRenderPass._rootSignature
#else
		IID_PPV_ARGS(inoutRenderPass._rootSignature.getAddress())
#endif
	);
	if (SUCCEEDED(hr) == false) return false;

	return true;
}
const wchar_t* charTowChar(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
#pragma warning(push)
#pragma warning(disable:4996)
	mbstowcs(wc, c, cSize);
#pragma warning(pop)

	return wc;
}
bool compileShader(const char* shaderPath, const char* entry, const bool isVertexShader, IDxcBlob* shader, D3D12_SHADER_BYTECODE& outShader)
{
	RenderResourcePtr<IDxcUtils> utils(nullptr);
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.getAddress()));
	if (FAILED(hr) == true)
	{
		DK_ASSERT_LOG(false, "");
		return false;
	}

	RenderResourcePtr<IDxcCompiler3> compiler3(nullptr);
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler3.getAddress()));
	if (FAILED(hr) == true)
	{
		DK_ASSERT_LOG(false, "");
		return false;
	}

	const wchar_t* shaderPathW = charTowChar(shaderPath);
	const wchar_t* shaderEntryW = charTowChar(entry);

	RenderResourcePtr<IDxcBlobEncoding> sourceBlob(nullptr);
	hr = utils->LoadFile(shaderPathW, nullptr, sourceBlob.getAddress());
	if (FAILED(hr) == true)
	{
		DK_ASSERT_LOG(false, "");
		dk_delete_array shaderPathW;
		dk_delete_array shaderEntryW;
		return false;
	}

	DKVector<LPCWSTR> arguments;
	//-E for the entry point (eg. PSMain)
	arguments.push_back(L"-E");
	arguments.push_back(shaderEntryW);

	//-T for the target profile (eg. ps_6_2)
	arguments.push_back(L"-T");
	arguments.push_back(isVertexShader ? L"vs_6_2" : L"ps_6_2");

	//Strip reflection data and pdbs (see later)
	arguments.push_back(L"-Qstrip_debug");
	arguments.push_back(L"-Qstrip_reflect");

	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
	arguments.push_back(DXC_ARG_DEBUG); //-Zi
	arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp

	//for (const std::wstring& define : defines)
	//{
	//	arguments.push_back(L"-D");
	//	arguments.push_back(define.c_str());
	//}

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_ACP;

	RenderResourcePtr<IDxcResult> result(nullptr);
	hr = compiler3->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()), NULL, IID_PPV_ARGS(result.getAddress()));
	dk_delete_array shaderPathW;
	dk_delete_array shaderEntryW;
	if (FAILED(hr))
	{
		RenderResourcePtr<IDxcBlobUtf8> errors(nullptr);
		hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.getAddress()), nullptr);
		if (SUCCEEDED(hr))
			DK_ASSERT_LOG(false, "Shader Compilation Error: %s", errors->GetStringPointer());

		return false;
	}

	HRESULT status(S_OK);
	hr = result->GetStatus(&status);
	if (FAILED(hr) || FAILED(status))
	{
		RenderResourcePtr<IDxcBlobUtf8> errors(nullptr);
		hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.getAddress()), nullptr);
		if (SUCCEEDED(hr))
			DK_ASSERT_LOG(false, "Shader Compilation Error: %s", errors->GetStringPointer());

		return false;
	}

	hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr);
	if (FAILED(hr))
	{
		DK_ASSERT_LOG(false, "");
		return false;
	}

	outShader.BytecodeLength = shader->GetBufferSize();
	outShader.pShaderBytecode = shader->GetBufferPointer();

	return true;
}
bool RenderModule::createPipelineObjectState(const char* vsPath, const char* vsEntry, const char* psPath, const char* psEntry, RenderPass& inoutRenderPass)
{
	ID3D12PipelineState* createdPipelineStateObject = nullptr;

	D3D12_SHADER_BYTECODE vertexShaderView = {};
	RenderResourcePtr<IDxcBlob> vertexShader = nullptr;
	bool success = compileShader(vsPath, vsEntry, true, vertexShader.get(), vertexShaderView);
	if (success == false)
	{
		DK_ASSERT_LOG(false, "VertexShader Compile에 실패");
		return false;
	}
	D3D12_SHADER_BYTECODE pixelShaderView = {};
	RenderResourcePtr<IDxcBlob> pixelShader = nullptr;
	success = compileShader(psPath, psEntry, false, pixelShader.get(), pixelShaderView);
	if (success == false)
	{
		DK_ASSERT_LOG(false, "PixelShader Compile에 실패");
		return false;
	}

	// #todo- 나중에 RenderPass에서 받아올 수 있도록 할 것
	static D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEXES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = ARRAYSIZE(inputLayout);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
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
	psoDesc.pRootSignature = inoutRenderPass._rootSignature.get();
	psoDesc.VS = vertexShaderView;
	psoDesc.PS = pixelShaderView;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = dsvFormat;

	success = SUCCEEDED(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(inoutRenderPass._pipelineStateObject.getAddress())));
	if (success == false)
	{
		return false;
	}

	return true;
}
bool RenderModule::createRenderPass(RenderPass::CreateInfo&& info)
{
	using FindResult = DKHashMap<DKString, RenderPass>::iterator;
	FindResult find = _renderPassMap.find(info._renderPassName);

	if (find != _renderPassMap.end())
	{
		DK_ASSERT_LOG(false, "중복된 이름의 RenderPass가 있습니다.\nName: %s", find->first.c_str());
		return false;
	}
	using InsertResult = DKPair<DKHashMap<DKString, RenderPass>::iterator, bool>;
	InsertResult insertResult = _renderPassMap.insert(DKPair<DKString, RenderPass>(info._renderPassName, RenderPass()));
	if (insertResult.second == false)
	{
		DK_ASSERT_LOG(false, "HashMap Insert에 실패!");
		return false;
	}

	RenderPass& renderPass = insertResult.first->second;
	renderPass._shaderVariables.swap(info._variables);
	if (createRootSignature(renderPass) == false)
		return false;
	if (createPipelineObjectState(info._vertexShaderPath, info._vertexShaderEntry, info._pixelShaderPath, info._pixelShaderEntry, renderPass) == false)
		return false;

	return true;
}
#pragma endregion

void RenderModule::waitFenceAndResetCommandList()
{
	kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	uint32 test = static_cast<uint>(_fences[kCurrentFrameIndex]->GetCompletedValue());
	if (static_cast<uint>(_fences[kCurrentFrameIndex]->GetCompletedValue()) < _fenceValues[kCurrentFrameIndex])
	{
		_fences[kCurrentFrameIndex]->SetEventOnCompletion(_fenceValues[kCurrentFrameIndex], _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}

	++_fenceValues[kCurrentFrameIndex];

	_commandList->reset();
}
void RenderModule::execute()
{
	_commandList->_commandList->Close();

	kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	DKVector<ID3D12CommandList*> commandLists;
	commandLists.push_back(_commandList->_commandList.get());
	_commandQueue->ExecuteCommandLists(static_cast<uint>(commandLists.size()), commandLists.data());

	kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	_commandQueue->Signal(_fences[kCurrentFrameIndex].get(), _fenceValues[kCurrentFrameIndex]);
}

ID3D12Resource* RenderModule::createBufferInternal(const void* data, const uint size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state)
{
	ID3D12Resource* returnBuffer = nullptr;
#if 0
	D3D12_HEAP_PROPERTIES props;
	props.Type = type;

	D3D12_RESOURCE_DESC desc{};
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
#else
	CD3DX12_HEAP_PROPERTIES props(type);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
#endif
	HRESULT hr = _device->CreateCommittedResource(
		&props, D3D12_HEAP_FLAG_NONE, &desc,
		state, nullptr, IID_PPV_ARGS(&returnBuffer)
	);
	
	if (FAILED(hr) == true)
	{
		return nullptr;
	}

	return returnBuffer;
}
ID3D12Resource* RenderModule::createDefaultBuffer(const void* data, const uint size, const D3D12_RESOURCE_STATES state)
{
	return createBufferInternal(data, size, D3D12_HEAP_TYPE_DEFAULT, state);
}
IBuffer* RenderModule::createUploadBuffer(const void* data, const uint size)
{
	RenderResourcePtr<ID3D12Resource> buffers[RenderModule::kFrameCount];
	uint32 alignedSize = (size + 255) & ~255;
	for (uint32 i = 0; i < kFrameCount; ++i)
	{
		buffers[i] = createBufferInternal(data, alignedSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	return dk_new IBuffer(buffers);
}
ID3D12Resource* RenderModule::createInitializedDefaultBuffer(const void* data, const uint bufferSize)
{
	ID3D12Resource* uploadBuffer = createBufferInternal(data, bufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	if (uploadBuffer == nullptr)
		return nullptr;

	void* uploadBufferGPUAddress = nullptr;
	HRESULT hr = uploadBuffer->Map(0, nullptr, &uploadBufferGPUAddress);
	DK_ASSERT_LOG(SUCCEEDED(hr), "Map 실패");
	memcpy(uploadBufferGPUAddress, data, bufferSize);

	D3D12_RESOURCE_STATES defaultBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	ID3D12Resource* defaultBuffer = createDefaultBuffer(data, bufferSize, defaultBufferState);
	if (defaultBuffer == nullptr)
		return nullptr;

	waitFenceAndResetCommandList();

	_commandList->_commandList->CopyResource(defaultBuffer, uploadBuffer);
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Transition.pResource = defaultBuffer;
	barrier.Transition.StateBefore = defaultBufferState;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	_commandList->_commandList->ResourceBarrier(1, &barrier);

	execute();

	return defaultBuffer;
}
const bool RenderModule::createVertexBuffer(const void* data, const uint strideSize, const uint bufferSize, VertexBufferViewRef& outView)
{
	ID3D12Resource* defaultBuffer = createInitializedDefaultBuffer(data, bufferSize);
	if (defaultBuffer == nullptr)
		return false;

	D3D12_VERTEX_BUFFER_VIEW view;
	view.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
	view.StrideInBytes = strideSize;
	view.SizeInBytes = bufferSize;

	outView = std::make_shared<D3D12_VERTEX_BUFFER_VIEW>(view);

	return true;
}
const bool RenderModule::createIndexBuffer(const void* data, const uint bufferSize, IndexBufferViewRef& outView)
{
	ID3D12Resource* defaultBuffer = createInitializedDefaultBuffer(data, bufferSize);
	if (defaultBuffer == nullptr)
		return false;

	D3D12_INDEX_BUFFER_VIEW view;
	view.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
	view.Format = DXGI_FORMAT_R32_UINT;
	view.SizeInBytes = bufferSize;

	outView = std::make_shared<D3D12_INDEX_BUFFER_VIEW>(view);

	return true;
}

const bool RenderModule::createTextureBindlessDescriptorHeap(RenderResourcePtr<ID3D12DescriptorHeap>& outDescriptorHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = TEXTUREBINDLESS_MAX_COUNT;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(outDescriptorHeap.getAddress()));
	if (FAILED(hr))
		return false;

	return true;
}

bool RenderModule::createTexture(const TextureRaw& textureRaw, D3D12_CPU_DESCRIPTOR_HANDLE& handleStart, const uint32 index)
{
	D3D12_RESOURCE_DESC resourceDescription = {};
	// now describe the texture with the information we have obtained from the image
	resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	resourceDescription.Width = textureRaw._width; // width of the texture
	resourceDescription.Height = textureRaw._height; // height of the texture
	resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	resourceDescription.Format = textureRaw._format; // This is the dxgi format of the image (format of the pixels)
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
	HRESULT hr = _device->CreateCommittedResource(
		&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDescription,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer)
	);
	if (FAILED(hr) == true)
	{
		return false;
	}
	defaultBuffer->SetName(L"DefaultBuffer - Texture");

	UINT64 textureUploadBufferSize = 0;
	// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
	// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
	// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
#if 0
	UINT64 imageBytesPerRow = resourceDescription.Width * (textureRaw._bitsPerPixel / 4);
	textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (resourceDescription.Height - 1)) + imageBytesPerRow;
#else
	_device->GetCopyableFootprints(&resourceDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
#endif

	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	ID3D12Resource* uploadBuffer;
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
	hr = _device->CreateCommittedResource(
		&heapProperty, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)
	);
	if (FAILED(hr) == true)
	{
		return false;
	}
	uploadBuffer->SetName(L"UploadBuffer - Texture");

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = textureRaw._data;
	textureData.RowPitch = textureRaw._width * (textureRaw._bitsPerPixel / 8);
	textureData.SlicePitch = textureData.RowPitch * textureRaw._height;

	waitFenceAndResetCommandList();

	UpdateSubresources(_commandList->_commandList.get(), defaultBuffer, uploadBuffer, 0, 0, 1, &textureData);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	_commandList->_commandList->ResourceBarrier(1, &barrier);

	execute();

	UINT descriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureRaw._format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	handleStart.ptr += index * _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_device->CreateShaderResourceView(defaultBuffer, &srvDesc, handleStart);

	return true;
}

void RenderModule::preRender()
{
	waitFenceAndResetCommandList();

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _renderTargetResources[kCurrentFrameIndex].get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->_commandList->ResourceBarrier(1, &barrier);

	const UINT rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += rtvDescriptorSize * kCurrentFrameIndex;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	_commandList->_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float clearColor[] = { 0.5f, 0.5f, 0.9f, 1.0f };
	_commandList->_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	_commandList->_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	_commandList->_commandList->RSSetViewports(1, _viewport.get());
	_commandList->_commandList->RSSetScissorRects(1, _scissorRect.get());
	_commandList->_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

bool RenderModule::bindRenderPass(RenderPass& renderPass)
{
	_commandList->_commandList->SetGraphicsRootSignature(renderPass._rootSignature.get());
	_commandList->_commandList->SetPipelineState(renderPass._pipelineStateObject.get());

	RenderResourcePtr<ID3D12DescriptorHeap>& textureDescriptorHeap = DuckingEngine::getInstance().getTextureManagerWritable().getTextureDescriptorHeapWritable();
	_commandList->_commandList->SetDescriptorHeaps(1, textureDescriptorHeap.getAddress());
	_commandList->_commandList->SetGraphicsRootDescriptorTable(0, textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	return true;
}
void RenderModule::bindConstantBuffer(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress)
{
	_commandList->_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex + 1, gpuAdress);
}
void RenderModule::setVertexBuffers(const uint32 startSlot, const uint32 numViews, const D3D12_VERTEX_BUFFER_VIEW* views)
{
	_commandList->_commandList->IASetVertexBuffers(startSlot, numViews, views);
}
void RenderModule::setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view)
{
	_commandList->_commandList->IASetIndexBuffer(view);
}
void RenderModule::drawIndexedInstanced(const uint32 indexCountPerInstance, const uint32 instanceCount, const uint32 startIndexLocation, const int baseVertexLocation, const uint32 startInstanceLocation)
{
	_commandList->_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void RenderModule::endRender()
{
#ifdef USE_IMGUI
	_commandList->_commandList->SetDescriptorHeaps(1, _pd3dSrvDescHeap.getAddress());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList->_commandList.get());
#endif // USE_IMGUI

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _renderTargetResources[kCurrentFrameIndex].get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->_commandList->ResourceBarrier(1, &barrier);

	execute();

	UINT syncInterval = 0; //gVSync ? 1 : 0;
	UINT presentFlags = 0; // CheckTearingSupport() && !gVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	HRESULT hr = _swapChain->Present(syncInterval, presentFlags);
	DK_ASSERT_LOG(SUCCEEDED(hr), "Present 실패!");
}

bool DKCommandList::reset()
{
	_commandAllocators[RenderModule::kCurrentFrameIndex]->Reset();
	_commandList->Reset(_commandAllocators[RenderModule::kCurrentFrameIndex].get(), nullptr);

	return false;
}

void IBuffer::upload(const void* data, uint32 size)
{
	void* address = nullptr;
	HRESULT hr = _buffers[0]->Map(0, nullptr, &address);
	DK_ASSERT_LOG(SUCCEEDED(hr), "Map에 실패하였습니다.");
	memcpy(address, data, size);
	_buffers[0]->Unmap(0, nullptr);
}

D3D12_GPU_VIRTUAL_ADDRESS IBuffer::getGPUVirtualAddress()
{
	return _buffers[0]->GetGPUVirtualAddress();
}
