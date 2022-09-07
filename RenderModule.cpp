#include "stdafx.h"
#include "RenderModule.h"

#pragma region Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "d3dx12.h"

#include <wrl.h>

#if defined(USE_TINYXML)
#include "tinyxml.h"
#endif
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#endif
#pragma endregion

#include "DuckingEngine.h"
#include "ResourceManager.h"
#include "TextureManager.h"

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"

#include "MaterialParameter.h"
#include "Material.h"

uint RenderModule::kCurrentFrameIndex = 0;
DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

#define TEXTUREBINDLESS_MAX_COUNT 256

// #todo- 나중에 RenderPass에서 받아올 수 있도록 할 것
static D3D12_INPUT_ELEMENT_DESC gSkinnedMeshLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

RenderPass::~RenderPass()
{
	_rootSignature->Release();
	_pipelineStateObject->Release();
}

DKCommandList::~DKCommandList()
{
	_commandAllocator->Release();
	_commandList->Release();
}

RenderModule::~RenderModule()
{
	_device->Release();
	// commandList는 소멸자에서 알아서 호출
	//_commandList->_commandAllocator->Release();
	//_commandList->_commandList->Release();
	_commandQueue->Release();
	_swapChain->Release();

	for (uint i = 0; i < kFrameCount; ++i)
	{
		_fences[i]->Release();
	}

	CloseHandle(_fenceEvent);

	_renderPassMap.clear();
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
	if (initialize_loadRenderPass() == false) return false;
	_commandList = createCommandList();
	if (_commandList == nullptr) return false;
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

	// Create RenderDevice
	ID3D12Device8* device;
	if (_useWarpDevice == true)
	{
		IDXGIAdapter* warpAdapter;
		if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))) == false)
		{
			return false;
		}
		if (SUCCEEDED(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device))) == false)
		{
			return false;
		}
	}
	else
	{
		IDXGIAdapter1* hardwareAdapter;
		GetHardwareAdapter(factory, &hardwareAdapter);
		if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device))))
		{
			return false;
		}
	}

	// Create CommandQueue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	if (FAILED(_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&_commandQueue))))
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

	IDXGISwapChain1* swapChain1 = static_cast<IDXGISwapChain1*>(_swapChain);
	hr = factory->CreateSwapChainForHwnd(_commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(hr) == true)
	{
		return false;
	}

	_swapChain = static_cast<IDXGISwapChain4*>(swapChain1);

	kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	// Create RenderTargetView and DescriptorHeap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = kFrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_renderTargetDescriptorHeap));
	if (FAILED(hr) == true)
	{
		return false;
	}

	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if (FAILED(hr) == true)
	{
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (uint i = 0; i < kFrameCount; ++i)
	{
		hr = _swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargetResources[i]));
		if (FAILED(hr) == true)
		{
			return false;
		}

		device->CreateRenderTargetView(_renderTargetResources[i], nullptr, rtvHandle);
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
	hr = device->CreateCommittedResource(
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
	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_depthStencilDescriptorHeap));
	if (FAILED(hr) == true)
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = dsvFormat;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
	device->CreateDepthStencilView(
		depthStencilBuffer, &depthStencilViewDesc, _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// etc
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
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_pd3dSrvDescHeap)) != S_OK)
		{
			return false;
		}

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(
			device, kFrameCount,DXGI_FORMAT_R8G8B8A8_UNORM, _pd3dSrvDescHeap,
			_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(), _pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart()
		);
	}
