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
#pragma endregion

#define USE_WINCODEC
#ifdef USE_WINCODEC
#include <wincodec.h>	// Image Process
DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

	else return DXGI_FORMAT_UNKNOWN;
}
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
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
uint32 GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
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

#pragma endregion

#include "DuckingEngine.h"
#include "ResourceManager.h"

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"

#include "Material.h"

namespace DK
{
	static const float4 gClearRenderTargetViewColor(0, 1, 0, 1);

	uint32 RenderModule::kCurrentFrameIndex = 0;
	uint32 RenderModule::kWidth = 0;
	uint32 RenderModule::kHeight = 0;
	DXGI_FORMAT gDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
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

#define TEXTUREBINDLESS_MAX_COUNT 4096
#define TEXTUREBINDLESS_SPACE 10

	template<typename T>
	RenderResourcePtr<T>::~RenderResourcePtr()
	{
		if (_ptr != nullptr)
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
	bool RenderModule::initialize(const HWND hwnd, const uint32 width, const uint32 height)
	{
		kWidth = width;
		kHeight = height;

		if (initialize_createDeviceAndCommandQueueAndSwapChain(hwnd, width, height) == false) 
			return false;

		_commandList.assign(createCommandList());

		if (_commandList.get() == nullptr) 
			return false;
		_commandList->_commandList->Close();

		if (initialize_createFence() == false) 
			return false;

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
	bool RenderModule::initialize_createDeviceAndCommandQueueAndSwapChain(const HWND hwnd, const uint32 width, const uint32 height)
	{
		HRESULT hr;

		// Create DirectX Factory
		IDXGIFactory4* factory;
		{
			UINT factoryFlags = 0;
#if defined(_DK_DEBUG_)
			factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
			hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory));
			if (SUCCEEDED(hr) == false)
				return false;

			// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
			// will be handled manually.
			hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
			if (SUCCEEDED(hr) == false)
				return false;
		}

		// Activate DebugLayer
		{
#if 0
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
#endif
		}

		// Create RenderDevice
		{
			if (_useWarpDevice == true)
			{
				IDXGIAdapter* warpAdapter;
				if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))) == false)
					return false;
				if (SUCCEEDED(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(_device.getAddress()))) == false)
					return false;
			}
			else
			{
				IDXGIAdapter1* hardwareAdapter;
				GetHardwareAdapter(factory, &hardwareAdapter);
				if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(_device.getAddress()))) == false)
					return false;
			}
		}

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
			if (FAILED(_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(_commandQueue.getAddress()))))
				return false;
		}

		// Create Bindless Texture Descriptor
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = TEXTUREBINDLESS_MAX_COUNT;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			hr = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_textureDescriptorHeap.getAddress()));
			if (FAILED(hr))
				return false;

			_textureDescriptorHeap->SetName(L"BindlessTextureDescriptorHeap");
		}

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
			dsvHeapDesc.NumDescriptors = DK_ARRAYSIZE_OF(_depthStencilResourceArr);
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			hr = _device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_depthStencilDescriptorHeap.getAddress()));
			if (FAILED(hr) == true)
				return false;

			const UINT dsvHandleSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

			CD3DX12_HEAP_PROPERTIES dsvHeapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			CD3DX12_RESOURCE_DESC dsvDesc = CD3DX12_RESOURCE_DESC::Tex2D(GetDepthResourceFormat(gDepthStencilFormat), width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			// TODO: 현재 Depth/Stencil 버퍼는 2개만 필요하다.. 근데 4개나 만들고 있다. 다음에 FrameBuffer Instance를 만들어서 관리할 수 있도록 해야겠다
			for (uint32 i = 0; i < DK_ARRAYSIZE_OF(_depthStencilResourceArr); ++i)
			{
				hr = _device->CreateCommittedResource(
					&dsvHeapProperty, D3D12_HEAP_FLAG_NONE, &dsvDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &depthOptimizedClearValue, IID_PPV_ARGS(_depthStencilResourceArr[i].getAddress())
				);
				if (FAILED(hr) == true)
					return false;

				D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
				depthStencilViewDesc.Format = gDepthStencilFormat;
				depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
				_device->CreateDepthStencilView(_depthStencilResourceArr[i].get(), &depthStencilViewDesc, dsvHandle);

				dsvHandle.ptr += dsvHandleSize;

				ScopeString<DK_MAX_BUFFER> indexString;
				StringUtil::itoa(i, indexString.data(), indexString.capacity());
				ScopeString<DK_MAX_BUFFER> dsvTextureName("DepthStencilTexture_");
				dsvTextureName.append(indexString.c_str());
				_depthStencilTextureArr[i] = allocateTextureSRV(dsvTextureName.c_str(), _depthStencilResourceArr[i].get(), GetDepthSRVFormat(gDepthStencilFormat));
				if (_depthStencilTextureArr[i] == nullptr)
				{
					DK_ASSERT_LOG(false, "PostProcess에 사용하는 Depth/Stencil Texture 생성에 실패.");
					return false;
				}

				_depthStencilResourceArr[i]->SetName(StringUtil::ConverCtoWC(dsvTextureName.c_str()).c_str());
			}
		}

		// Create RTV descriptorHeap
		UINT rtvDescriptorSize = 0;
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = DK_ARRAYSIZE_OF(_renderTargetResourceArr) + DK_ARRAYSIZE_OF(_backBufferResourceArr);
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			hr = _device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(_renderTargetViewHeap.getAddress()));
			if (FAILED(hr) == true)
				return false;

			rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			if (FAILED(hr) == true)
				return false;

			_renderTargetViewHeap->SetName(L"RTVDescriptorHeap");
		}

		// Create RTV
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		{
			for (uint32 i = 0; i < DK_ARRAYSIZE_OF(_renderTargetResourceArr); ++i)
			{
				CD3DX12_HEAP_PROPERTIES rtvHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
				CD3DX12_RESOURCE_DESC rtResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(renderTargetFormat, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
				D3D12_CLEAR_VALUE clearValue;
				clearValue.Format = renderTargetFormat;
				clearValue.Color[0] = gClearRenderTargetViewColor.x;
				clearValue.Color[1] = gClearRenderTargetViewColor.y;
				clearValue.Color[2] = gClearRenderTargetViewColor.z;
				clearValue.Color[3] = gClearRenderTargetViewColor.w;
				hr = _device->CreateCommittedResource(&rtvHeapProperties, D3D12_HEAP_FLAG_NONE, &rtResourceDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(_renderTargetResourceArr[i].getAddress()));
				DK_ASSERT_LOG(SUCCEEDED(hr), "RenderTarget Resource 생성 실패");

				_device->CreateRenderTargetView(_renderTargetResourceArr[i].get(), nullptr, rtvHandle);
				rtvHandle.ptr += rtvDescriptorSize;

				ScopeString<DK_MAX_BUFFER> indexString;
				StringUtil::itoa(i, indexString.data(), indexString.capacity());
				ScopeString<DK_MAX_BUFFER> rtvTextureName("RenderTargetTexture_");
				rtvTextureName.append(indexString.c_str());
				_renderTargetTextureArr[i] = allocateTextureSRV(rtvTextureName.c_str(), _renderTargetResourceArr[i].get(), renderTargetFormat);
				if (_renderTargetTextureArr[i] == nullptr)
				{
					DK_ASSERT_LOG(false, "Fail - Create RenderTargetTexture");
					return false;
				}

				_renderTargetResourceArr[i]->SetName(StringUtil::ConverCtoWC(rtvTextureName.c_str()).c_str());
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
			swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
			swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			hr = factory->CreateSwapChainForHwnd(_commandQueue.get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(_swapChain.getAddress()));
			if (FAILED(hr) == true)
				return false;
			kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

			for (uint32 i = 0; i < kFrameCount; ++i)
			{
				hr = _swapChain->GetBuffer(i, IID_PPV_ARGS(_backBufferResourceArr[i].getAddress()));
				if (FAILED(hr) == true)
					return false;

				_device->CreateRenderTargetView(_backBufferResourceArr[i].get(), nullptr, rtvHandle);
				rtvHandle.ptr += rtvDescriptorSize;

				ScopeString<DK_MAX_BUFFER> indexString;
				StringUtil::itoa(i, indexString.data(), indexString.capacity());
				ScopeStringW<DK_MAX_BUFFER> resourceName(L"BackBufferResource_");
				resourceName.append(StringUtil::ConverCtoWC(indexString.c_str()).c_str());
				_backBufferResourceArr[i]->SetName(resourceName.c_str());
			}
		}

		// Viewport
		{
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

		RenderResourcePtr<ID3D12CommandAllocator> outCommandAllocator[RenderModule::kFrameCount] = { nullptr, };
		RenderResourcePtr<ID3D12GraphicsCommandList> outCommandList = nullptr;
		for (uint32 i = 0; i < RenderModule::kFrameCount; ++i)
		{
			hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(outCommandAllocator[i].getAddress()));
			if (FAILED(hr))
				return nullptr;

			outCommandAllocator[i]->SetName(L"CommandAllocator");
		}

		hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, outCommandAllocator[0].get(), NULL, IID_PPV_ARGS(outCommandList.getAddress()));
		if (FAILED(hr))
			return nullptr;

		outCommandList->SetName(L"CommandList");

		return dk_new DKCommandList(outCommandAllocator, outCommandList);
	}
	bool RenderModule::initialize_createFence()
	{
		for (uint32 i = 0; i < kFrameCount; ++i)
		{
			HRESULT hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fences[i].getAddress()));
			if (FAILED(hr))
				return false;

			_fences[i]->Signal(_fenceValues[i]);
		}

		_fenceEvent = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		return true;

		return true;
	}

	bool RenderModule::createRootSignature(RenderPass& renderPass, Pipeline& inoutPipeline)
	{
		const uint32 parameterCount = static_cast<uint32>(renderPass._shaderParameterMap.size() + inoutPipeline._shaderParameterMap.size()) + 1;	// Texture Bindless 전용 +1
		DKVector<D3D12_ROOT_PARAMETER> rootParameters;
		rootParameters.resize(parameterCount);
		uint32 rootParameterIndex = 0;

		// Bindless Texture 2D Table
		{
			D3D12_DESCRIPTOR_RANGE  texture2DTableDescriptorRange[1];
			texture2DTableDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			texture2DTableDescriptorRange[0].NumDescriptors = TEXTUREBINDLESS_MAX_COUNT;
			texture2DTableDescriptorRange[0].RegisterSpace = TEXTUREBINDLESS_SPACE;
			texture2DTableDescriptorRange[0].BaseShaderRegister = 0;
			texture2DTableDescriptorRange[0].OffsetInDescriptorsFromTableStart = 0;
			D3D12_ROOT_DESCRIPTOR_TABLE texture2DTableDescriptorTable;
			texture2DTableDescriptorTable.NumDescriptorRanges = _countof(texture2DTableDescriptorRange);
			texture2DTableDescriptorTable.pDescriptorRanges = &texture2DTableDescriptorRange[0];

			rootParameters[rootParameterIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[rootParameterIndex].DescriptorTable = texture2DTableDescriptorTable;
			rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			++rootParameterIndex;
		}

		for(DKPair<const DKString, ShaderParameter>& renderPassShaderParameter : renderPass._shaderParameterMap)
		{
			D3D12_ROOT_DESCRIPTOR constantBufferDescriptor = {};
			constantBufferDescriptor.RegisterSpace = 0;		// 현재 DuckingEngine은 0번 Space만 사용합니다.
			constantBufferDescriptor.ShaderRegister = renderPassShaderParameter.second._register;

			rootParameters[rootParameterIndex].ParameterType = renderPassShaderParameter.second._type == ShaderParameterType::StructuredBuffer ? D3D12_ROOT_PARAMETER_TYPE_SRV : D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[rootParameterIndex].Descriptor = constantBufferDescriptor;
			rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			renderPassShaderParameter.second._rootParameterIndex = rootParameterIndex;
			++rootParameterIndex;
		}

		// RenderPass Parameters
		for (DKPair<const DKString, ShaderParameter>& shaderParameter : inoutPipeline._shaderParameterMap)
		{
			D3D12_ROOT_DESCRIPTOR constantBufferDescriptor = {};
			constantBufferDescriptor.RegisterSpace = 0;		// 현재 DuckingEngine은 0번 Space만 사용합니다.
			constantBufferDescriptor.ShaderRegister = shaderParameter.second._register;

			rootParameters[rootParameterIndex].ParameterType = shaderParameter.second._type == ShaderParameterType::StructuredBuffer ? D3D12_ROOT_PARAMETER_TYPE_SRV : D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[rootParameterIndex].Descriptor = constantBufferDescriptor;
			rootParameters[rootParameterIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			shaderParameter.second._rootParameterIndex = rootParameterIndex;
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
			signature->GetBufferPointer(), static_cast<uint32>(signature->GetBufferSize()),
#if 0
			__uuidof(ID3D12RootSignature), &inoutPipeline._rootSignature
#else
			IID_PPV_ARGS(inoutPipeline._rootSignature.getAddress())
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

		ScopeString<DK_MAX_PATH> shaderFullPath = GlobalPath::makeResourceFullPath(shaderPath);

		Ptr<const wchar_t> shaderPathW(charTowChar(shaderFullPath.c_str()));
		Ptr<const wchar_t> shaderEntryW(charTowChar(entry));

		RenderResourcePtr<IDxcBlobEncoding> sourceBlob(nullptr);
		hr = utils->LoadFile(shaderPathW.get(), nullptr, sourceBlob.getAddress());
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		// 참고: https://simoncoenen.com/blog/programming/graphics/DxcCompiling
		DKVector<LPCWSTR> arguments;
		//-E for the entry point (eg. PSMain)
		arguments.push_back(L"-E");
		arguments.push_back(shaderEntryW.get());

		//-T for the target profile (eg. ps_6_2)
		arguments.push_back(L"-T");
		arguments.push_back(isVertexShader ? L"vs_6_2" : L"ps_6_2");

#ifndef _DK_DEBUG_
		// HLSL Object파일에 Reflect, PBD파일을 제거하는 옵션
		// 하지만 IDxcResult에는 여전히 포함하기 때문에 getOutput으로 결과를 가져올 수 있습니다. (DXC_OUT_REFLECTION, DXC_OUT_PDB)
		//Strip reflection data and pdbs (see later)
		arguments.push_back(L"-Qstrip_debug");
		arguments.push_back(L"-Qstrip_reflect");
#endif

#ifdef _DK_DEBUG_
		arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
		arguments.push_back(DXC_ARG_DEBUG); //-Zi
#endif
		arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp

		arguments.push_back(L"-I");
		ScopeStringW<DK_MAX_PATH> includePath = GlobalPath::makeResourceFullPathW(L"Material");
		arguments.push_back(includePath.c_str());

		//for (const std::wstring& define : defines)
		//{
		//	arguments.push_back(L"-D");
		//	arguments.push_back(define.c_str());
		//}

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
		sourceBuffer.Size = sourceBlob->GetBufferSize();
		sourceBuffer.Encoding = DXC_CP_ACP;

		RenderResourcePtr<IDxcIncludeHandler> defaultIncludeHandler;
		hr = utils->CreateDefaultIncludeHandler(defaultIncludeHandler.getAddress());
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "IncludeHandler 생성에 실패했습니다. Shader Compiler을 하지 않습니다.");
			return false;
		}

		RenderResourcePtr<IDxcResult> result(nullptr);
		hr = compiler3->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()), defaultIncludeHandler.get(), IID_PPV_ARGS(result.getAddress()));
		if (FAILED(hr))
		{
			// DXC_OUT_OBJECT
			// DXC_OUT_ERRORS
			// DXC_OUT_PDB
			// DXC_OUT_SHADER_HASH
			// DXC_OUT_DISASSEMBLY
			// DXC_OUT_HLSL
			// DXC_OUT_TEXT
			// DXC_OUT_REFLECTION
			// DXC_OUT_ROOT_SIGNATURE
			RenderResourcePtr<IDxcBlobUtf8> errors(nullptr);
			hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.getAddress()), nullptr);
			if (SUCCEEDED(hr))
				DK_ASSERT_LOG(false, "Shader Compile Error\nPath: %s\nLog: %s", shaderPath, errors->GetStringPointer());

			return false;
		}

		HRESULT status(S_OK);
		hr = result->GetStatus(&status);
		if (FAILED(hr) || FAILED(status))
		{
			RenderResourcePtr<IDxcBlobUtf8> errors(nullptr);
			hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.getAddress()), nullptr);
			const char* test = errors->GetStringPointer();
			DK_ASSERT_LOG(FAILED(hr), "Shader Compile Error\nPath: %s\nLog: %s", shaderPath, test);

			return false;
		}

