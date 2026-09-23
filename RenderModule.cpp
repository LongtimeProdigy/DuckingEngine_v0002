#include "stdafx.h"
#include "RenderModule.h"
#include <cstdint>

#pragma region Lib
#define USE_WINCODEC
#ifdef USE_WINCODEC
#include <wincodec.h>	// Image Process
DXGI_FORMAT GetDXGIFormatFromWICFormat(const WICPixelFormatGUID& format)
{
	if (format == GUID_WICPixelFormat128bppRGBAFloat)      return DXGI_FORMAT_R32G32B32A32_FLOAT;
	if (format == GUID_WICPixelFormat64bppRGBAHalf)        return DXGI_FORMAT_R16G16B16A16_FLOAT;
	if (format == GUID_WICPixelFormat64bppRGBA)            return DXGI_FORMAT_R16G16B16A16_UNORM;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	if (format == GUID_WICPixelFormat96bppRGBFloat)        return DXGI_FORMAT_R32G32B32_FLOAT;
#endif

	if (format == GUID_WICPixelFormat32bppRGBA)            return DXGI_FORMAT_R8G8B8A8_UNORM;
	if (format == GUID_WICPixelFormat32bppBGRA)            return DXGI_FORMAT_B8G8R8A8_UNORM;
	if (format == GUID_WICPixelFormat32bppBGR)             return DXGI_FORMAT_B8G8R8X8_UNORM;

	if (format == GUID_WICPixelFormat32bppRGBA1010102XR)   return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
	if (format == GUID_WICPixelFormat32bppRGBA1010102)     return DXGI_FORMAT_R10G10B10A2_UNORM;

	if (format == GUID_WICPixelFormat16bppBGRA5551)        return DXGI_FORMAT_B5G5R5A1_UNORM;
	if (format == GUID_WICPixelFormat16bppBGR565)          return DXGI_FORMAT_B5G6R5_UNORM;

	if (format == GUID_WICPixelFormat32bppGrayFloat)       return DXGI_FORMAT_R32_FLOAT;
	if (format == GUID_WICPixelFormat16bppGrayHalf)        return DXGI_FORMAT_R16_FLOAT;
	if (format == GUID_WICPixelFormat16bppGray)            return DXGI_FORMAT_R16_UNORM;
	if (format == GUID_WICPixelFormat8bppGray)             return DXGI_FORMAT_R8_UNORM;
	if (format == GUID_WICPixelFormat8bppAlpha)            return DXGI_FORMAT_A8_UNORM;

	return DXGI_FORMAT_UNKNOWN;
}
WICPixelFormatGUID GetConvertToWICFormat(const WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

	else return GUID_WICPixelFormatDontCare;
}
uint32 GetDXGIFormatBitsPerPixel(const DXGI_FORMAT& dxgiFormat)
{
	if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

	else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
	else
	{
		DK_ASSERT_LOG(false, "올바르지 않은 Texture Type입니다. 확인 요망!");
		return 0xffffffff;
	}
}
#endif

#if defined(_DK_DEBUG_)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif
#pragma endregion

#include "DuckingEngine.h"
#include "ResourceManager.h"
#include "ShaderCompiler.h"
#include "RaytracingRenderer.h"

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"

#include "Material.h"

namespace DK
{
	static constexpr const bool gSerializeRender = false;

	RenderPass* gCurrentBindedRenderPass = nullptr;
	Pipeline* gCurrentBindedPipeline = nullptr;

	static const float4 gClearRenderTargetViewColor(1, 0, 0, 1);

	uint32 RenderModule::kCurrentFrameIndex = 0;
	uint32 RenderModule::kCurrentBackBufferIndex = 0;
	uint32 RenderModule::kWidth = 0;
	uint32 RenderModule::kHeight = 0;
	static constexpr const DXGI_FORMAT gDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DXGI_FORMAT GetDepthResourceFormat(DXGI_FORMAT depthformat)
	{
		DXGI_FORMAT resformat;
		switch (depthformat)
		{
		case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
			resformat = DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS;
			break;
		case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
			resformat = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
			break;
		case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
			resformat = DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
			break;
		case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			resformat = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS;
			break;
		}

		return resformat;
	}
	DXGI_FORMAT GetDepthSRVFormat(DXGI_FORMAT depthformat)
	{
		DXGI_FORMAT srvformat;
		switch (depthformat)
		{
		case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
			srvformat = DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
			break;
		case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
			srvformat = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
			srvformat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
			break;
		case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			srvformat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			break;
		}
		return srvformat;
	}

#define TEXTUREBINDLESS_MAX_COUNT 8192
#define BINDLESSTEXTUREARRAY_SPACE 10
	static constexpr const D3D12_DESCRIPTOR_HEAP_TYPE gTextureBindlessDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	RenderModule::~RenderModule()
	{
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
	bool RenderModule::initialize(const HWND hwnd, const uint32 width, const uint32 height)
	{
		kWidth = width;
		kHeight = height;

		if (initialize_createDeviceAndCommandQueueAndSwapChain(hwnd, width, height) == false) 
			return false;

		if(createCommandList(_commandList) == false)
			return false;

		if (initialize_createFence() == false) 
			return false;

		execute();
		waitFenceAndResetCommandList();

		return true;
	}

	void RenderModule::destroy()
	{
		_renderPassMap.clear();
		_textureDescriptorHeap.release();
		DK_ASSERT_LOG(_textureContainer.empty(), "");

#if defined(USE_IMGUI)
		_imguiDescriptorHeap.release();
#endif

		for (uint32 i = 0; i < DK_COUNT_OF(_depthStencilTextureArr); ++i)
			_depthStencilTextureArr[i].reset();
		_depthStencilDescriptorHeap.release();

		for (uint32 i = 0; i < DK_COUNT_OF(_backBufferResourceArr); ++i)
			_backBufferResourceArr[i] = IBuffer();

		for (uint32 i = 0; i < DK_COUNT_OF(_renderTargetTextureArr); ++i)
			_renderTargetTextureArr[i].reset();
		_renderTargetDescriptorHeap.release();

		_swapChain.release();
		_scissorRect.release();
		_viewport.release();

		CloseHandle(_fenceEvent);
		for (uint32 i = 0; i < DK_COUNT_OF(_fences); ++i)
			_fences[i].release();

		_commandList = DKCommandList();
		_commandQueue.release();

		_device.release();
		ReportLiveObjects();
	}

	bool CheckTearingSupport(RenderResourcePtr<IDXGIFactory4>& factory)
	{
		HRESULT hr;
		BOOL allowTearing = FALSE;
		{
			IDXGIFactory5* factory5;
			hr = factory->QueryInterface(IID_PPV_ARGS(&factory5));
			if (SUCCEEDED(hr))
				hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		}
		return SUCCEEDED(hr) && allowTearing;
	}
	bool RenderModule::initialize_createDeviceAndCommandQueueAndSwapChain(const HWND hwnd, const uint32 width, const uint32 height)
	{
		HRESULT hr;

		// Create DirectX Factory
		RenderResourcePtr<IDXGIFactory4> factory;
		{
			UINT factoryFlags = 0;
#if defined(_DK_DEBUG_)
			factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
			IDXGIFactory4* factory4;
			hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory4));
			if (SUCCEEDED(hr) == false)
				return false;
			factory = factory4;
		}

		// Activate DebugLayer
		{
#if 0 && !defined(USE_PIX)
			//실험적 기능 목록을 사용하도록 설정합니다.
			const GUID D3D12ExperimentalShaderModelsID = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
				0x76f5573e,
				0xf13a,
				0x40f5,
				{ 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
			};
			hr = D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr);
			if (FAILED(hr))
				return false;
#endif

#ifdef _DK_DEBUG_
			// CreateDevice이전에 실행해야합니다. Device생성 이후에 호출하면 Device Remove가 발생함.
			// Enable the D3D12 debug layer.
			ID3D12Debug* debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				debugController->EnableDebugLayer();
			debugController->Release();
#endif
		}

		// Create RenderDevice
		{
			if (_useWarpDevice == true)
			{
				IDXGIAdapter* warpAdapter;
				if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))) == false)
					return false;
				ID3D12Device8* device;
				if (SUCCEEDED(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))) == false)
				{
					warpAdapter->Release();
					return false;
				}

				warpAdapter->Release();
				_device = device;
			}
			else
			{
				IDXGIAdapter1* hardwareAdapter;
				GetHardwareAdapter(factory, &hardwareAdapter);
				ID3D12Device8* device;
				if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))) == false)
					return false;

				hardwareAdapter->Release();
				_device = device;
			}
		}

#if defined(_DK_DEBUG_)
		_device->SetName(L"Duckingengine D3D12 Device");

		// 1. 디바이스 생성 후 Info Queue 인터페이스를 가져옵니다.
		ID3D12InfoQueue* pInfoQueue = nullptr;
		hr = _device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		if (SUCCEEDED(hr))
		{
			// 2. 에러(Error) 발생 시 중단점 활성화
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			// 3. 데이터 손상(Corruption) 발생 시 중단점 활성화
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			// (선택 사항) 경고 발생 시에도 멈추고 싶다면 아래 줄 주석 해제
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
			// 4. 수동으로 가져온 인터페이스이므로 사용 후 Release 호출
		}
		pInfoQueue->Release();
#endif

		// Find ShaderModel (현재 어디서도 쓰지 않음) (아마 컴파일 셰이더할때 쓸 수 있을듯)
		{
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
				shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;
		}

		// Create CommandQueue
		{
			D3D12_COMMAND_QUEUE_DESC cqDesc = {};
			cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			ID3D12CommandQueue* commandQueue;
			if (FAILED(_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue))))
				return false;
#if defined(_DK_DEBUG_)
			_commandQueue = commandQueue;
			_commandQueue->SetName(L"DuckingEngine CommandQueue");
#endif
		}

		// Create Bindless Texture DescriptorHeap
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = TEXTUREBINDLESS_MAX_COUNT;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.Type = gTextureBindlessDescriptorHeapType;
			ID3D12DescriptorHeap* textureDescriptorHeap;
			hr = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&textureDescriptorHeap));
			if (FAILED(hr))
				return false;

#if defined(_DK_DEBUG_)
			_textureDescriptorHeap = textureDescriptorHeap;
			_textureDescriptorHeap->SetName(L"BindlessTextureDescriptorHeap");
#endif
		}

		static constexpr const D3D12_RESOURCE_STATES kResourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		// Create Depth/Stencil View
		{
			/*
			* Depth/Stencil Reosurce/DSV/SRV 만들 때 타입 조심하자!
			* https://stackoverflow.com/questions/38933565/which-format-to-use-for-a-shader-resource-view-into-depth-stencil-buffer-resourc
			* https://stackoverflow.com/questions/20256815/how-to-check-the-content-of-the-depth-stencil-buffer
			*/
			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = gDepthStencilFormat;
			depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
			depthOptimizedClearValue.DepthStencil.Stencil = 0;

			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = DK_COUNT_OF(_renderTargetTextureArr);
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ID3D12DescriptorHeap* depthStencilDescriptorHeap;
			hr = _device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&depthStencilDescriptorHeap));
			if (FAILED(hr) == true)
				return false;