#endif // USE_IMGUI

	return true;
}
#if defined(USE_TINYXML)
bool loadParameterDefinition(TiXmlElement* parameterNode, MaterialParameterDefinition& outParameter)
#else
static_assert("현재 지원되는 XML 로더가 없습니다.");
#endif
{
	const char* parameterName = parameterNode->ToElement()->Attribute("Name");
	const char* parameterTypeRaw = parameterNode->ToElement()->Attribute("Type");
	const uint parameterRegister = parameterNode->ToElement()->Attribute("Register") != nullptr ? atoi(parameterNode->ToElement()->Attribute("Register")) : -1;
	const char* parameterValue = parameterNode->ToElement()->Value();

	const MaterialParameterType parameterType = convertNameToEnum(parameterTypeRaw);
	if (parameterType == MaterialParameterType::COUNT)
	{
		DK_ASSERT_LOG(false, "현재 지원하지 않는 MaterialType을 사용하였습니다. ParameterName: %s, ParameterType: %s", parameterName, parameterType);
		return false;
	}

	outParameter._name = parameterName;
	outParameter._type = parameterType;
	outParameter._register = parameterRegister;

	Ptr<void>& valuePtr = outParameter._value;
	switch (outParameter._type)
	{
	case MaterialParameterType::FLOAT:
		float value = atof(parameterValue);
		valuePtr.assign(dk_new float);

		memcpy(valuePtr.get(), &value, sizeof(float));
		break;
	case MaterialParameterType::TEXTURE:
	{
		size_t pathSize = strlen(parameterValue);
		valuePtr.assign(dk_new char[pathSize]);

		memcpy(valuePtr.get(), parameterValue, pathSize);
		break;
	}
	default:
		return false;
	}

	return true;
}
bool RenderModule::initialize_loadRenderPass()
{
	static const char* renderPassGroupPath = "C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/RenderPass/RenderPassGroup.xml";
#if defined(USE_TINYXML)
	TiXmlDocument doc;
	doc.LoadFile(renderPassGroupPath);
	TiXmlElement* rootNode = doc.FirstChildElement("RenderPassGroup");
	if (rootNode == nullptr) return false;

	for (TiXmlNode* renderPassNode = rootNode->FirstChild(); renderPassNode != nullptr; renderPassNode = renderPassNode->NextSibling())
	{
		const char* renderPassName		= renderPassNode->ToElement()->Attribute("Name");
		auto foundRenderPass = _renderPassMap.find(renderPassName);
		if (foundRenderPass == _renderPassMap.end())
		{
			DK_ASSERT_LOG(false, "이미 있는 RenderPass[%s]를 재정의 하였습니다. 현재 것은 무시하고 넘어갑니다.", renderPassName);
			continue;
		}

		// parsing
		const char* vertexShaderPath = nullptr;
		const char* vertexShaderEntry = nullptr;
		const char* pixelShaderPath = nullptr;
		const char* pixelShaderEntry = nullptr;
		const char* materialPath = nullptr;
		RenderPass renderPass;
		for (TiXmlNode* childNode = renderPassNode->FirstChild(); childNode != nullptr; childNode = childNode->NextSibling())
		{
			std::string nodeName = childNode->ToElement()->Value();
			if (nodeName == "VertexShader")
			{
				vertexShaderEntry = childNode->ToElement()->Attribute("Entry");
				vertexShaderPath = childNode->ToElement()->Value();
			}
			else if (nodeName == "PixelShader")
			{
				pixelShaderEntry = childNode->ToElement()->Attribute("Entry");
				pixelShaderPath = childNode->ToElement()->Value();
			}
			else if (nodeName == "Parameter")
			{
				MaterialParameterDefinition parameter;
				if (loadParameterDefinition(childNode->ToElement(), parameter) == false)
				{
					return false;
				}
				renderPass._parameters.push_back(std::move(parameter));
			}
			else if (nodeName == "Material")
			{
				if (createMaterialDefinition(materialPath, renderPass._materialDefinition) == false)
				{
					return false;
				}
			}
			else
			{
				DK_ASSERT_LOG(false, "지원하지 RenderPass ChildNode입니다. NodeName: %s", nodeName);
				return false;
			}
		}

		if (createRootSignature(renderPass) == false)
		{
			return false;
		}
		if (createPipelineObjectState(vertexShaderPath, vertexShaderEntry, pixelShaderPath, pixelShaderEntry, renderPass) == false)
		{
			return false;
		}

		_renderPassMap.insert(std::make_pair(renderPassName, std::move(renderPass)));
	}
#else
	static_assert("현재 지원하지 XML 로더가 없습니다.");
#endif
}
bool RenderModule::createMaterialDefinition(const char* materialPath, MaterialDefinition& outMaterialDefinition) const
{
#if defined(USE_TINYXML)
	TiXmlDocument doc;
	doc.LoadFile(materialPath);
	TiXmlElement* rootNode = doc.FirstChildElement("Material");
	if (rootNode == nullptr)
	{
		DK_ASSERT_LOG(false, "Material XML이 이상합니다. path: %s", materialPath);
		return false;
	}

	outMaterialDefinition._materialName = rootNode->Attribute("Name");

	for (TiXmlNode* parameterNode = rootNode->FirstChild(); parameterNode != nullptr; parameterNode = parameterNode->NextSibling())
	{
		MaterialParameterDefinition parameter;
		if (loadParameterDefinition(parameterNode->ToElement(), parameter) == false)
		{
			return false;
		}

		outMaterialDefinition._parameters.push_back(std::move(parameter));
	}

	return true;
#else
	static_assert("현재 지원하는 XML 로더가 없습니다.");
	return false;
#endif
}
bool RenderModule::createRootSignature(RenderPass& inoutRenderPass) const
{
	// Material ConstantBuffer 전용 하나 추가합니다.
	// Texture Bindless 전용 하나 더 추가합니다.
	const uint parameterCount = inoutRenderPass._parameters.size() + 1 + 1;
	DKVector<D3D12_ROOT_PARAMETER> rootParameters;
	rootParameters.resize(parameterCount);
	uint rootParameterIndex = 0;

	// Bindless Texture 2D Table
	D3D12_DESCRIPTOR_RANGE  texture2DTableDescriptorRange[1];
	texture2DTableDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	texture2DTableDescriptorRange[0].NumDescriptors = TEXTUREBINDLESS_MAX_COUNT;
	texture2DTableDescriptorRange[0].RegisterSpace = 0;						// 현재 DuckingEngine은 0번 Space만 사용합니다.
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
	for (const MaterialParameterDefinition& parameterDefinition : inoutRenderPass._parameters)
	{
		D3D12_ROOT_DESCRIPTOR constantBufferDescriptor = {};
		constantBufferDescriptor.RegisterSpace = 0;		// 현재 DuckingEngine은 0번 Space만 사용합니다.
		constantBufferDescriptor.ShaderRegister = parameterDefinition._register;

		rootParameters[rootParameterIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[rootParameterIndex].Descriptor = constantBufferDescriptor;
		rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

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
	rootSignatureDesc.NumParameters = rootParameters.size();
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

	ID3D12RootSignature* rootSignature = nullptr;
	bool success = _device->CreateRootSignature(
		0,
		signature->GetBufferPointer(), static_cast<uint>(signature->GetBufferSize()),
#if 0
		__uuidof(ID3D12RootSignature), &inoutRenderPass._rootSignature
#else
		IID_PPV_ARGS(&inoutRenderPass._rootSignature)
#endif
	);
	if (success == false) return false;

	return true;
}
const wchar_t* charTowChar(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}
bool loadShader(const char* shaderPath, const char* entry, const bool isVertexShader, D3D12_SHADER_BYTECODE outShader)
{
	ID3DBlob* errorBuffer;
	ID3DBlob* shader;
	const wchar_t* wPath = charTowChar(shaderPath);
	HRESULT hr = D3DCompileFromFile(
		wPath, nullptr, nullptr, entry,
		isVertexShader == true ? "vs_5_0" : "ps_5_0",
#if defined(_DK_DEBUG_)
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
#else
		0
#endif // _DK_DEBUG_
		0, &shader, &errorBuffer
	);
	dk_delete_array(wPath);
	if (FAILED(hr) == true)
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		return false;
	}

	outShader.BytecodeLength = shader->GetBufferSize();
	outShader.pShaderBytecode = shader->GetBufferPointer();
	return true;
}
bool RenderModule::createPipelineObjectState(const char* vsPath, const char* vsEntry, const char* psPath, const char* psEntry, RenderPass& inoutRenderPass) const
{
	ID3D12PipelineState* createdPipelineStateObject = nullptr;

	// #todo- ModelProperty로부터 얻어온 Shader를 사용해야할 것입니다.
	D3D12_SHADER_BYTECODE vertexShader;
	bool success = loadShader(vsPath, vsEntry, true, vertexShader);
	if (success == false)
	{
		DK_ASSERT_LOG(false, "VertexShader Compile에 실패");
		return false;
	}
	D3D12_SHADER_BYTECODE pixelShader;
	success = loadShader(psPath, psEntry, false, pixelShader);
	if (success == false)
	{
		DK_ASSERT_LOG(false, "PixelShader Compile에 실패");
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC* inputLayout = gSkinnedMeshLayout;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
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
	psoDesc.pRootSignature = inoutRenderPass._rootSignature;
	psoDesc.VS = vertexShader;
	psoDesc.PS = pixelShader;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = dsvFormat;

	success = SUCCEEDED(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&inoutRenderPass._pipelineStateObject)));
	if (success == false)
	{
		return false;
	}

	return true;
}
DKCommandList* RenderModule::createCommandList()
{
	HRESULT hr;

	ID3D12CommandAllocator* outCommandAllocator = nullptr;
	ID3D12GraphicsCommandList* outCommandList = nullptr;
	hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&outCommandAllocator));
	if (FAILED(hr))
	{
		return false;
	}

	hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, outCommandAllocator, NULL, IID_PPV_ARGS(&outCommandList));
	if (FAILED(hr))
	{
		outCommandAllocator->Release();
		return false;
	}

	return dk_new DKCommandList(outCommandAllocator, outCommandList);
}
bool RenderModule::initialize_createFence()
{
	for (uint i = 0; i < kFrameCount; ++i)
	{
		HRESULT hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fences[i]));
		if (FAILED(hr))
		{
			return false;
		}

		_fences[i]->Signal(_fenceValues[i]);
	}
}
bool RenderModule::initialize_createFenceEvent()
{
	_fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
}
#pragma endregion