#ifdef _DK_DEBUG_
		//{
		//	RenderResourcePtr<IDxcBlob> debugData;
		//	RenderResourcePtr<IDxcBlobUtf16> debugDataPath;
		//	hr = result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(debugData.getAddress()), debugDataPath.getAddress());
		//	if (FAILED(hr))
		//	{
		//		DK_ASSERT_LOG(false, "DebugData 가져오기 실패!");
		//		return false;
		//	}
		//	{
		//		DK_ASSERT_LOG(false, "DebugData(%d): %s", debugData->GetBufferSize(), debugData->GetBufferPointer());
		//		DxcBuffer dataBuffer;
		//		dataBuffer.Ptr = debugData->GetBufferPointer();
		//		dataBuffer.Size = debugData->GetBufferSize();
		//	}
		//	{
		//		DK_ASSERT_LOG(false, "DebugDataPath(%d): %s", debugDataPath->GetBufferSize(), debugDataPath->GetBufferPointer());
		//		DxcBuffer dataBuffer;
		//		dataBuffer.Ptr = debugDataPath->GetBufferPointer();
		//		dataBuffer.Size = debugDataPath->GetBufferSize();
		//	}
		//}

		RenderResourcePtr<IDxcBlob> reflectionData;
		result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionData.getAddress()), nullptr);
		DxcBuffer reflectionBuffer;
		reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
		reflectionBuffer.Size = reflectionData->GetBufferSize();
		reflectionBuffer.Encoding = 0;
		RenderResourcePtr<ID3D12ShaderReflection> shaderReflection;
		utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(shaderReflection.getAddress()));

		D3D12_SHADER_DESC shaderDesc;
		shaderReflection->GetDesc(&shaderDesc);
		const uint32 cBufferCount = shaderDesc.ConstantBuffers;
		for (uint32 i = 0; i < cBufferCount; ++i)
		{
			ID3D12ShaderReflectionConstantBuffer* cbReflection = nullptr;
			cbReflection = shaderReflection->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC shaderBufferDesc;
			cbReflection->GetDesc(&shaderBufferDesc);
			DKString shaderBufferName = shaderBufferDesc.Name;
		}