#if defined(_DK_DEBUG_)
			_depthStencilDescriptorHeap = depthStencilDescriptorHeap;
			_depthStencilDescriptorHeap->SetName(L"DSV DescriptorHeap");
#endif

			_depthStencilViewSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

			// TODO: 현재 Depth/Stencil 버퍼는 2개만 필요하다.. 근데 4개나 만들고 있다. 다음에 FrameBuffer Instance를 만들어서 관리할 수 있도록 해야겠다
			for (uint32 i = 0; i < dsvHeapDesc.NumDescriptors; ++i)
			{
#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> indexString;
				StringUtil::itoa(i, indexString.data(), indexString.capacity());
				ScopeString<DK_MAX_BUFFER> dsvTextureName("DepthStencilTexture_");
				dsvTextureName.append(indexString.c_str());
#endif
				IBufferRef buffer = createBuffer2DInternal(width, height, 1, GetDepthResourceFormat(gDepthStencilFormat), D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_HEAP_TYPE_DEFAULT, kResourceState, &depthOptimizedClearValue, StringUtil::convertCtoWC(dsvTextureName.c_str()).c_str());
				if (buffer == nullptr)
					return false;

				D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
				depthStencilViewDesc.Format = gDepthStencilFormat;
				depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
				_device->CreateDepthStencilView(buffer->_buffer, &depthStencilViewDesc, dsvHandle);
				dsvHandle.ptr += _depthStencilViewSize;

				_depthStencilTextureArr[i] = ITextureRef(dk_new ITexture(DKString(dsvTextureName.c_str()), 1, GetDepthSRVFormat(gDepthStencilFormat), DK::move(buffer)));
				allocateTextureSRV(_depthStencilTextureArr[i].get());
			}
		}

		// Create RTV descriptorHeap
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = DK_COUNT_OF(_renderTargetTextureArr) + DK_COUNT_OF(_backBufferResourceArr);
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			ID3D12DescriptorHeap* rtvHeap;
			hr = _device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
			if (FAILED(hr) == true)
				return false;

			_renderTargetViewSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			if (FAILED(hr) == true)
				return false;

#if defined(_DK_DEBUG_)
			_renderTargetDescriptorHeap = rtvHeap;
			_renderTargetDescriptorHeap->SetName(L"RTVDescriptorHeap");
#endif
		}

		// Create RTV
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		{
			D3D12_CLEAR_VALUE clearValue;
			clearValue.Format = renderTargetFormat;
			clearValue.Color[0] = gClearRenderTargetViewColor.x;
			clearValue.Color[1] = gClearRenderTargetViewColor.y;
			clearValue.Color[2] = gClearRenderTargetViewColor.z;
			clearValue.Color[3] = gClearRenderTargetViewColor.w;

			for (uint32 i = 0; i < DK_COUNT_OF(_renderTargetTextureArr); ++i)
			{
#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> indexString;
				StringUtil::itoa(i, indexString.data(), indexString.capacity());
				ScopeString<DK_MAX_BUFFER> rtvTextureName("RenderTargetTexture_");
				rtvTextureName.append(indexString.c_str());
#endif
				IBufferRef buffer = createBuffer2DInternal(width, height, 1, renderTargetFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_HEAP_TYPE_DEFAULT, kResourceState, &clearValue, StringUtil::convertCtoWC(rtvTextureName.c_str()).c_str());
				if (buffer == nullptr)
					return false;

				_device->CreateRenderTargetView(buffer->_buffer, nullptr, rtvHandle);
				rtvHandle.ptr += _renderTargetViewSize;

				_renderTargetTextureArr[i] = ITextureRef(dk_new ITexture(DKString(rtvTextureName.c_str()), 1, renderTargetFormat, DK::move(buffer)));
				allocateTextureSRV(_renderTargetTextureArr[i].get());
			}
		}

		// Create SwapChain and BackBuffer RTV
		{
			DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.Width = width;
			swapChainDesc.Height = height;
			swapChainDesc.Format = renderTargetFormat;
			swapChainDesc.Stereo = FALSE;
			swapChainDesc.SampleDesc = sampleDesc;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = kFrameCount;
			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			// It is recommended to always allow tearing if tearing support is available.
			swapChainDesc.Flags = CheckTearingSupport(factory) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
			swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			IDXGISwapChain4* swapChain;
			hr = factory->CreateSwapChainForHwnd(_commandQueue.get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
			if (FAILED(hr) == true)
				return false;
			_swapChain = swapChain;

			kCurrentFrameIndex = 0;
			kCurrentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();

			for (uint32 i = 0; i < kFrameCount; ++i)
			{
				ID3D12Resource* backBuffer;
				hr = _swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
				if (FAILED(hr) == true)
					return false;

#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> indexString;
				StringUtil::itoa(i, indexString.data(), indexString.capacity());
				ScopeStringW<DK_MAX_BUFFER> resourceName(L"BackBufferResource_");
				resourceName.append(StringUtil::convertCtoWC(indexString.c_str()).c_str());
				backBuffer->SetName(resourceName.c_str());
#endif

				const D3D12_RESOURCE_DESC desc = backBuffer->GetDesc();
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
				UINT64 sizeInBytes = 0;
				_device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &sizeInBytes);

				_backBufferResourceArr[i] = IBuffer(IBuffer::Type::BACKBUFFER, DK::move(backBuffer), sizeInBytes, D3D12_RESOURCE_STATE_PRESENT);

				_device->CreateRenderTargetView(_backBufferResourceArr[i]._buffer, nullptr, rtvHandle);
				rtvHandle.ptr += _renderTargetViewSize;
			}
		}

		// CreateSwapChainForHwnd뒤에 호출되어야함
		// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
		// will be handled manually.
		hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		if (SUCCEEDED(hr) == false)
			return false;

		// Viewport
		{
			RECT clientRect;
			GetClientRect(hwnd, &clientRect);

			const int clientWidth = clientRect.right - clientRect.left;
			const int clientHeight = clientRect.bottom - clientRect.top;

			_viewport = dk_new D3D12_VIEWPORT;
			_viewport->TopLeftX = 0;
			_viewport->TopLeftY = 0;
			_viewport->Width = static_cast<FLOAT>(clientWidth);
			_viewport->Height = static_cast<FLOAT>(clientHeight);
			_viewport->MinDepth = 0.0f;
			_viewport->MaxDepth = 1.0f;

			_scissorRect = dk_new D3D12_RECT;
			_scissorRect->left = 0;
			_scissorRect->top = 0;
			_scissorRect->right = static_cast<LONG>(clientWidth);
			_scissorRect->bottom = static_cast<LONG>(clientHeight);
		}

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
			desc.Type = gTextureBindlessDescriptorHeapType;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ID3D12DescriptorHeap* imguiHeap;
			if (_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiHeap)) != S_OK)
			{
				DK_ASSERT_LOG(false, "");
				return false;
			}
			_imguiDescriptorHeap = imguiHeap;

			ImGui_ImplWin32_Init(hwnd);
			ImGui_ImplDX12_Init(
				_device.get(), kFrameCount, DXGI_FORMAT_R8G8B8A8_UNORM, _imguiDescriptorHeap.get(),
				_imguiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), _imguiDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
			);
		}

		assert(io.BackendPlatformName != nullptr);
		DK_LOG("Backend: %s", io.BackendPlatformName);
		DK_LOG("BackendFlags: %d", io.BackendFlags);
		DK_LOG("DisplayFramebufferScale: %f, %f", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
#endif // USE_IMGUI

		return true;
	}
	const bool RenderModule::createCommandList(DKCommandList& outCommandList)
	{
		HRESULT hr;

		bool successAllocate = true;
		ID3D12CommandAllocator* commandAllocator[RenderModule::kFrameCount] = { nullptr, };
		for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
		{
			hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
			if (FAILED(hr))
			{
				successAllocate = false;
				break;
			}

#if defined(_DK_DEBUG_)
			commandAllocator[i]->SetName(L"CommandAllocator");
#endif
		}

		if (successAllocate == false)
		{
			for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
			{
				if(commandAllocator[i] != nullptr)
					commandAllocator[i]->Release();
			}
		}

		ID3D12GraphicsCommandList4* commandList = nullptr;
		hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], NULL, IID_PPV_ARGS(&commandList));
		if (FAILED(hr))
			return false;

		outCommandList = DKCommandList(DK::move(commandAllocator), DK::move(commandList));

#if defined(_DK_DEBUG_)
		outCommandList->SetName(L"CommandList");