void RenderModule::waitFenceAndResetCommandList()
{
	kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	if (static_cast<uint>(_fences[kCurrentFrameIndex]->GetCompletedValue()) < _fenceValues[kCurrentFrameIndex])
	{
		_fences[kCurrentFrameIndex]->SetEventOnCompletion(_fenceValues[kCurrentFrameIndex], _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}

	++_fenceValues[kCurrentFrameIndex];

	_commandList->_commandAllocator->Reset();
	_commandList->_commandList->Reset(_commandList->_commandAllocator, nullptr);
}
void RenderModule::execute()
{
	_commandList->_commandList->Close();

	DKVector<ID3D12CommandList*> commandLists;
	commandLists.push_back(_commandList->_commandList);
	_commandQueue->ExecuteCommandLists(static_cast<uint>(commandLists.size()), commandLists.data());

	_commandQueue->Signal(_fences[kCurrentFrameIndex], _fenceValues[kCurrentFrameIndex]);
}
void RenderModule::executeImmediately()
{
	execute();
	waitFenceAndResetCommandList();
}

ID3D12Resource* RenderModule::createBufferInternal(const void* data, const uint size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state) const
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
ID3D12Resource* RenderModule::createDefaultBuffer(const void* data, const uint size, const D3D12_RESOURCE_STATES state) const
{
	return createBufferInternal(data, size, D3D12_HEAP_TYPE_DEFAULT, state);
}
ID3D12Resource* RenderModule::createUploadBuffer(const void* data, const uint size, const D3D12_RESOURCE_STATES state) const
{
	return createBufferInternal(data, size, D3D12_HEAP_TYPE_UPLOAD, state);
}

const bool RenderModule::loadVertexBuffer(const char* modelPath, const uint subMeshIndex, const void* data, const uint strideSize, const uint bufferSize, VertexBufferViewRef& outView)
{
	ID3D12Resource* uploadBuffer = createUploadBuffer(data, bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	if (uploadBuffer == nullptr)
	{
		return false;
	}

	BYTE* dataBegin{ nullptr };
	HRESULT hr = uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin));
	memcpy(dataBegin, data, bufferSize);
	uploadBuffer->Unmap(0, nullptr);

	D3D12_RESOURCE_STATES defaultBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	ID3D12Resource* defaultBuffer = createDefaultBuffer(data, bufferSize, defaultBufferState);
	{
		uploadBuffer->Release();
		return false;
	}

	waitFenceAndResetCommandList();

	_commandList->_commandList->CopyResource(defaultBuffer, uploadBuffer);
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Transition.pResource = defaultBuffer;
	barrier.Transition.StateBefore = defaultBufferState;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	_commandList->_commandList->ResourceBarrier(1, &barrier);

	executeImmediately();

	D3D12_VERTEX_BUFFER_VIEW view;
	view.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
	view.StrideInBytes = strideSize;
	view.SizeInBytes = bufferSize;

	outView = std::make_shared<D3D12_VERTEX_BUFFER_VIEW>(view);

	return true;
}
const bool RenderModule::loadIndexBuffer(const void* data, const uint bufferSize, IndexBufferViewRef& outView)
{
	ID3D12Resource* uploadBuffer = createUploadBuffer(data, bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	if (uploadBuffer == nullptr)
	{
		return false;
	}

	BYTE* dataBegin{ nullptr };
	HRESULT hr = uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin));
	memcpy(dataBegin, data, bufferSize);
	uploadBuffer->Unmap(0, nullptr);

	D3D12_RESOURCE_STATES defaultBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	ID3D12Resource* defaultBuffer = createDefaultBuffer(data, bufferSize, defaultBufferState);
	{
		uploadBuffer->Release();
		return false;
	}

	waitFenceAndResetCommandList();

	_commandList->_commandList->CopyResource(defaultBuffer, uploadBuffer);
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Transition.pResource = defaultBuffer;
	barrier.Transition.StateBefore = defaultBufferState;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	_commandList->_commandList->ResourceBarrier(1, &barrier);

	executeImmediately();

	D3D12_INDEX_BUFFER_VIEW view;
	view.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
	view.Format = DXGI_FORMAT_R32_UINT;
	view.SizeInBytes = bufferSize;

	outView = std::make_shared<D3D12_INDEX_BUFFER_VIEW>(view);

	return true;
}