#endif

		RenderResourcePtr<IDxcBlobUtf16> shaderName = nullptr;
		hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), shaderName.getAddress());
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		outShader.BytecodeLength = shader->GetBufferSize();
		outShader.pShaderBytecode = shader->GetBufferPointer();

		return true;
	}
	bool RenderModule::createPipelineObjectState(const Pipeline::CreateInfo& pipelineCreateInfo, Pipeline& inoutPipeline)
	{
		D3D12_SHADER_BYTECODE vertexShaderView = {};
		RenderResourcePtr<IDxcBlob> vertexShader = nullptr;
		bool success = compileShader(pipelineCreateInfo._vertexShaderPath, pipelineCreateInfo._vertexShaderEntry, true, vertexShader.get(), vertexShaderView);
		if (success == false)
			return false;
		D3D12_SHADER_BYTECODE pixelShaderView = {};
		RenderResourcePtr<IDxcBlob> pixelShader = nullptr;
		success = compileShader(pipelineCreateInfo._pixelShaderPath, pipelineCreateInfo._pixelShaderEntry, false, pixelShader.get(), pixelShaderView);
		if (success == false)
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
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
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

		HRESULT hr = _device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(inoutPipeline._pipelineStateObject.getAddress()));
		if (SUCCEEDED(hr) == false)
		{
			DK_ASSERT_LOG(false, "Pipeline State Object 생성에 실패");
			return false;
		}

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

		const uint32 strCount = ARRAYSIZE(primitiveTopologyTypeStr);
		for (uint32 i = 0; i < strCount; ++i)
		{
			if (strcmp(value, primitiveTopologyTypeStr[i]) == 0)
				return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(i);
		}

		DK_ASSERT_LOG(false, "Ptimitive정보가 올바르지 않습니다. pipeline이 작동되지 않을테니 반드시 확인이 필요합니다.");
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
	bool RenderModule::createRenderPass(const DKString& renderPassName, RenderPass::CreateInfo&& renderPassCreateInfo)
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
		renderPass._shaderParameterMap.swap(renderPassCreateInfo._shaderParameterMap);

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
			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType = convertPrimitiveTopologyType(pipelineCreateInfo._primitiveTopologyType);
			if (primitiveType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED)
				continue;

			newPipeline._primitiveTopologyType = primitiveType;

			newPipeline._shaderParameterMap.swap(pipelineCreateInfo._shaderParameterMap);
			if (createRootSignature(renderPass, newPipeline) == false)
				return false;
			if (createPipelineObjectState(pipelineCreateInfo, newPipeline) == false)
				return false;

			renderPass._pipelineMap.insert(DKPair<DKString, Pipeline>(pipelineName, DK::move(newPipeline)));
		}

		return true;
	}