#endif

		return true;
	}
	bool RenderModule::initialize_createFence()
	{
		for (uint32 i = 0; i < kFrameCount; ++i)
		{
			ID3D12Fence* fence;
			HRESULT hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
			if (FAILED(hr))
				return false;

			_fences[i] = fence;
#if defined(_DK_DEBUG_)
			_fences[i]->SetName(L"Fence");
#endif
		}

		_fenceEvent = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		return true;

		return true;
	}

	bool RenderModule::createRootSignature(const Pipeline::CreateInfo& createInfo, const DKVector<ShaderResourceReflection>& resources, Pipeline& inoutPipeline)
	{
		const bool isRaytracing = createInfo._raygenShaderPath.empty() == false;

		DKVector<D3D12_ROOT_PARAMETER> rootParameters;
		rootParameters.resize(2); // 기존 bindless SRV/UAV table

		uint32 rootParameterIndex = 0;

		// Bindless Texture 2D Table
		// For SRV
		DKVector<D3D12_DESCRIPTOR_RANGE> srvDescriptorTableRanges;
		srvDescriptorTableRanges.resize(isRaytracing ? 4 : 1);
		srvDescriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvDescriptorTableRanges[0].NumDescriptors = kMaxTextureSRVCount;// TEXTUREBINDLESS_MAX_COUNT;
		srvDescriptorTableRanges[0].RegisterSpace = BINDLESSTEXTUREARRAY_SPACE;
		srvDescriptorTableRanges[0].BaseShaderRegister = 0;
		srvDescriptorTableRanges[0].OffsetInDescriptorsFromTableStart = 0;

		if (isRaytracing)
		{
			srvDescriptorTableRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			srvDescriptorTableRanges[1].NumDescriptors = RaytracingRenderer::kRaytracingDescriptorCount;
			srvDescriptorTableRanges[1].RegisterSpace = RaytracingRenderer::BINDLESSVERTEXARRAY_SPACE;
			srvDescriptorTableRanges[1].BaseShaderRegister = 0;
			srvDescriptorTableRanges[1].OffsetInDescriptorsFromTableStart = kMaxTextureSRVCount + kMaxTextureUAVCount;

			srvDescriptorTableRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			srvDescriptorTableRanges[2].NumDescriptors = RaytracingRenderer::kRaytracingDescriptorCount;
			srvDescriptorTableRanges[2].RegisterSpace = RaytracingRenderer::BINDLESSINDEXARRAY_SPACE;
			srvDescriptorTableRanges[2].BaseShaderRegister = 0;
			srvDescriptorTableRanges[2].OffsetInDescriptorsFromTableStart = kMaxTextureSRVCount + kMaxTextureUAVCount + RaytracingRenderer::kRaytracingDescriptorCount; //D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND

			srvDescriptorTableRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			srvDescriptorTableRanges[3].NumDescriptors = RaytracingRenderer::kRaytracingDescriptorCount;
			srvDescriptorTableRanges[3].RegisterSpace = RaytracingRenderer::BINDLESSMATERIALARRAY_SPACE;
			srvDescriptorTableRanges[3].BaseShaderRegister = 0;
			srvDescriptorTableRanges[3].OffsetInDescriptorsFromTableStart = kMaxTextureSRVCount + kMaxTextureUAVCount + RaytracingRenderer::kRaytracingDescriptorCount * 2; //D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		}

		D3D12_ROOT_DESCRIPTOR_TABLE srvDescriptorTable;
		srvDescriptorTable.NumDescriptorRanges = srvDescriptorTableRanges.size();
		srvDescriptorTable.pDescriptorRanges = srvDescriptorTableRanges.data();

		rootParameters[rootParameterIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[rootParameterIndex].DescriptorTable = srvDescriptorTable;
		rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		++rootParameterIndex;

		// For UAV
		D3D12_DESCRIPTOR_RANGE  uavDescriptorTableRanges[1];
		uavDescriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavDescriptorTableRanges[0].NumDescriptors = kMaxTextureUAVCount;// TEXTUREBINDLESS_MAX_COUNT;
		uavDescriptorTableRanges[0].RegisterSpace = BINDLESSTEXTUREARRAY_SPACE;
		uavDescriptorTableRanges[0].BaseShaderRegister = 0;
		uavDescriptorTableRanges[0].OffsetInDescriptorsFromTableStart = kMaxTextureSRVCount;
		D3D12_ROOT_DESCRIPTOR_TABLE uavDescriptorTable;
		uavDescriptorTable.NumDescriptorRanges = DK_COUNT_OF(uavDescriptorTableRanges);
		uavDescriptorTable.pDescriptorRanges = &uavDescriptorTableRanges[0];

		rootParameters[rootParameterIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[rootParameterIndex].DescriptorTable = uavDescriptorTable;
		rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		++rootParameterIndex;

		for(const ShaderResourceReflection& resource : resources)
		{
			// check bindless
			if (resource._bindCount == 0 && (resource._space == BINDLESSTEXTUREARRAY_SPACE || resource._space == RaytracingRenderer::BINDLESSVERTEXARRAY_SPACE || resource._space == RaytracingRenderer::BINDLESSINDEXARRAY_SPACE || resource._space == RaytracingRenderer::BINDLESSMATERIALARRAY_SPACE)
				|| resource._type == D3D_SIT_SAMPLER)
				continue;

			bool useRootConstants = false;
			for (const RootConstant32BitParameter& setting : createInfo._rootConstant32BitParameter)
			{
				if (setting._bufferName == resource._name)
				{
					useRootConstants = true;
					break;
				}
			}

			if (useRootConstants)
			{
				const uint32 rootIndex = static_cast<uint32>(rootParameters.size());
				const uint32 dwordCount = (resource._constantBufferSize + 3) / 4;
				CD3DX12_ROOT_PARAMETER root;
				root.InitAsConstants(dwordCount, resource._register, resource._space, D3D12_SHADER_VISIBILITY_ALL);
				rootParameters.push_back(root);

				DKVector<char> buffer;
				buffer.resize(dwordCount * 4, 0);

				inoutPipeline._rootConstant32BitParameterBuffer.push_back(DK::move(buffer));
				void* storage = inoutPipeline._rootConstant32BitParameterBuffer.back().data();
				for (const auto& variable : resource._variables)
				{
					RootConstant32BitParameterBindingInfo info;
					info._rootParameterIndex = rootIndex;
					info._offset = variable._offset;
					info._buffer = storage;

					inoutPipeline._rootConstant32BitParameterMap.insert(DKPair<DKString, RootConstant32BitParameterBindingInfo>(variable._name, DK::move(info)));
				}
			}
			else
			{
				//typedef enum _D3D_SHADER_INPUT_TYPE {
				//	D3D_SIT_CBUFFER = 0,
				//	D3D_SIT_TBUFFER,
				//	D3D_SIT_TEXTURE,
				//	D3D_SIT_SAMPLER,
				//	D3D_SIT_UAV_RWTYPED,
				//	D3D_SIT_STRUCTURED,
				//	D3D_SIT_UAV_RWSTRUCTURED,
				//	D3D_SIT_BYTEADDRESS,
				//	D3D_SIT_UAV_RWBYTEADDRESS,
				//	D3D_SIT_UAV_APPEND_STRUCTURED,
				//	D3D_SIT_UAV_CONSUME_STRUCTURED,
				//	D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER,
				//	D3D_SIT_RTACCELERATIONSTRUCTURE,
				//	D3D_SIT_UAV_FEEDBACKTEXTURE,
				//	D3D10_SIT_CBUFFER,
				//	D3D10_SIT_TBUFFER,
				//	D3D10_SIT_TEXTURE,
				//	D3D10_SIT_SAMPLER,
				//	D3D11_SIT_UAV_RWTYPED,
				//	D3D11_SIT_STRUCTURED,
				//	D3D11_SIT_UAV_RWSTRUCTURED,
				//	D3D11_SIT_BYTEADDRESS,
				//	D3D11_SIT_UAV_RWBYTEADDRESS,
				//	D3D11_SIT_UAV_APPEND_STRUCTURED,
				//	D3D11_SIT_UAV_CONSUME_STRUCTURED,
				//	D3D11_SIT_UAV_RWSTRUCTURED_WITH_COUNTER
				//} D3D_SHADER_INPUT_TYPE;
				switch (resource._type)
				{
				case D3D_SIT_CBUFFER:
				case D3D_SIT_STRUCTURED:
				case D3D_SIT_RTACCELERATIONSTRUCTURE:
				{
					const uint32 rootIndex = static_cast<uint32>(rootParameters.size());

					CD3DX12_ROOT_PARAMETER root;
					if (resource._type != D3D_SIT_CBUFFER)
						root.InitAsShaderResourceView(resource._register, resource._space, D3D12_SHADER_VISIBILITY_ALL);
					else
						root.InitAsConstantBufferView(resource._register, resource._space, D3D12_SHADER_VISIBILITY_ALL);
					rootParameters.push_back(root);

					ShaderParameter parameter;
					parameter._type = ShaderParameterType::Buffer;
					parameter._register = resource._register;
					parameter._space = resource._space;
					parameter._rootParameterIndex = rootIndex;

					inoutPipeline._shaderParameterMap.insert(DKPair<DKString, ShaderParameter>(resource._name, DK::move(parameter)));
				}
				break;
				default:
				{
					DK_ASSERT_LOG(false, "");
					return false;
				}
				}
			}
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
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_STATIC_SAMPLER_DESC samplerRepeatBilinear = {};
		samplerRepeatBilinear.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerRepeatBilinear.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerRepeatBilinear.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerRepeatBilinear.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerRepeatBilinear.MipLODBias = 0;
		samplerRepeatBilinear.MaxAnisotropy = 0;
		samplerRepeatBilinear.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerRepeatBilinear.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplerRepeatBilinear.MinLOD = 0.0f;
		samplerRepeatBilinear.MaxLOD = D3D12_FLOAT32_MAX;
		samplerRepeatBilinear.ShaderRegister = 1;
		samplerRepeatBilinear.RegisterSpace = 0;
		samplerRepeatBilinear.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		DKVector<D3D12_STATIC_SAMPLER_DESC> samplers;
		samplers.push_back(sampler);
		samplers.push_back(samplerRepeatBilinear);

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.NumParameters = static_cast<uint32>(rootParameters.size());
		rootSignatureDesc.pParameters = rootParameters.data();
		rootSignatureDesc.NumStaticSamplers = samplers.size();
		rootSignatureDesc.pStaticSamplers = samplers.data();
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
			DK_ASSERT_LOG(false, "Failed serializeRootSignature");
			if (signature != nullptr)
				signature->Release();
			if (errorBuffer != nullptr)
				errorBuffer->Release();
			return false;
		}

		ID3D12RootSignature* rootSignature;
		hr = _device->CreateRootSignature(0, signature->GetBufferPointer(), static_cast<uint32>(signature->GetBufferSize()), IID_PPV_ARGS(&rootSignature));
		if (SUCCEEDED(hr) == false)
		{
			DK_ASSERT_LOG(false, "Failed craete RootSignature");
			if (signature != nullptr)
				signature->Release();
			if (errorBuffer != nullptr)
				errorBuffer->Release();
			return false;
		}

		inoutPipeline._rootSignature = rootSignature;
#if defined(_DK_DEBUG_)
		inoutPipeline._rootSignature->SetName(StringUtil::convertCtoWC(inoutPipeline._createInfo._pipelineName.c_str()).c_str());
#endif

		if (signature != nullptr)
			signature->Release();
		if (errorBuffer != nullptr)
			errorBuffer->Release();

		return true;
	}
	
	D3D12_PRIMITIVE_TOPOLOGY_TYPE convertPrimitiveTopologyType(const char* value)
	{
		static const char* primitiveTopologyTypeStr[] =
		{
			"Undefined",
			"Point",
			"Line",
			"Triangle",
			"Patch"
		};
		const uint32 strCount = DK_COUNT_OF(primitiveTopologyTypeStr);
		for (uint32 i = 0; i < strCount; ++i)
		{
			if (strcmp(value, primitiveTopologyTypeStr[i]) == 0)
				return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(i);
		}

		DK_ASSERT_LOG(false, "Ptimitive정보가 올바르지 않습니다. pipeline이 작동되지 않을테니 반드시 확인이 필요합니다.");
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
	bool RenderModule::createPipelineObjectState(ShaderCompiler& shaderCompiler, const Pipeline::CreateInfo& pipelineCreateInfo, Pipeline& inoutPipeline)
	{
		DKVector<DKString> emptyDefines;
		DKVector<ShaderResourceReflection> pipelineResources;

		if (pipelineCreateInfo._vertexShaderPath.empty() == false)
		{
			inoutPipeline._type = Pipeline::Type::GRAPHIC;

			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType = convertPrimitiveTopologyType(pipelineCreateInfo._primitiveTopologyType.c_str());
			if (primitiveType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED)
				return false;

			inoutPipeline._primitiveTopologyType = primitiveType;

			D3D12_SHADER_BYTECODE vertexShaderView = {};
			RenderResourcePtr<IDxcBlob> vertexShader = nullptr;
			bool success = shaderCompiler.compileShader(
				pipelineCreateInfo._vertexShaderPath.c_str(), pipelineCreateInfo._vertexShaderEntry.c_str(),
				ShaderType::VertexShader, emptyDefines,
				vertexShader, vertexShaderView, pipelineResources
			);
			if (success == false)
				return false;
			D3D12_SHADER_BYTECODE pixelShaderView = {};
			RenderResourcePtr<IDxcBlob> pixelShader = nullptr;
			success = shaderCompiler.compileShader(
				pipelineCreateInfo._pixelShaderPath.c_str(), pipelineCreateInfo._pixelShaderEntry.c_str(),
				ShaderType::PixelShader, emptyDefines,
				pixelShader, pixelShaderView, pipelineResources
			);
			if (success == false)
				return false;

			if (createRootSignature(pipelineCreateInfo, pipelineResources, inoutPipeline) == false)
				return false;

			DKVector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
			inputLayout.reserve(pipelineCreateInfo._layout.size());
			for (const Pipeline::CreateInfo::LayoutInfo& layoutElement : pipelineCreateInfo._layout)
			{
				DXGI_FORMAT layoutFormat;
				switch (layoutElement._type)
				{
				case Pipeline::CreateInfo::LayoutInfo::Type::UINT4:
					layoutFormat = DXGI_FORMAT_R32G32B32A32_UINT;
					break;
				case Pipeline::CreateInfo::LayoutInfo::Type::FLOAT2:
					layoutFormat = DXGI_FORMAT_R32G32_FLOAT;
					break;
				case Pipeline::CreateInfo::LayoutInfo::Type::FLOAT3:
					layoutFormat = DXGI_FORMAT_R32G32B32_FLOAT;
					break;
				case Pipeline::CreateInfo::LayoutInfo::Type::FLOAT4:
					layoutFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
					break;
				default:
					DK_ASSERT_LOG(false, "존재하지 않는 Layout세팅할려합니다.");
					break;
				}

				inputLayout.push_back({ layoutElement._name.c_str(), 0, layoutFormat, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			}

			D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
			inputLayoutDesc.NumElements = static_cast<UINT>(inputLayout.size());
			inputLayoutDesc.pInputElementDescs = inputLayout.data();

			D3D12_RASTERIZER_DESC rasterizerDesc = {};
			rasterizerDesc.FillMode = pipelineCreateInfo._fillMode == Pipeline::CreateInfo::FillMode::WIREFRAME ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
			rasterizerDesc.CullMode = pipelineCreateInfo._cullMode == Pipeline::CreateInfo::CullMode::FRONT ? D3D12_CULL_MODE_FRONT : (pipelineCreateInfo._cullMode == Pipeline::CreateInfo::CullMode::BACK ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE);
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
			depthStencilDesc.DepthEnable = pipelineCreateInfo._depthEnable;
			depthStencilDesc.DepthWriteMask = depthStencilDesc.DepthEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			depthStencilDesc.DepthFunc = depthStencilDesc.DepthEnable ? D3D12_COMPARISON_FUNC_LESS : D3D12_COMPARISON_FUNC_NEVER;
			depthStencilDesc.StencilEnable = FALSE;
			depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			depthStencilDesc.FrontFace = defaultStencilOp;
			depthStencilDesc.BackFace = defaultStencilOp;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = inputLayoutDesc;
			psoDesc.pRootSignature = inoutPipeline._rootSignature.get();
			psoDesc.VS = vertexShaderView;
			psoDesc.PS = pixelShaderView;
			psoDesc.PrimitiveTopologyType = inoutPipeline._primitiveTopologyType;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc = sampleDesc;
			psoDesc.SampleMask = 0xffffffff;
			psoDesc.RasterizerState = rasterizerDesc;
			psoDesc.BlendState = blendDesc;
			psoDesc.NumRenderTargets = 1;
			psoDesc.DepthStencilState = depthStencilDesc;
			psoDesc.DSVFormat = gDepthStencilFormat;

			ID3D12PipelineState* pso;
			HRESULT hr = _device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
			if (SUCCEEDED(hr) == false)
			{
				DK_ASSERT_LOG(false, "Pipeline State Object 생성에 실패");
				return false;
			}

			inoutPipeline._pipelineStateObject = pso;
#if defined(_DK_DEBUG_)
			inoutPipeline._createInfo = pipelineCreateInfo;
			inoutPipeline._pipelineStateObject->SetName(StringUtil::convertCtoWC(inoutPipeline._createInfo._pipelineName.c_str()).c_str());
#endif
		}
		else if(pipelineCreateInfo._computeShaderPath.empty() == false)
		{
			inoutPipeline._type = Pipeline::Type::COMPUTE;

			D3D12_SHADER_BYTECODE computeShaderView = {};
			RenderResourcePtr<IDxcBlob> computeShader = nullptr;
			uint32 threadGroupSize[3] = {};

			bool success = shaderCompiler.compileShader(
				pipelineCreateInfo._computeShaderPath.c_str(), pipelineCreateInfo._computeShaderEntry.c_str(),
				ShaderType::ComputeShader, emptyDefines,
				computeShader, computeShaderView, pipelineResources, threadGroupSize
			);
			if (success == false)
				return false;

			if (createRootSignature(pipelineCreateInfo, pipelineResources, inoutPipeline) == false)
				return false;

			D3D12_COMPUTE_PIPELINE_STATE_DESC cpsoDesc = {};
			cpsoDesc.pRootSignature = inoutPipeline._rootSignature.get();
			cpsoDesc.CS = computeShaderView;
			cpsoDesc.NodeMask = 0;

			ID3D12PipelineState* pso;
			HRESULT hr = _device->CreateComputePipelineState(&cpsoDesc, IID_PPV_ARGS(&pso));
			if (FAILED(hr))
			{
				DK_ASSERT_LOG(false, "Compute Pipeline State Object 생성 실패");
				return false;
			}

			inoutPipeline._pipelineStateObject = pso;
#if defined(_DK_DEBUG_)
			inoutPipeline._createInfo = pipelineCreateInfo;
			inoutPipeline._pipelineStateObject->SetName(StringUtil::convertCtoWC(inoutPipeline._createInfo._pipelineName.c_str()).c_str());
#endif

			for (uint32 i = 0; i < 3; ++i)
				inoutPipeline._threadGroupSize[i] = threadGroupSize[i];
		}
		else if (pipelineCreateInfo._raygenShaderPath.empty() == false)
		{
			inoutPipeline._type = Pipeline::Type::RAYTRACING;

			// 이 Scope Stack내(CreateStateObject호출까지) 유지되어야 해서 이 곳에서 Stack에 할당함
			RenderResourcePtr<IDxcBlob> raygenShaderBlob;
			RenderResourcePtr<IDxcBlob> missShaderBlob;
			RenderResourcePtr<IDxcBlob> cloesetShaderBlob;
			const DKStringW raygenEntry = StringUtil::convertCtoWC(pipelineCreateInfo._raygenEntry.c_str()).c_str();
			const DKStringW missEntry = StringUtil::convertCtoWC(pipelineCreateInfo._missEntry.c_str()).c_str();
			const DKStringW closestEntry = StringUtil::convertCtoWC(pipelineCreateInfo._closestEntry.c_str()).c_str();

			DKVector<D3D12_DXIL_LIBRARY_DESC> libraryDesc;
			DKVector<DKVector<D3D12_EXPORT_DESC>> exportDesc;
			{
				D3D12_SHADER_BYTECODE raygenShaderView = {};
				const bool success = shaderCompiler.compileShader(
					pipelineCreateInfo._raygenShaderPath.c_str(), "",
					ShaderType::Raytracing, emptyDefines,
					raygenShaderBlob, raygenShaderView, pipelineResources
				);
				if (success == false)
					return false;

				libraryDesc.push_back(D3D12_DXIL_LIBRARY_DESC());
				exportDesc.push_back(DKVector<D3D12_EXPORT_DESC>());

				libraryDesc[0].DXILLibrary.pShaderBytecode = raygenShaderView.pShaderBytecode;
				libraryDesc[0].DXILLibrary.BytecodeLength = raygenShaderView.BytecodeLength;
				libraryDesc[0].NumExports += 1;

				D3D12_EXPORT_DESC desc;
				desc.Name = raygenEntry.c_str();
				desc.ExportToRename = nullptr;
				desc.Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[0].push_back(desc);
			}

			if (pipelineCreateInfo._missShaderPath == pipelineCreateInfo._raygenShaderPath)
			{
				libraryDesc[0].NumExports += 1;

				D3D12_EXPORT_DESC desc;
				desc.Name = missEntry.c_str();
				desc.ExportToRename = nullptr;
				desc.Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[0].push_back(desc);
			}
			else
			{
				D3D12_SHADER_BYTECODE missShaderView = {};
				const bool success = shaderCompiler.compileShader(
					pipelineCreateInfo._missShaderPath.c_str(), "",
					ShaderType::Raytracing, emptyDefines,
					missShaderBlob, missShaderView, pipelineResources
				);
				if (success == false)
					return false;

				libraryDesc.push_back(D3D12_DXIL_LIBRARY_DESC());
				exportDesc.push_back(DKVector<D3D12_EXPORT_DESC>());

				libraryDesc[1].DXILLibrary.pShaderBytecode = missShaderView.pShaderBytecode;
				libraryDesc[1].DXILLibrary.BytecodeLength = missShaderView.BytecodeLength;
				libraryDesc[1].NumExports += 1;

				D3D12_EXPORT_DESC desc;
				desc.Name = missEntry.c_str();
				desc.ExportToRename = nullptr;
				desc.Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[1].push_back(desc);
			}

			if (pipelineCreateInfo._closestShaderPath == pipelineCreateInfo._raygenShaderPath)
			{
				libraryDesc[0].NumExports += 1;

				D3D12_EXPORT_DESC desc;
				desc.Name = closestEntry.c_str();
				desc.ExportToRename = nullptr;
				desc.Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[0].push_back(desc);
			}
			else if (pipelineCreateInfo._closestShaderPath == pipelineCreateInfo._missShaderPath)
			{
				libraryDesc[1].NumExports += 1;

				D3D12_EXPORT_DESC desc;
				desc.Name = closestEntry.c_str();
				desc.ExportToRename = nullptr;
				desc.Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[1].push_back(desc);
			}
			else
			{
				D3D12_SHADER_BYTECODE cloesetShaderView = {};
				const bool success = shaderCompiler.compileShader(
					pipelineCreateInfo._closestShaderPath.c_str(), "",
					ShaderType::Raytracing, emptyDefines,
					cloesetShaderBlob, cloesetShaderView, pipelineResources
				);
				if (success == false)
					return false;

				libraryDesc.push_back(D3D12_DXIL_LIBRARY_DESC());
				exportDesc.push_back(DKVector<D3D12_EXPORT_DESC>());

				libraryDesc[2].DXILLibrary.pShaderBytecode = cloesetShaderView.pShaderBytecode;
				libraryDesc[2].DXILLibrary.BytecodeLength = cloesetShaderView.BytecodeLength;
				libraryDesc[2].NumExports += 1;

				D3D12_EXPORT_DESC desc;
				desc.Name = closestEntry.c_str();
				desc.ExportToRename = nullptr;
				desc.Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[2].push_back(desc);
			}

			DK_ASSERT_LOG(libraryDesc.size() == exportDesc.size(), "");

			const uint32 libCount = libraryDesc.size();
			for (uint32 i = 0; i < libCount; ++i)
				libraryDesc[i].pExports = exportDesc[i].data();

			if (createRootSignature(pipelineCreateInfo, pipelineResources, inoutPipeline) == false)
				return false;

			// HitGroup
			D3D12_HIT_GROUP_DESC hitGroup = {};
			hitGroup.HitGroupExport = L"HitGroup";
			hitGroup.ClosestHitShaderImport = L"ClosestHit";
			hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

			// ShaderConfig
			D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
			shaderConfig.MaxPayloadSizeInBytes = sizeof(float) * 4;
			shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2;

			// PipelineConfig
			D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
			pipelineConfig.MaxTraceRecursionDepth = 1;

			// Rootsignature
			D3D12_GLOBAL_ROOT_SIGNATURE globalRoot = {};
			globalRoot.pGlobalRootSignature = inoutPipeline._rootSignature.get();

			// Subobjects
			DKVector<D3D12_STATE_SUBOBJECT> subobjects;
			subobjects.resize(libCount + 4);
			for (uint32 i = 0; i < libCount; ++i)
			{
				subobjects[i].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
				subobjects[i].pDesc = &libraryDesc[i];
			}
			subobjects[libCount + 0].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			subobjects[libCount + 0].pDesc = &hitGroup;
			subobjects[libCount + 1].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			subobjects[libCount + 1].pDesc = &shaderConfig;
			subobjects[libCount + 2].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			subobjects[libCount + 2].pDesc = &pipelineConfig;
			subobjects[libCount + 3].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			subobjects[libCount + 3].pDesc = &globalRoot;

			// StateObject
			D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
			stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
			stateObjectDesc.NumSubobjects = subobjects.size();
			stateObjectDesc.pSubobjects = subobjects.data();
			ID3D12StateObject* pso;
			HRESULT hr = _device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&pso));
			if (FAILED(hr))
			{
				DK_ASSERT_LOG(false, "Failed Raytracing CreateStateObject");
				return false;
			}
			inoutPipeline._rtStateObject = pso;

			// StateObject Properties
			ID3D12StateObjectProperties* psoProp;
			hr = inoutPipeline._rtStateObject->QueryInterface(IID_PPV_ARGS(&psoProp));
			if (FAILED(hr))
			{
				DK_ASSERT_LOG(false, "Failed Raytracing QueryInterface");
				return false;
			}
			inoutPipeline._rtStateObjectProperties = psoProp;
#if defined(_DK_DEBUG_)
			inoutPipeline._createInfo = pipelineCreateInfo;
			inoutPipeline._rtStateObject->SetName(StringUtil::convertCtoWC(inoutPipeline._createInfo._pipelineName.c_str()).c_str());
#endif
		}
		else
		{
			DK_ASSERT_LOG(false, "올바르지 않은 Pipeline Type 지정");
			return false;
		}

		return true;
	}
	bool RenderModule::createRenderPass(ShaderCompiler& shaderCompiler, const DKString& renderPassName, RenderPass::CreateInfo&& renderPassCreateInfo)
	{
		using FindResult = DKHashMap<DKString, RenderPass>::iterator;
		FindResult find = _renderPassMap.find(renderPassName);

		if (find != _renderPassMap.end())
		{
			DK_ASSERT_LOG(false, "중복된 이름의 RenderPass가 있습니다.\nName: %s", find->first.c_str());
			return false;
		}
		using InsertResult = DKPair<DKHashMap<DKString, RenderPass>::iterator, bool>;
		InsertResult insertResult = _renderPassMap.insert(DKPair<DKString, RenderPass>(renderPassName, RenderPass()));
		if (insertResult.second == false)
		{
			DK_ASSERT_LOG(false, "HashMap Insert에 실패!");
			return false;
		}

		using PipelineCreateInfoIter = DKVector<DKPair<DKString, Pipeline::CreateInfo>>;
		RenderPass& renderPass = insertResult.first->second;

		const uint32 pipelineCount = static_cast<uint32>(renderPassCreateInfo._pipelineArr.size());
		for (uint32 i = 0; i < pipelineCount; ++i)
		{
			const DKString& pipelineName = renderPassCreateInfo._pipelineArr[i].first;

			using PipelineFindResult = DKHashMap<DKString, Pipeline>::iterator;
			PipelineFindResult foundPipeline = renderPass._pipelineMap.find(pipelineName);
			if (foundPipeline != renderPass._pipelineMap.end())
			{
				DK_ASSERT_LOG(false, "중복된 이름의 Pipeline(%s)이 같은 RenderPass(%s) 내에 있습니다.", pipelineName.c_str(), renderPassName.c_str());
				continue;
			}

			Pipeline::CreateInfo& pipelineCreateInfo = renderPassCreateInfo._pipelineArr[i].second;

			Pipeline newPipeline;
			if (createPipelineObjectState(shaderCompiler, pipelineCreateInfo, newPipeline) == false)
				return false;

			renderPass._pipelineMap.insert(DKPair<DKString, Pipeline>(pipelineName, DK::move(newPipeline)));
		}

		return true;
	}
#pragma endregion

#if defined(_DK_DEBUG_)
	const bool RenderModule::reloadShader()
	{
		ShaderCompiler shaderCompiler;

		for (DKHashMap<DKString, RenderPass>::iterator iter = _renderPassMap.begin(); iter != _renderPassMap.end(); ++iter)
		{
			for (DKHashMap<DKString, Pipeline>::iterator pipelineIter = iter->second._pipelineMap.begin(); pipelineIter != iter->second._pipelineMap.end(); ++pipelineIter)
			{
				if (createPipelineObjectState(shaderCompiler, pipelineIter->second._createInfo, pipelineIter->second) == false)
					return false;
			}
		}

		DuckingEngine::getInstance().GetRaytracingRendererWritable().createShaderBindingTable(this);

		return true;
	}

	void RenderModule::ReportLiveObjects() const
	{
		// D3D12 objects
		//if (_device.get())
		//{
		//	ID3D12DebugDevice* debugDevice = nullptr;
		//	if (SUCCEEDED(const_cast<RenderResourcePtr<ID3D12Device8>&>(_device)->QueryInterface(IID_PPV_ARGS(&debugDevice))))
		//	{
		//		debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		//		debugDevice->Release();
		//	}
		//}

		// DXGI objects
		IDXGIDebug1* dxgiDebug = nullptr;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
			dxgiDebug->Release();
		}
	}
#endif

	bool isExecuted = false;
	void RenderModule::waitAllGPU()
	{
		//if (gSerializeRender)
		//{
		//	_commandList->Close();

		//	DKVector<ID3D12CommandList*> commandLists;
		//	commandLists.push_back(_commandList._commandList.get());
		//	_commandQueue->ExecuteCommandLists(static_cast<uint32>(commandLists.size()), commandLists.data());

		//	++_fenceValues[kCurrentFrameIndex];
		//	HRESULT hr = _commandQueue->Signal(_fences[kCurrentFrameIndex].get(), _fenceValues[kCurrentFrameIndex]);
		//	assert(SUCCEEDED(hr));
		//}
		//else
		//{
		//	++_fenceValues[kCurrentFrameIndex];
		//	HRESULT hr = _commandQueue->Signal(_fences[kCurrentFrameIndex].get(), _fenceValues[kCurrentFrameIndex]);
		//	assert(SUCCEEDED(hr));
		//}

		for (uint32 i = 0; i < kFrameCount; ++i)
		{
			if (static_cast<uint32>(_fences[i]->GetCompletedValue()) < _fenceValues[i])
			{
				_fences[i]->SetEventOnCompletion(_fenceValues[i], _fenceEvent);
				WaitForSingleObject(_fenceEvent, INFINITE);
			}
		}
	}

	void RenderModule::execute(const bool isPresent)
	{
		_commandList->Close();

		DKVector<ID3D12CommandList*> commandLists;
		commandLists.push_back(_commandList._commandList.get());
		_commandQueue->ExecuteCommandLists(static_cast<uint32>(commandLists.size()), commandLists.data());

		if (isPresent)
		{
			UINT syncInterval = 0; //gVSync ? 1 : 0;
			UINT presentFlags = 0; // CheckTearingSupport() && !gVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
			HRESULT hr = _swapChain->Present(syncInterval, presentFlags);
			DK_ASSERT_LOG(SUCCEEDED(hr), "Present 실패!");
			kCurrentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
		}

		++_fenceValues[kCurrentFrameIndex];
		HRESULT hr = _commandQueue->Signal(_fences[kCurrentFrameIndex].get(), _fenceValues[kCurrentFrameIndex]);
		assert(SUCCEEDED(hr));

		isExecuted = true;
	}
	void RenderModule::waitFenceAndResetCommandList()
	{
		if (isExecuted == false)
			return;
		isExecuted = false;

		kCurrentFrameIndex = 0;

		const uint32 completeFenceValue = static_cast<uint32>(_fences[kCurrentFrameIndex]->GetCompletedValue());
		if (completeFenceValue < _fenceValues[kCurrentFrameIndex])
		{
			HRESULT hr = _fences[kCurrentFrameIndex]->SetEventOnCompletion(_fenceValues[kCurrentFrameIndex], _fenceEvent);
			assert(SUCCEEDED(hr));
			WaitForSingleObject(_fenceEvent, INFINITE);
		}

		_commandList._commandAllocators[kCurrentFrameIndex]->Reset();
		_commandList->Reset(_commandList._commandAllocators[kCurrentFrameIndex].get(), nullptr);
	}

	void RenderModule::resourceBarrierTransition(IBufferRef& buffer, const D3D12_RESOURCE_STATES afterState)
	{
		if (buffer == nullptr)
			return;

		resourceBarrierTransition(*buffer.get(), afterState);
	}
	void RenderModule::resourceBarrierTransition(IBuffer& buffer, const D3D12_RESOURCE_STATES afterState)
	{
		if (buffer._currentState == afterState)
			return;

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(const_cast<ID3D12Resource*>(buffer._buffer.get()), buffer._currentState, afterState);
		_commandList->ResourceBarrier(1, &barrier);

		buffer._currentState = afterState;
	}
	void RenderModule::copyResource(const IBufferRef& targetBuffer, const IBufferRef& sourceBuffer, const D3D12_RESOURCE_STATES afterState)
	{
		DK_ASSERT_LOG(_blockCopy == false, "");
		DK_ASSERT_LOG(targetBuffer->_type == IBuffer::Type::DEFAULT, "Destination Buffer는 반드시 Default여야합니다.");
		DKCommandList::CopyResourcePendingData pendingData;
		pendingData._source = sourceBuffer;
		pendingData._target = targetBuffer;
		pendingData._afterState = afterState;
		_commandList._copyResourcePendingArray.push_back(DK::move(pendingData));
	}

	RenderResourcePtr<ID3D12Resource> createBufferInternalRaw(RenderResourcePtr<ID3D12Device8>& device, const uint32 size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state, const wchar_t* debugName)
	{
		ID3D12Resource* buffer = nullptr;
		CD3DX12_HEAP_PROPERTIES props(type);
		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
		HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&buffer));
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "CreateBuffer Failed!\nBufferName: %s", StringUtil::convertWCtoC(debugName).c_str());
			return nullptr;
		}