const MaterialDefinition* RenderModule::findRenderPassByMaterialName(const DKString& materialName) const
{
	for (DKHashMap<const char*, RenderPass>::const_iterator iter = _renderPassMap.begin(); iter != _renderPassMap.end(); ++iter)
	{
		if (materialName == iter->second._materialDefinition._materialName)
		{
			return &iter->second._materialDefinition;
		}
	}

	return nullptr;
}

const bool RenderModule::createTextureBindlessDescriptorHeap(ID3D12DescriptorHeap* outDescriptorHeap) const
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = TEXTUREBINDLESS_MAX_COUNT;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	bool success = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&outDescriptorHeap));
	if (success == false)
		return false;

	return true;
}

bool RenderModule::createTexture(const TextureRaw& textureRaw, const D3D12_CPU_DESCRIPTOR_HANDLE& handle) const
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

	UINT64 textureUploadBufferSize;
	// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
	// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
	// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
	//textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
	_device->GetCopyableFootprints(&resourceDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	ID3D12Resource* uploadBuffer;
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
	hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
	if (FAILED(hr) == true)
	{
		return false;
	}
	uploadBuffer->SetName(L"UploadBuffer - Texture");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = textureRaw._data; // pointer to our image data
	textureData.RowPitch = textureRaw._width * textureRaw._bitsPerPixel; // size of all our triangle vertex data
	textureData.SlicePitch = textureRaw._height; // also the size of our triangle vertex data

	waitFenceAndResetCommandList();

	UpdateSubresources(_commandList->_commandList, defaultBuffer, uploadBuffer, 0, 0, 1, &textureData);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	_commandList->_commandList->ResourceBarrier(1, &barrier);

	executeImmediately();

	UINT descriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureRaw._format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	_device->CreateShaderResourceView(defaultBuffer, &srvDesc, handle);

	return true;
}