#pragma endregion

	void RenderModule::waitFenceAndResetCommandList()
	{
		kCurrentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

		if (static_cast<uint32>(_fences[kCurrentFrameIndex]->GetCompletedValue()) < _fenceValues[kCurrentFrameIndex])
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

		DKVector<ID3D12CommandList*> commandLists;
		commandLists.push_back(_commandList->_commandList.get());
		_commandQueue->ExecuteCommandLists(static_cast<uint32>(commandLists.size()), commandLists.data());
		_commandQueue->Signal(_fences[kCurrentFrameIndex].get(), _fenceValues[kCurrentFrameIndex]);
	}

	ID3D12Resource* RenderModule::createBufferInternal(const uint32 size, const D3D12_HEAP_TYPE type, const D3D12_RESOURCE_STATES state, const DKStringW& debugName)
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

		returnBuffer->SetName(debugName.c_str());

		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "CreateBuffer Failed! 랜더링이 정상적이지 않을 수 있습니다.");
			return nullptr;
		}

		return returnBuffer;
	}
	ID3D12Resource* RenderModule::createDefaultBuffer(const uint32 size, const D3D12_RESOURCE_STATES state, const DKStringW& debugName)
	{
		return createBufferInternal(size, D3D12_HEAP_TYPE_DEFAULT, state, debugName);
	}
	IBuffer* RenderModule::createUploadBuffer(const uint32 size, const DKStringW& debugName)
	{
		RenderResourcePtr<ID3D12Resource> buffers[RenderModule::kFrameCount];
		uint32 alignedSize = (size + 255) & ~255;
		for (uint32 i = 0; i < kFrameCount; ++i)
		{
			buffers[i] = createBufferInternal(alignedSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, debugName);
		}

		return dk_new IBuffer(buffers, size);
	}
	ID3D12Resource* RenderModule::createInitializedDefaultBuffer(const void* data, const uint32 bufferSize, const DKStringW& debugName)
	{
		ID3D12Resource* uploadBuffer = createBufferInternal(bufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, debugName);
		if (uploadBuffer == nullptr)
			return nullptr;

		void* uploadBufferGPUAddress = nullptr;
		HRESULT hr = uploadBuffer->Map(0, nullptr, &uploadBufferGPUAddress);
		DK_ASSERT_LOG(SUCCEEDED(hr), "Map 실패");
		memcpy(uploadBufferGPUAddress, data, bufferSize);

		D3D12_RESOURCE_STATES defaultBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
		ID3D12Resource* defaultBuffer = createDefaultBuffer(bufferSize, defaultBufferState, debugName);
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
	const bool RenderModule::createVertexBuffer(const void* data, const uint32 strideSize, const uint32 vertexCount, VertexBufferViewRef& outView, const DKStringW& debugName)
	{
		uint32 bufferSizeInBytes = strideSize * vertexCount;
		ID3D12Resource* defaultBuffer = createInitializedDefaultBuffer(data, bufferSizeInBytes, debugName);
		if (defaultBuffer == nullptr)
			return false;

		D3D12_VERTEX_BUFFER_VIEW view;
		view.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
		view.StrideInBytes = strideSize;
		view.SizeInBytes = bufferSizeInBytes;

		outView = std::make_shared<D3D12_VERTEX_BUFFER_VIEW>(view);

		return true;
	}
	const bool RenderModule::createIndexBuffer(const void* data, const uint32 bufferSize, IndexBufferViewRef& outView, const DKStringW& debugName)
	{
		uint32 bufferSizeInBytes = sizeof(uint32) * bufferSize;
		ID3D12Resource* defaultBuffer = createInitializedDefaultBuffer(data, bufferSizeInBytes, debugName);
		if (defaultBuffer == nullptr)
			return false;

		D3D12_INDEX_BUFFER_VIEW view;
		view.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
		view.Format = DXGI_FORMAT_R32_UINT;
		view.SizeInBytes = bufferSizeInBytes;

		outView = std::make_shared<D3D12_INDEX_BUFFER_VIEW>(view);

		return true;
	}

	ITextureRef RenderModule::allocateTextureSRV(const DKString& name, ID3D12Resource* textureBuffer, const DXGI_FORMAT format)
	{
		EnsureMainThread();

		ITexture::TextureSRVType index = static_cast<ITexture::TextureSRVType>(_textureContainer.size());
		if (_deletedTextureSRVArr.empty() == false)
		{
			index = _deletedTextureSRVArr.back();
			_deletedTextureSRVArr.pop_back();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE textureDescriptorHeapHandle = _textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		textureDescriptorHeapHandle.ptr += index * _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		_device->CreateShaderResourceView(textureBuffer, &srvDesc, textureDescriptorHeapHandle);

		using InsertResult = DKPair<DKHashMap<DKString, ITextureRef>::iterator, bool>;
		InsertResult insertResult = _textureContainer.insert(DKPair<DKString, ITextureRef>(name, dk_new ITexture(name, index)));
		if (insertResult.second == false)
		{
			DK_ASSERT_LOG(false, "HashMap Insert실패. 해시 자체의 오류일 수 있음");
			return nullptr;
		}

		return insertResult.first->second;
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

		const DKStringW wTexturePath = StringUtil::ConverCtoWC(fileName);
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
	ITextureRef RenderModule::allocateTexture(const DKString& path)
	{
		DKHashMap<DKString, ITextureRef>::iterator findResult = _textureContainer.find(path);
		if (findResult != _textureContainer.end())
			return findResult->second;

		ScopeString<DK_MAX_PATH> textureFullPath = GlobalPath::makeResourceFullPath(path);

		TextureRaw textureRaw;
		const bool loadingTextureSuccess = loadImageDataFromFile(textureFullPath.c_str(), textureRaw);
		if (loadingTextureSuccess == false)
		{
			DK_ASSERT_LOG(false, "TextureData 로딩에 실패했습니다.");
			// #todo- Null RefPtr을 static하게 만들고 그거 반환해야함
		}

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
		HRESULT hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer));
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "TextureData 로딩에 실패했습니다.");
			// #todo- Null RefPtr을 static하게 만들고 그거 반환해야함
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
		hr = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "TextureData 로딩에 실패했습니다.");
			// #todo- Null RefPtr을 static하게 만들고 그거 반환해야함
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

		return allocateTextureSRV(path, defaultBuffer, textureRaw._format);
	}

	void RenderModule::deallocateTextureSRV(const DKString& path, const ITexture::TextureSRVType srvIndex)
	{
		EnsureMainThread();

		const size_t insertResult = _textureContainer.erase(path);
		DK_ASSERT_LOG(insertResult == 1, "TextureContainer에 없는 TextureSRV를 해제 시도합니다.\nPath: %s", path.c_str());
		_deletedTextureSRVArr.push_back(srvIndex);
	}

	void RenderModule::preRender()
	{
		waitFenceAndResetCommandList();		
	}

	void RenderModule::bindRenderPass(const uint32 rtvReadSlot, const uint32 rtvSlot, const bool bindDSV, const bool clearTarget)
	{
		const uint32 rtvIndex = kCurrentFrameIndex * kFrameCount + rtvSlot;

		if(rtvReadSlot != 0xFFFFFFFF)
		{
			const uint32 rtvPrevIndex = kCurrentFrameIndex * kFrameCount + rtvReadSlot;

			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = _renderTargetResourceArr[rtvPrevIndex].get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			_commandList->_commandList->ResourceBarrier(1, &barrier);
		}

		{
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = 2 == rtvSlot ? _backBufferResourceArr[kCurrentFrameIndex].get() : _renderTargetResourceArr[rtvIndex].get();
			barrier.Transition.StateBefore = 2 == rtvSlot ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			_commandList->_commandList->ResourceBarrier(1, &barrier);
		}

		if (rtvReadSlot != 0xFFFFFFFF && 2 != rtvSlot)
		{
			const uint32 rtvPrevIndex = kCurrentFrameIndex * kFrameCount + rtvReadSlot;

			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = _depthStencilResourceArr[rtvPrevIndex].get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			_commandList->_commandList->ResourceBarrier(1, &barrier);
		}

		const UINT rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += rtvSlot == 2 ? rtvDescriptorSize * (4 + kCurrentFrameIndex) : rtvDescriptorSize * rtvIndex;

		if (bindDSV)
		{
			{
				D3D12_RESOURCE_BARRIER barrier;
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource = _depthStencilResourceArr[rtvIndex].get();
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				_commandList->_commandList->ResourceBarrier(1, &barrier);
			}

			const UINT dsvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			dsvHandle.ptr += dsvDescriptorSize * rtvIndex;
			_commandList->_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

			if (clearTarget)
			{
				_commandList->_commandList->ClearRenderTargetView(rtvHandle, gClearRenderTargetViewColor.m, 0, nullptr);
				_commandList->_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
			}
		}
		else
		{
			_commandList->_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
			if (clearTarget)
				_commandList->_commandList->ClearRenderTargetView(rtvHandle, gClearRenderTargetViewColor.m, 0, nullptr);
		}

		_commandList->_commandList->RSSetViewports(1, _viewport.get());
		_commandList->_commandList->RSSetScissorRects(1, _scissorRect.get());
		_commandList->_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
	bool RenderModule::bindPipeline(Pipeline& pipeline)
	{
		_commandList->_commandList->SetGraphicsRootSignature(pipeline._rootSignature.get());
		_commandList->_commandList->SetPipelineState(pipeline._pipelineStateObject.get());
		_commandList->_commandList->IASetPrimitiveTopology(convertPrimitiveTopology(pipeline._primitiveTopologyType));
		_commandList->_commandList->SetDescriptorHeaps(1, _textureDescriptorHeap.getAddress());
		_commandList->_commandList->SetGraphicsRootDescriptorTable(0, _textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		return true;
	}
	void RenderModule::bindConstantBuffer(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress)
	{
		_commandList->_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAdress);
	}
	void RenderModule::bindShaderResourceView(const uint32 rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS& gpuAdress)
	{
		_commandList->_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAdress);
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

		{
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = _backBufferResourceArr[kCurrentFrameIndex].get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			_commandList->_commandList->ResourceBarrier(1, &barrier);
		}

		execute();

		UINT syncInterval = 0; //gVSync ? 1 : 0;
		UINT presentFlags = 0; // CheckTearingSupport() && !gVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		HRESULT hr = _swapChain->Present(syncInterval, presentFlags);
		DK_ASSERT_LOG(SUCCEEDED(hr), "Present 실패!");
	}

	bool DKCommandList::reset()
	{
		_commandAllocators[_lastResetIndex]->Reset();
		_commandList->Reset(_commandAllocators[_lastResetIndex].get(), nullptr);

		_lastResetIndex = (_lastResetIndex + 1) % 2;

		return false;
	}

	void IBuffer::upload(const void* data)
	{
		//DK_ASSERT_LOG(data != nullptr, "nullptr인 data를 upload요청하려고합니다.");

		_lastUploadIndex = (_lastUploadIndex + 1) % 2;

		void* address = nullptr;
		HRESULT hr = _buffers[_lastUploadIndex]->Map(0, nullptr, &address);
		DK_ASSERT_LOG(SUCCEEDED(hr), "Map에 실패하였습니다.");
		memcpy(address, data, _bufferSize);
		_buffers[_lastUploadIndex]->Unmap(0, nullptr);
	}

	D3D12_GPU_VIRTUAL_ADDRESS IBuffer::getGPUVirtualAddress()
	{
		return _buffers[_lastUploadIndex]->GetGPUVirtualAddress();
	}

	ITexture::~ITexture()
	{
		DuckingEngine::getInstance().GetRenderModuleWritable().deallocateTextureSRV(getPath(), getSRV());
	}

}