#if defined(_DK_DEBUG_)
		buffer->SetName(debugName);
#endif

		return RenderResourcePtr<ID3D12Resource>(buffer);
	}
	IBufferRef RenderModule::createBuffer2DInternal(const uint32 width, const uint32 height, const uint32 mipLevelCount, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue, const wchar_t* debugName)
	{
		ID3D12Resource* buffer;
		const CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, mipLevelCount, 1, 0, flags);
		const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
		HRESULT hr = _device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &textureDesc, state, clearValue, IID_PPV_ARGS(&buffer));
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "Texture creation failed");
			return nullptr;
		}
#if defined(_DK_DEBUG_)
		buffer->SetName(debugName);
#endif

		return IBufferRef(dk_new IBuffer(RenderResourcePtr<ID3D12Resource>(buffer), width, height, 8 * 4, mipLevelCount, state));
	}

	IBufferRef RenderModule::createConstantBuffer(const uint32 size, const wchar_t* debugName)
	{
		const uint32 alignedSize = (size + 255) & ~255;
		RenderResourcePtr<ID3D12Resource> buffer = createBufferInternalRaw(_device, alignedSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, debugName);
		return IBufferRef(dk_new IBuffer(IBuffer::Type::UPLOAD, DK::move(buffer), size, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	IBufferRef RenderModule::createUploadBuffer(const uint32 size, const wchar_t* debugName)
	{
		RenderResourcePtr<ID3D12Resource> buffer = createBufferInternalRaw(_device, size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, debugName);
		return IBufferRef(dk_new IBuffer(IBuffer::Type::UPLOAD, DK::move(buffer), size, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	IBufferRef RenderModule::createDefaultBuffer(const void* data, const uint32 bufferSize, const D3D12_RESOURCE_STATES state, const wchar_t* debugName)
	{
		DK_ASSERT_LOG(data != nullptr, "DefaultBuffer 생성시에는 반드시 Data가 있어야합니다.");

		RenderResourcePtr<ID3D12Resource> buffer = createBufferInternalRaw(_device, bufferSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, debugName);
		IBufferRef defaultBuffer(dk_new IBuffer(IBuffer::Type::DEFAULT, DK::move(buffer), bufferSize, state));
		if (defaultBuffer == nullptr)
			return nullptr;

		if (data != nullptr)
		{
			IBufferRef uploadBuffer = createUploadBuffer(bufferSize, debugName);
			if (uploadBuffer == nullptr)
				return nullptr;

			uploadBuffer->upload(data);
			copyResource(defaultBuffer, uploadBuffer, state);
		}

		return DK::move(defaultBuffer);
	}

	IBufferRef RenderModule::createVertexBuffer(const void* data, const uint32 strideSize, const uint32 vertexCount, VertexBufferViewRef& outView, const wchar_t* debugName)
	{
		uint32 bufferSizeInBytes = strideSize * vertexCount;
		IBufferRef outBuffer = createDefaultBuffer(data, bufferSizeInBytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, debugName);
		if (outBuffer == nullptr)
			return nullptr;

		D3D12_VERTEX_BUFFER_VIEW view;
		view.BufferLocation = outBuffer->_buffer->GetGPUVirtualAddress();
		view.StrideInBytes = strideSize;
		view.SizeInBytes = bufferSizeInBytes;
		outView = std::make_shared<D3D12_VERTEX_BUFFER_VIEW>(view);

		return DK::move(outBuffer);
	}
	IBufferRef RenderModule::createIndexBuffer(const uint32* data, const uint32 indexCount, IndexBufferViewRef& outView, const wchar_t* debugName)
	{
		uint32 bufferSizeInBytes = sizeof(uint32) * indexCount;
		IBufferRef outBuffer = createDefaultBuffer(data, bufferSizeInBytes, D3D12_RESOURCE_STATE_INDEX_BUFFER, debugName);
		if (outBuffer == nullptr)
			return nullptr;

		D3D12_INDEX_BUFFER_VIEW view;
		view.BufferLocation = outBuffer->_buffer->GetGPUVirtualAddress();
		view.Format = DXGI_FORMAT_R32_UINT;
		view.SizeInBytes = bufferSizeInBytes;
		outView = std::make_shared<D3D12_INDEX_BUFFER_VIEW>(view);

		return DK::move(outBuffer);
	}

	const bool RenderModule::allocateTextureSRV(ITexture* texture)
	{
		EnsureMainThread();

		uint32 index;
		if (_deletedTextureSRVArr.empty() == false)
		{
			index = _deletedTextureSRVArr.back();
			_deletedTextureSRVArr.pop_back();
		}
		else
		{
			index = _currentTextureSRV++;
		}

		DK_ASSERT_LOG(index < kMaxTextureSRVCount, "TextureSRV의 최대 개수를 초과했습니다. TextureSRV를 더 이상 할당할 수 없습니다.");
		DK_ASSERT_LOG(index < TEXTUREBINDLESS_MAX_COUNT, "TextureSRV의 최대 개수를 초과했습니다. TextureSRV를 더 이상 할당할 수 없습니다.");

		D3D12_CPU_DESCRIPTOR_HANDLE textureDescriptorHeapHandle = _textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texture->getFormat();
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texture->getMipLevelCount();
		textureDescriptorHeapHandle.ptr += index * _device->GetDescriptorHandleIncrementSize(gTextureBindlessDescriptorHeapType);
		_device->CreateShaderResourceView(texture->_textureBuffer._buffer.get(), &srvDesc, textureDescriptorHeapHandle);

		texture->_textureSRVIndex = index;

		return true;
	}
	const bool RenderModule::allocateTextureUAV(ITexture* texture)
	{
		EnsureMainThread();
		DK_ASSERT_LOG(texture->getMipLevelCount() == 1, "UAV로 접근할 Texture는 MipLevel이 1개인 경우에만 허용됩니다.");

		uint32 index;
		if (_deletedTextureUAVArr.empty() == false)
		{
			index = _deletedTextureUAVArr.back();
			_deletedTextureUAVArr.pop_back();
		}
		else
		{
			index = _currentTextureUAV++;
		}

		DK_ASSERT_LOG(index < kMaxTextureUAVCount, "TextureUAV의 최대 개수를 초과했습니다. TextureUAV를 더 이상 할당할 수 없습니다.");
		DK_ASSERT_LOG(index < TEXTUREBINDLESS_MAX_COUNT, "TextureUAV의 최대 개수를 초과했습니다. TextureUAV를 더 이상 할당할 수 없습니다.");

		if (_bindlessViewSize == 0)
			_bindlessViewSize = _device->GetDescriptorHandleIncrementSize(gTextureBindlessDescriptorHeapType);

		D3D12_CPU_DESCRIPTOR_HANDLE textureDescriptorHeapHandle = _textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = texture->getFormat();
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0; // UAV로 접근할 Mip 레벨 지정
		textureDescriptorHeapHandle.ptr += (index + kMaxTextureSRVCount) * _bindlessViewSize;
		_device->CreateUnorderedAccessView(texture->_textureBuffer._buffer.get(), nullptr, &uavDesc, textureDescriptorHeapHandle);

		texture->_textureUAVIndex = index;

		return true;
	}
	class TextureRaw
	{
	public:
		uint32 _width;
		uint32 _height;
		uint32 _bitsPerPixel;
		byte* _data;
		DXGI_FORMAT _format;
	};
	bool loadImageDataFromFile(const char* fileName, _OUT_ TextureRaw& textureRaw)
	{
		HRESULT hr;

		static IWICImagingFactory* wicFactory;

		IWICBitmapDecoder* wicDecoder = NULL;
		IWICBitmapFrameDecode* wicFrame = NULL;
		IWICFormatConverter* wicConverter = NULL;

		bool imageConverted = false;

		if (wicFactory == NULL)
		{
			CoInitialize(NULL);

			hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
			if (FAILED(hr))
				return false;
		}

		const DKStringW wTexturePath = StringUtil::convertCtoWC(fileName);
		hr = wicFactory->CreateDecoderFromFilename(
			wTexturePath.c_str(),            // Image we want to load in
			NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
			GENERIC_READ,                    // We want to read from this file
			WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
			&wicDecoder                      // the wic decoder to be created
		);
		if (FAILED(hr)) return false;

		hr = wicDecoder->GetFrame(0, &wicFrame);
		if (FAILED(hr)) return false;

		WICPixelFormatGUID pixelFormat;
		hr = wicFrame->GetPixelFormat(&pixelFormat);
		if (FAILED(hr)) return false;

		// get size of image
		hr = wicFrame->GetSize(&textureRaw._width, &textureRaw._height);
		if (FAILED(hr)) return false;

		DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

		if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

			if (convertToPixelFormat == GUID_WICPixelFormatDontCare) return false;

			dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

			hr = wicFactory->CreateFormatConverter(&wicConverter);
			if (FAILED(hr)) return false;

			BOOL canConvert = FALSE;
			hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
			if (FAILED(hr) || !canConvert) return false;

			hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
			if (FAILED(hr)) return false;

			imageConverted = true;
		}

		textureRaw._bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // number of bits per pixel
		uint32 bytesPerRow = (textureRaw._width * textureRaw._bitsPerPixel) / 8; // number of bytes in each row of the image data
		uint32 imageSize = bytesPerRow * textureRaw._height; // total image size in bytes

		textureRaw._data = (BYTE*)malloc(imageSize);
		textureRaw._format = dxgiFormat;

		if (imageConverted)
		{
			hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, textureRaw._data);
			if (FAILED(hr))
			{
				dk_delete_array(textureRaw._data);
				return false;
			}
		}
		else
		{
			hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, textureRaw._data);
			if (FAILED(hr))
			{
				dk_delete_array(textureRaw._data);
				return false;
			}
		}

		return true;
	}
	UINT BitsPerPixel(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			return 32;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return 16;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
			return 8;

			// 4 bits / pixel average.
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

			// 8 bits / pixel average.
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;

		default:
			return 0;
		}
	}
	void GetSurfaceInfo(UINT width, UINT height, DXGI_FORMAT format, UINT64& outRowPitch, UINT64& outSlicePitch, UINT& outRowCount)
	{
		switch (format)
		{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		{
			const UINT64 blocksWide = std::max<UINT64>(1, (width + 3) / 4);
			const UINT64 blocksHigh = std::max<UINT64>(1, (height + 3) / 4);

			outRowPitch = blocksWide * 8;
			outRowCount = static_cast<UINT>(blocksHigh);
			outSlicePitch = outRowPitch * blocksHigh;
			return;
		}

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
		{
			const UINT64 blocksWide = std::max<UINT64>(1, (width + 3) / 4);
			const UINT64 blocksHigh = std::max<UINT64>(1, (height + 3) / 4);

			outRowPitch = blocksWide * 16;
			outRowCount = static_cast<UINT>(blocksHigh);
			outSlicePitch = outRowPitch * blocksHigh;
			return;
		}

		default:
		{
			const UINT bpp = BitsPerPixel(format);
			DK_ASSERT_LOG(bpp != 0, "Unsupported DXGI format");

			outRowPitch = (static_cast<UINT64>(width) * bpp + 7) / 8;
			outRowCount = height;
			outSlicePitch = outRowPitch * height;
			return;
		}
		}
	}
	ITextureRef RenderModule::createTexture(const DKString& path, const uint32 width, const uint32 height, const byte* data, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES state, const bool createSRV, const bool createUAV)
	{
		if(data != nullptr)
		{
			UINT64 rowPitch;
			UINT64 slicePitch;
			UINT rowCount;

			GetSurfaceInfo(width, height, format, rowPitch, slicePitch, rowCount);

			D3D12_SUBRESOURCE_DATA mipData = {};
			mipData.pData = data;
			mipData.RowPitch = static_cast<LONG_PTR>(rowPitch);
			mipData.SlicePitch = static_cast<LONG_PTR>(slicePitch);

			return createTexture(path, width, height, &mipData, 1, format, flags, state, createSRV, createUAV);
		}

		return createTexture(path, width, height, (D3D12_SUBRESOURCE_DATA*)nullptr, 1, format, flags, state, createSRV, createUAV);
	}
	ITextureRef RenderModule::createTexture(const DKString& path, const uint32 width, const uint32 height, const D3D12_SUBRESOURCE_DATA* initialData, const uint8 mipLevelCount, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES state, const bool createSRV, const bool createUAV)
	{
		if (width == 0 || height == 0 || mipLevelCount == 0)
		{
			DK_ASSERT_LOG(false, "Invalid texture dimensions or mip count");
			return nullptr;
		}

		// Explicit mip counts only; 0 (automatic full-chain allocation) is not accepted.
		uint32 maxMipCount = 1;
		for (uint32 size = width > height ? width : height; size > 1; size >>= 1)
			++maxMipCount;

		if (mipLevelCount > maxMipCount)
		{
			DK_ASSERT_LOG(false, "Mip count exceeds texture dimensions");
			return nullptr;
		}

		if ((createSRV && (flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) || (createUAV && !(flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)))
		{
			DK_ASSERT_LOG(false, "Texture flags do not allow requested views");
			return nullptr;
		}

		const D3D12_RESOURCE_STATES initialState = initialData ? D3D12_RESOURCE_STATE_COPY_DEST : state;

#if defined(_DK_DEBUG_)
		ScopeStringW<DK_MAX_PATH> textureName(L"Texture(");
		textureName.append(StringUtil::convertCtoWC(path.c_str()).c_str());
		textureName.append(L")");
#endif
		IBufferRef textureResource = createBuffer2DInternal(width, height, mipLevelCount, format, flags, D3D12_HEAP_TYPE_DEFAULT, initialState, nullptr, textureName.c_str());
		if (textureResource == nullptr)
			return nullptr;

		if (initialData != nullptr)
		{
			D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {};
			formatInfo.Format = format;
			HRESULT hr = _device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(formatInfo));
			if (FAILED(hr) || formatInfo.PlaneCount != 1)
			{
				DK_ASSERT_LOG(false, "Texture upload requires a single-plane format");
				return nullptr;
			}

			const UINT subresourceCount = mipLevelCount;

			DKVector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(subresourceCount);
			DKVector<UINT> rowCounts(subresourceCount);
			DKVector<UINT64> rowSizes(subresourceCount);
			UINT64 uploadSize = 0;
			const CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, mipLevelCount, 1, 0, flags);
			_device->GetCopyableFootprints(&textureDesc, 0, subresourceCount, 0, layouts.data(), rowCounts.data(), rowSizes.data(), &uploadSize);
			if (uploadSize == 0 || uploadSize == UINT64_MAX || uploadSize > static_cast<UINT64>(SIZE_MAX))
			{
				DK_ASSERT_LOG(false, "Invalid texture upload size");
				return nullptr;
			}

			// Validate the CPU source layout for each mip.
			// Source RowPitch may include padding; it need not be 256-byte aligned.
			for (UINT mip = 0; mip < subresourceCount; ++mip)
			{
				const D3D12_SUBRESOURCE_DATA& source = initialData[mip];
				if (source.pData == nullptr || source.RowPitch <= 0 || source.SlicePitch <= 0 || rowCounts[mip] == 0 || rowSizes[mip] == 0)
				{
					DK_ASSERT_LOG(false, "Invalid mip source data");
					return nullptr;
				}

				const UINT64 sourceRowPitch = static_cast<UINT64>(source.RowPitch);
				const UINT64 sourceSlicePitch = static_cast<UINT64>(source.SlicePitch);
				if (sourceRowPitch < rowSizes[mip])
				{
					DK_ASSERT_LOG(false, "Mip RowPitch is too small");
					return nullptr;
				}

				// Minimum readable bytes: padding is unnecessary after the last row.
				const UINT64 precedingRows = rowCounts[mip] - 1;
				if (precedingRows > (static_cast<UINT64>(INTPTR_MAX) - rowSizes[mip]) / sourceRowPitch)
				{
					DK_ASSERT_LOG(false, "Mip source size overflow");
					return nullptr;
				}

				const UINT64 requiredSourceSize = precedingRows * sourceRowPitch + rowSizes[mip];
				if (sourceSlicePitch < requiredSourceSize)
				{
					DK_ASSERT_LOG(false, "Mip SlicePitch is too small");
					return nullptr;
				}
			}

#if defined(_DK_DEBUG_)
			ScopeStringW<DK_MAX_PATH> uploadName(L"UploadTexture(");
			uploadName.append(StringUtil::convertCtoWC(path.c_str()).c_str());
			uploadName.append(L")");
#endif
			IBufferRef uploadBuffer = createUploadBuffer(uploadSize, uploadName.c_str());
			if (uploadBuffer == nullptr)
				return nullptr;

			const UINT64 copiedSize = UpdateSubresources(
				_commandList._commandList.get(),
				textureResource->_buffer,
				uploadBuffer->_buffer,
				0,
				subresourceCount,
				uploadSize,
				layouts.data(),
				rowCounts.data(),
				rowSizes.data(),
				initialData
			);
			if (copiedSize == 0)
			{
				DK_ASSERT_LOG(false, "Texture mip upload staging failed");
				return nullptr;
			}

			if (state != D3D12_RESOURCE_STATE_COPY_DEST)
				resourceBarrierTransition(textureResource, state);

			execute();
			waitFenceAndResetCommandList();
		}

		ITexture* texture = dk_new ITexture(path, mipLevelCount, format, DK::move(textureResource));
		if (createSRV)
			allocateTextureSRV(texture);
		if (createUAV)
			allocateTextureUAV(texture);

		return ITextureRef(texture);
	}
	ITextureRef RenderModule::loadAndCreateTexture(const DKString& path)
	{
		DKHashMap<DKString, ITexture*>::iterator findResult = _textureContainer.find(path);
		if (findResult != _textureContainer.end())
		{
			return findResult->second->shared_from_this();
		}

		ScopeString<DK_MAX_PATH> textureFullPath = GlobalPath::makeResourceFullPath(path);
		TextureRaw textureRaw;
		const bool loadingTextureSuccess = loadImageDataFromFile(textureFullPath.c_str(), textureRaw);
		if (loadingTextureSuccess == false)
		{
			DK_ASSERT_LOG(false, "TextureData 로딩에 실패했습니다.");
			return nullptr;
		}

		// TODO : Miplevel이 생기면 1말고 Resource에서 가져올 수 있도록하자
		ITextureRef newTexture = createTexture(
			path, textureRaw._width, textureRaw._height, textureRaw._data, textureRaw._format, 
			D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			true, false);

#if defined(_DK_DEBUG_)
		newTexture->_inContainer = true;
#endif

		auto [iter, inserted] = _textureContainer.try_emplace(newTexture->getPath(), newTexture.get());
		if (inserted == false)
		{
			DK_ASSERT_LOG(false, "HashMap Insert실패. 해시 자체의 오류일 수 있음");
			return nullptr;
		}

		return newTexture;
	}

	void RenderModule::deleteTexture(ITexture* texture)
	{
		// 이 곳에서 getSRV(), getUAV()를 사용하면 할당되지 않은 경우 내부 어썰트가 발생해서, 여기서만 예외적으로 SRV멤버변수를 직접 접근한다.
		if (texture->_textureSRVIndex != ITexture::kErrorTextureResourceViewIndex)
			deallocateTextureSRV(texture->getSRV());
		if (texture->_textureUAVIndex != ITexture::kErrorTextureResourceViewIndex)
			deallocateTextureUAV(texture->getUAV());

		const size_t insertResult = _textureContainer.erase(texture->getPath());
		DK_ASSERT_LOG(texture->_inContainer == false || insertResult == 1, "TextureContainer에 없는 TextureSRV를 해제 시도합니다.\nPath: %s", texture->getPath().c_str());
	}

	void RenderModule::deallocateTextureSRV(const TextureResourceViewType index)
	{
		EnsureMainThread();
		DK_ASSERT_LOG(index < kMaxTextureSRVCount, "TextureSRV의 최대 개수를 초과했습니다. TextureSRV를 더 이상 할당할 수 없습니다.");
		_deletedTextureSRVArr.push_back(index);
	}
	void RenderModule::deallocateTextureUAV(const TextureResourceViewType index)
	{
		EnsureMainThread();
		DK_ASSERT_LOG(index < kMaxTextureUAVCount, "TextureUAV의 최대 개수를 초과했습니다. TextureUAV를 더 이상 할당할 수 없습니다.");
		_deletedTextureUAVArr.push_back(index);
	}

	void RenderModule::preRender()
	{
		if(gSerializeRender == false)
			waitFenceAndResetCommandList();

		_commandList._prevUploadResourcePendingArray.clear();
		_commandList._prevCopyResourcePendingArray.clear();

#if defined(_DK_DEBUG_)
		_blockUpload = true;
		_blockCopy = true;
#endif

		for (const DKCommandList::UploadResourcePendingData& pending : _commandList._uploadResourcePendingArray)
		{
			if (pending._target.get() == nullptr)
				continue;

			DK_ASSERT_LOG(pending.getData() != nullptr, "nullptr인 data를 upload요청하려고합니다.");
			DK_ASSERT_LOG(pending._target->_type == IBuffer::Type::UPLOAD && pending._target->isValid(), "Upload버퍼에 대해서만 호출 가능합니다.");

			void* address = nullptr;
			HRESULT hr = pending._target.get()->_buffer->Map(0, nullptr, &address);
			DK_ASSERT_LOG(SUCCEEDED(hr), "Map에 실패하였습니다.");
			memcpy(address, pending.getData(), pending._target->_bufferSize);
			pending._target.get()->_buffer->Unmap(0, nullptr);
		}

		for (const DKCommandList::CopyResourcePendingData& pending : _commandList._copyResourcePendingArray)
		{
			resourceBarrierTransition(*pending._source.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrierTransition(*pending._target.get(), D3D12_RESOURCE_STATE_COPY_DEST);
			_commandList->CopyResource(pending._target->_buffer.get(), pending._source->_buffer.get());
			if(pending._afterState != static_cast<D3D12_RESOURCE_STATES>(DKCommandList::CopyResourcePendingData::INVALID_AFTER_STATE))
				resourceBarrierTransition(*pending._target.get(), pending._afterState);
		}
		_commandList._prevUploadResourcePendingArray.swap(_commandList._uploadResourcePendingArray);
		_commandList._prevCopyResourcePendingArray.swap(_commandList._copyResourcePendingArray);
	}

	void RenderModule::bindRenderPass(const uint32 rtvReadSlot, const uint32 rtvSlot, const bool bindDSV, const bool clearTarget)
	{
		const bool isDeffered = rtvReadSlot == 0xFFFFFFFF && rtvSlot == 0;
		const bool isPostProcess = rtvReadSlot == 0 && rtvSlot == 1;
		const bool isGBuffer = rtvReadSlot == 1 && rtvSlot == 2;
		const bool isBackBuffer = rtvSlot == 2;

		_commandList->RSSetViewports(1, _viewport.get());
		_commandList->RSSetScissorRects(1, _scissorRect.get());
		_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 이전 renderTarget의 Shader에서 쓸 수 있게 Read로 변경
		if(isPostProcess || isGBuffer)
		{
			const uint32 rtvPrevIndex = 0 * kFrameCount + rtvReadSlot;
			resourceBarrierTransition(_renderTargetTextureArr[rtvPrevIndex]->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			// PostProcessing이면 DepthStencil도 Shader에서 쓸 수 있게 Read로 변경
			if (isPostProcess)
			{
				const uint32 rtvPrevIndex = 0 * kFrameCount + rtvReadSlot;
				resourceBarrierTransition(_depthStencilTextureArr[rtvPrevIndex]->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
		}

		// Deffered인 경우 현재프레임의 rtv를 다시 RenderTarget으로 변경
		// PostProcess, GBUffer인 경우 현재프레임의 BackBuffer 또는 RenderTarget을 Shader에서 쓸 수 있게 RenderTarget으로 변경
		if (isDeffered || isPostProcess || isGBuffer)
		{
			const uint32 rtvIndex = 0 * kFrameCount + rtvSlot;

			// 현재 Pass에 해당하는 Texture를 renderTarget으로 변경
			IBuffer& renderTargetBuffer = isBackBuffer ? _backBufferResourceArr[kCurrentBackBufferIndex] : _renderTargetTextureArr[rtvIndex]->getTextureBuffer();
			resourceBarrierTransition(renderTargetBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

			// Deffered 최초 한번 시에 렌더타겟과 뎁스스텐실을 동시에 바인딩해야해서 따로 처리
			if (isDeffered)
			{
				resourceBarrierTransition(_depthStencilTextureArr[rtvIndex]->getTextureBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				rtvHandle.ptr += _renderTargetViewSize * rtvIndex;
				D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				dsvHandle.ptr += _depthStencilViewSize * rtvIndex;
				_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

				if (clearTarget)
				{
					_commandList->ClearRenderTargetView(rtvHandle, gClearRenderTargetViewColor.m, 0, nullptr);
					_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
				}
			}
			else if (isPostProcess || isGBuffer)
			{
				const UINT rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				rtvHandle.ptr += isBackBuffer ? rtvDescriptorSize * (4 + kCurrentBackBufferIndex) : rtvDescriptorSize * rtvIndex;

				_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
				if (clearTarget)
					_commandList->ClearRenderTargetView(rtvHandle, gClearRenderTargetViewColor.m, 0, nullptr);
			}
		}
	}
	D3D_PRIMITIVE_TOPOLOGY convertPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
	{
		/*
		* D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED	= 0,
		* D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT		= 1,	D3D_PRIMITIVE_TOPOLOGY_POINTLIST = 1
		* D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE		= 2,	D3D_PRIMITIVE_TOPOLOGY_LINELIST = 2, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP = 3,
		* D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3,	D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 4
		* D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH		= 4
		*/
		switch (type)
		{
		case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
		case D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED:
		case D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH:
		default:
			DK_ASSERT_LOG(false, "지원하지 않느 Type을 지정하였습니다. Pipeline을 확인해주세요.");
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}
	bool RenderModule::bindPipeline(Pipeline& pipeline, const Pipeline::Type type)
	{
		ID3D12DescriptorHeap* heaps[] = { _textureDescriptorHeap.get(),  };
		if (type == Pipeline::Type::RAYTRACING)
		{
			_commandList->SetPipelineState1(pipeline._rtStateObject.get());
			_commandList->SetDescriptorHeaps(DK_COUNT_OF(heaps), heaps);	// TODO 비싼 함수니까 initialize쪽으로 옮기자. 어차피 bindless인데..
			_commandList->SetComputeRootSignature(pipeline._rootSignature.get());
			_commandList->SetComputeRootDescriptorTable(0, _textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			_commandList->SetComputeRootDescriptorTable(1, _textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		}
		else
		{
			_commandList->SetPipelineState(pipeline._pipelineStateObject.get());
			_commandList->SetDescriptorHeaps(DK_COUNT_OF(heaps), heaps);	// TODO 비싼 함수니까 initialize쪽으로 옮기자. 어차피 bindless인데..
			if (type == Pipeline::Type::COMPUTE)
			{
				_commandList->SetComputeRootSignature(pipeline._rootSignature.get());
				_commandList->SetComputeRootDescriptorTable(0, _textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
				_commandList->SetComputeRootDescriptorTable(1, _textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			}
			else
			{
				_commandList->SetGraphicsRootSignature(pipeline._rootSignature.get());
				_commandList->SetGraphicsRootDescriptorTable(0, _textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
				_commandList->IASetPrimitiveTopology(convertPrimitiveTopology(pipeline._primitiveTopologyType));
			}
		}

		return true;
	}
	void RenderModule::setRoot32BitConstants(const uint32 rootParameterIndex, const uint32 count, const void* data, uint32 offset, const Pipeline::Type type)
	{
		if (type != Pipeline::Type::GRAPHIC)
			_commandList->SetComputeRoot32BitConstants(rootParameterIndex, count, data, offset / 4);
		else
			_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, count, data, offset / 4);
	}
	void RenderModule::bindConstantBuffer(const uint32 rootParameterIndex, const IBufferRef& buffer, const Pipeline::Type type)
	{
		if (type != Pipeline::Type::GRAPHIC)
			_commandList->SetComputeRootConstantBufferView(rootParameterIndex, buffer->_buffer->GetGPUVirtualAddress());
		else
			_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, buffer->_buffer->GetGPUVirtualAddress());
	}
	void RenderModule::bindShaderResourceView(const uint32 rootParameterIndex, const IBufferRef& buffer, const Pipeline::Type type)
	{
		if (type != Pipeline::Type::GRAPHIC)
			_commandList->SetComputeRootShaderResourceView(rootParameterIndex, buffer->_buffer->GetGPUVirtualAddress());
		else
			_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, buffer->_buffer->GetGPUVirtualAddress());
	}
	void RenderModule::setVertexBuffers(const uint32 startSlot, const uint32 numViews, const D3D12_VERTEX_BUFFER_VIEW* views)
	{
		_commandList->IASetVertexBuffers(startSlot, numViews, views);
	}
	void RenderModule::setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view)
	{
		_commandList->IASetIndexBuffer(view);
	}
	void RenderModule::drawIndexedInstanced(const uint32 indexCountPerInstance, const uint32 instanceCount, const uint32 startIndexLocation, const int baseVertexLocation, const uint32 startInstanceLocation)
	{
		_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}

	void RenderModule::dispatch(const uint32 threadGroupCountX, const uint32 threadGroupCountY, const uint32 threadGroupCountZ)
	{
		if (threadGroupCountX == 0 || threadGroupCountY == 0 || threadGroupCountZ == 0)
			return;

		if (gCurrentBindedPipeline->_type != Pipeline::Type::COMPUTE)
		{
			DK_ASSERT_LOG(false, "dispatchThreads requires a compute pipeline.");
			return;
		}

		const uint32* size = gCurrentBindedPipeline->_threadGroupSize;
		if (size[0] == 0 || size[1] == 0 || size[2] == 0)
		{
			DK_ASSERT_LOG(false, "Compute thread group size is not initialized.");
			return;
		}

		// 덧셈 overflow 없이 올림 나눗셈
		auto divideRoundUp = [](uint32 count, uint32 groupSize) -> uint32
		{
			return count / groupSize + (count % groupSize != 0);
		};

		const uint32 groupsX = divideRoundUp(threadGroupCountX, size[0]);
		const uint32 groupsY = divideRoundUp(threadGroupCountY, size[1]);
		const uint32 groupsZ = divideRoundUp(threadGroupCountZ, size[2]);

		if (groupsX > D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION || groupsY > D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION || groupsZ > D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION)
		{
			DK_ASSERT_LOG(false, "Compute dispatch group count exceeds the limit.");
			return;
		}

		_commandList->Dispatch(groupsX, groupsY, groupsZ);
	}

	void RenderModule::endRender()
	{
#ifdef USE_IMGUI
		ID3D12DescriptorHeap* heaps[] = { _imguiDescriptorHeap.get(), };
		_commandList->SetDescriptorHeaps(DK_COUNT_OF(heaps), heaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList._commandList.get());
#endif // USE_IMGUI

		resourceBarrierTransition(_backBufferResourceArr[kCurrentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT);

		execute(true);

		if(gSerializeRender)
			waitFenceAndResetCommandList();

#if defined(_DK_DEBUG_)
		_blockUpload = false;
		_blockCopy = false;
#endif
	}

	void RenderModule::dispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc)
	{
		_commandList->DispatchRays(pDesc);
	}

	void IBuffer::upload(const void* data)
	{
		DK_ASSERT_LOG(DuckingEngine::getInstance().GetRenderModule()._blockUpload == false, "");
		DK_ASSERT_LOG(data != nullptr, "nullptr인 data를 upload요청하려고합니다.");
		DK_ASSERT_LOG(_type == Type::UPLOAD && isValid(), "Upload버퍼에 대해서만 호출 가능합니다.");

		RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
		DKCommandList& cl = rm._commandList;

		DKCommandList::UploadResourcePendingData pending;
		pending._target = shared_from_this();
		pending._data.resize(_bufferSize);
		DK::memcpy(pending._data.data(), data, _bufferSize);
		cl._uploadResourcePendingArray.push_back(DK::move(pending));
	}

	ITexture::~ITexture()
	{
#if defined(_DK_DEBUG_)
		if (DuckingEngine::getInstance().GetRenderModule()._isDestroyed)
		{
			DK_ASSERT_LOG(false, "Muse called before rendermodule destroy!!!\nPath: %s", _path.c_str());
			return;
		}
#endif
		DuckingEngine::getInstance().GetRenderModuleWritable().deleteTexture(this);
	}

}
