#include "stdafx.h"
#include "RenderModule.h"
#include "FileUtil.h"
#include <cstdint>
#include <cstdlib>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 721; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

#pragma region Lib
#define USE_WINCODEC
#ifdef USE_WINCODEC
#include <wincodec.h>	// Image Process
namespace DK
{
	static DXGI_FORMAT GetDXGIFormatFromWICFormat(const WICPixelFormatGUID& format)
	{
		if (format == GUID_WICPixelFormat128bppRGBAFloat)
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		if (format == GUID_WICPixelFormat64bppRGBAHalf)
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		if (format == GUID_WICPixelFormat64bppRGBA)
		return DXGI_FORMAT_R16G16B16A16_UNORM;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
		if (format == GUID_WICPixelFormat96bppRGBFloat)
		return DXGI_FORMAT_R32G32B32_FLOAT;
#endif

		if (format == GUID_WICPixelFormat32bppRGBA)
		return DXGI_FORMAT_R8G8B8A8_UNORM;
		if (format == GUID_WICPixelFormat32bppBGRA)
		return DXGI_FORMAT_B8G8R8A8_UNORM;
		if (format == GUID_WICPixelFormat32bppBGR)
		return DXGI_FORMAT_B8G8R8X8_UNORM;

		if (format == GUID_WICPixelFormat32bppRGBA1010102XR)
		return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
		if (format == GUID_WICPixelFormat32bppRGBA1010102)
		return DXGI_FORMAT_R10G10B10A2_UNORM;

		if (format == GUID_WICPixelFormat16bppBGRA5551)
		return DXGI_FORMAT_B5G5R5A1_UNORM;
		if (format == GUID_WICPixelFormat16bppBGR565)
		return DXGI_FORMAT_B5G6R5_UNORM;

		if (format == GUID_WICPixelFormat32bppGrayFloat)
		return DXGI_FORMAT_R32_FLOAT;
		if (format == GUID_WICPixelFormat16bppGrayHalf)
		return DXGI_FORMAT_R16_FLOAT;
		if (format == GUID_WICPixelFormat16bppGray)
		return DXGI_FORMAT_R16_UNORM;
		if (format == GUID_WICPixelFormat8bppGray)
		return DXGI_FORMAT_R8_UNORM;
		if (format == GUID_WICPixelFormat8bppAlpha)
		return DXGI_FORMAT_A8_UNORM;

		return DXGI_FORMAT_UNKNOWN;
	}
	static WICPixelFormatGUID GetConvertToWICFormat(const WICPixelFormatGUID& wicFormatGUID)
	{
		if (wicFormatGUID == GUID_WICPixelFormatBlackWhite)
		return GUID_WICPixelFormat8bppGray;
		else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat2bppGray)
		return GUID_WICPixelFormat8bppGray;
		else if (wicFormatGUID == GUID_WICPixelFormat4bppGray)
		return GUID_WICPixelFormat8bppGray;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint)
		return GUID_WICPixelFormat16bppGrayHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint)
		return GUID_WICPixelFormat32bppGrayFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555)
		return GUID_WICPixelFormat16bppBGRA5551;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010)
		return GUID_WICPixelFormat32bppRGBA1010102;
		else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint)
		return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint)
		return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint)
		return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint)
		return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint)
		return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf)
		return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf)
		return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat)
		return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat)
		return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint)
		return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint)
		return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE)
		return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha)
		return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
		else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB)
		return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB)
		return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf)
		return GUID_WICPixelFormat64bppRGBAHalf;
#endif

		else return GUID_WICPixelFormatDontCare;
	}
	static uint32 GetDXGIFormatBitsPerPixel(const DXGI_FORMAT& dxgiFormat)
	{
		if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
		return 128;
		else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT)
		return 64;
		else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM)
		return 64;
		else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
		return 32;
		else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM)
		return 32;
		else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM)
		return 32;
		else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
		return 32;

		else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
		return 32;
		else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM)
		return 16;
		else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM)
		return 16;
		else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT)
		return 32;
		else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT)
		return 16;
		else if (dxgiFormat == DXGI_FORMAT_R16_UNORM)
		return 16;
		else if (dxgiFormat == DXGI_FORMAT_R8_UNORM)
		return 8;
		else if (dxgiFormat == DXGI_FORMAT_A8_UNORM)
		return 8;
		else
		{
			DK_ASSERT_LOG(false, "올바르지 않은 Texture Type입니다. 확인 요망!");
			return 0xffffffff;
		}
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
#if defined(_DK_DEBUG_)
#include "GpuAssertDiagnostics.h"
#endif
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

	uint32 RenderModule::kCurrentFrameIndex = 0;
	uint32 RenderModule::kCurrentBackBufferIndex = 0;
	uint32 RenderModule::kWidth = 0;
	uint32 RenderModule::kHeight = 0;
	static constexpr const DXGI_FORMAT gRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT GetDepthResourceFormat(DXGI_FORMAT depthformat)
	{
		DXGI_FORMAT resformat = DXGI_FORMAT_UNKNOWN;
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
		DXGI_FORMAT srvformat = DXGI_FORMAT_UNKNOWN;
		switch (depthformat)
		{
		case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
			srvformat = DXGI_FORMAT::DXGI_FORMAT_R16_UNORM;
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
		if (initialize_createDeviceAndCommandQueueAndSwapChain(hwnd, width, height) == false)
			return false;

		if (createCommandList(_commandList) == false)
			return false;

		if (initialize_createFence() == false)
			return false;

		if (createBackBufferTargets(width, height) == false)
			return false;

		kWidth = width;
		kHeight = height;

#if defined(_DK_DEBUG_)
		const HRESULT timestampResult = _commandQueue->GetTimestampFrequency(&_gpuTimestampFrequency);
		DK_ASSERT_LOG(SUCCEEDED(timestampResult) && _gpuTimestampFrequency > 0, "GPU timestamp frequency is unavailable");
		if (FAILED(timestampResult) || _gpuTimestampFrequency == 0)
			return false;

		const GpuAssertRecord debugHangData = {};
		const uint32 debugHangBufferSize = sizeof(debugHangData);
		const CD3DX12_HEAP_PROPERTIES debugHangHeap(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC debugHangDesc = CD3DX12_RESOURCE_DESC::Buffer( debugHangBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		ID3D12Resource* debugHangResource = nullptr;
		HRESULT debugHangHr = _device->CreateCommittedResource(&debugHangHeap, D3D12_HEAP_FLAG_NONE, &debugHangDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&debugHangResource));
		if (FAILED(debugHangHr))
			return false;

		IBufferRef debugHangUpload = createUploadBuffer(debugHangBufferSize, DK_DEBUG_NAME(L"DebugHangBuffer Upload"));
		if (debugHangUpload == nullptr || debugHangUpload->isValid() == false)
			return false;

		_gpuAssertResetUpload = debugHangUpload;
		_gpuAssertBuffer = IBufferRef(dk_new IBuffer(IBuffer::Type::DEFAULT, RenderResourcePtr<ID3D12Resource>(debugHangResource), debugHangBufferSize, D3D12_RESOURCE_STATE_COMMON));
		debugHangResource->SetName(L"GpuAssertBuffer");
		const CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);
		const CD3DX12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(debugHangBufferSize);
		ID3D12Resource* readback = nullptr;
		if (FAILED(_device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback))))
			return false;
		_gpuAssertReadback = readback;
		readback->SetName(L"GpuAssert Readback");

		void* debugHangMapped = nullptr;
		const D3D12_RANGE noRead = { 0, 0 };
		debugHangHr = debugHangUpload->_buffer->Map(0, &noRead, &debugHangMapped);
		if (FAILED(debugHangHr))
			return false;

		DK::memcpy(debugHangMapped, &debugHangData, debugHangBufferSize);
		const D3D12_RANGE writtenRange = { 0, debugHangBufferSize };
		debugHangUpload->_buffer->Unmap(0, &writtenRange);

		resourceBarrierTransition(*_gpuAssertBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
		_commandList->CopyBufferRegion(debugHangResource, 0, debugHangUpload->_buffer.get(), 0, debugHangBufferSize);

		resourceBarrierTransition(*_gpuAssertBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif

		execute();
		waitFenceAndResetCommandList();
		resetCommandList();

		return true;
	}

	bool RenderModule::createBackBufferTargets(uint32 width, uint32 height)
	{
		RenderPassTargets targets[kBackBufferCount];
		D3D12_CPU_DESCRIPTOR_HANDLE view = _backBufferRtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (uint32 i = 0; i < kBackBufferCount; ++i)
		{
			ID3D12Resource* resource = nullptr;
			if (FAILED(_swapChain->GetBuffer(i, IID_PPV_ARGS(&resource))))
				return false;

			const D3D12_RESOURCE_DESC desc = resource->GetDesc();
			IBuffer buffer(IBuffer::Type::BACKBUFFER, RenderResourcePtr<ID3D12Resource>(resource), 0, D3D12_RESOURCE_STATE_PRESENT);
			_device->CreateRenderTargetView(resource, nullptr, view);
			targets[i]._colors[0] = { ITextureRef(dk_new ITexture(DK_DEBUG_NAME("BackBuffer"), 1, desc.Format, DK::move(buffer))), view };
			targets[i]._colorCount = 1;
			targets[i]._width = width;
			targets[i]._height = height;
			view.ptr += _renderTargetViewSize;
		}

		for (uint32 i = 0; i < kBackBufferCount; ++i)
			_backBufferPassTargets[i] = DK::move(targets[i]);

		return true;
	}

	bool RenderModule::createRenderPassTargets(const char* debugName, const RenderTargetDefinition& definition, uint32 width, uint32 height, RenderPassTargets& targets)
	{
		targets._width = width;
		targets._height = height;
		targets._colorCount = definition._colorCount;
#if defined(_DK_DEBUG_)
		const DKStringW resourceName = StringUtil::convertCtoWC(debugName);
		const wchar_t* resourceDebugName = resourceName.c_str();
#endif
		D3D12_DESCRIPTOR_HEAP_DESC heap{};
		if (definition._colorCount != 0)
		{
			heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heap.NumDescriptors = definition._colorCount;
			ID3D12DescriptorHeap* rtvHeap = nullptr;
			if (FAILED(_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&rtvHeap))))
				return false;

			targets._rtvHeap = rtvHeap;

			D3D12_CPU_DESCRIPTOR_HANDLE view = rtvHeap->GetCPUDescriptorHandleForHeapStart();
			for (uint32 i = 0; i < definition._colorCount; ++i)
			{
				const RenderTargetDefinition::Attachment& color = definition._colors[i];
				D3D12_CLEAR_VALUE clear{};
				clear.Format = color._format;
				IBufferRef buffer = createBuffer2DInternal(width, height, 1, color._format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, DK_DEBUG_NAME(resourceDebugName));
				if (buffer == nullptr)
					return false;

				_device->CreateRenderTargetView(buffer->_buffer.get(), nullptr, view);
				RenderPassAttachment& attachment = targets._colors[i];
				attachment = { ITextureRef(dk_new ITexture(DK_DEBUG_NAME(debugName), 1, color._format, DK::move(buffer))), view };
				if (color._srv && allocateTextureSRV(*attachment._texture) == false)
					return false;

				view.ptr += _renderTargetViewSize;
			}
		}

		const RenderTargetDefinition::Attachment& depth = definition._depth;
		if (depth._format == DXGI_FORMAT_UNKNOWN)
			return true;

		heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heap.NumDescriptors = 1;
		ID3D12DescriptorHeap* dsvHeap = nullptr;
		if (FAILED(_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&dsvHeap))))
			return false;

		targets._dsvHeap = dsvHeap;

		D3D12_CLEAR_VALUE clear{};
		clear.Format = depth._format;
		clear.DepthStencil.Depth = 0.0f;
		clear.DepthStencil.Stencil = 0;
		const D3D12_RESOURCE_FLAGS flags = depth._srv ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		const DXGI_FORMAT resourceFormat = depth._srv ? GetDepthResourceFormat(depth._format) : depth._format;
		IBufferRef buffer = createBuffer2DInternal(width, height, 1, resourceFormat, flags, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, DK_DEBUG_NAME(resourceDebugName));
		if (buffer == nullptr)
			return false;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
		dsv.Format = depth._format;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		const D3D12_CPU_DESCRIPTOR_HANDLE view = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_device->CreateDepthStencilView(buffer->_buffer.get(), &dsv, view);
		const DXGI_FORMAT srvFormat = depth._srv ? GetDepthSRVFormat(depth._format) : depth._format;
		targets._depth = { ITextureRef(dk_new ITexture(DK_DEBUG_NAME(debugName), 1, srvFormat, DK::move(buffer))), view };
		return depth._srv == false || allocateTextureSRV(*targets._depth._texture);
	}

	bool RenderModule::createRenderTarget(const DKString& name, RenderTargetDefinition&& definition)
	{
		if (_renderTargets.find(StringUtil::hash(name)) != _renderTargets.end())
			return false;

		RenderTarget target;
#if defined(_DK_DEBUG_)
		target._name = name;
#endif
		using Size = RenderTargetDefinition::Size;
		if (definition._size == Size::SwapChain)
		{
			definition._colorCount = 1;
			definition._colors[0]._format = _backBufferPassTargets[0]._colors[0]._texture->getFormat();
		}
		else
		{
			const uint32 width = definition._size == Size::Window ? kWidth : definition._width;
			const uint32 height = definition._size == Size::Window ? kHeight : definition._height;
			if (createRenderPassTargets(DK_DEBUG_NAME(name.c_str()), definition, width, height, target._targets) == false)
				return false;
		}

		target._definition = DK::move(definition);
		return _renderTargets.try_emplace(StringUtil::registerHash(name), DK::move(target)).second;
	}

	const RenderTarget* RenderModule::findRenderTarget(const char* name) const
	{
		const DKHashMap<uint64, RenderTarget>::const_iterator entry = _renderTargets.find(StringUtil::hash(name));
		return entry == _renderTargets.end() ? nullptr : &entry->second;
	}

	const bool RenderModule::resize(const uint32 width, const uint32 height)
	{
		if (width == 0 || height == 0)
			return false;

		waitAllGPU();
		DXGI_SWAP_CHAIN_DESC1 desc{};
		if (FAILED(_swapChain->GetDesc1(&desc)))
			return false;

		DKVector<DKPair<RenderTarget*, RenderPassTargets>> resized;
		for (DKHashMap<uint64, RenderTarget>::value_type& entry : _renderTargets)
		{
			RenderTarget& target = entry.second;
			if (target._definition._size != RenderTargetDefinition::Size::Window)
				continue;
			RenderPassTargets targets;
			if (createRenderPassTargets(DK_DEBUG_NAME(target._name.c_str()), target._definition, width, height, targets) == false)
				return false;
			resized.emplace_back(&target, DK::move(targets));
		}

		for (RenderPassTargets& backBuffer : _backBufferPassTargets)
			backBuffer = RenderPassTargets{};

		if (FAILED(_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, desc.Flags)))
		{
			if (createBackBufferTargets(desc.Width, desc.Height) == false)
				PostQuitMessage(-1);

			kCurrentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
			return false;
		}

		if (createBackBufferTargets(width, height) == false)
		{
			PostQuitMessage(-1);
			return false;
		}

		for (DKPair<RenderTarget*, RenderPassTargets>& entry : resized)
			entry.first->_targets = DK::move(entry.second);

		kWidth = width;
		kHeight = height;
		kCurrentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
		return true;
	}

	void RenderModule::destroy()
	{
		_renderPassInstances.clear();
		_renderPassMap.clear();
#if defined(_DK_DEBUG_)
		_gpuAssertBuffer.reset();
		_gpuAssertResetUpload.reset();
		_gpuAssertReadback.release();
		_gpuAssertReadbackPending = false;
		_gpuTimestampBlocks.clear();
		_gpuTimings.clear();
		_gpuTimingScopeStack.clear();
		_recordedGpuTimingSamples.clear();
		_submittedGpuTimingSamples.clear();
		_gpuTotalMilliseconds = 0.0;
		_gpuTotalPeakMilliseconds = 0.0;
		_gpuTimingsPending = false;
		_hasGpuTimings = false;
#endif
		_textureDescriptorHeap.release();
		DK_ASSERT_LOG(_textureContainer.empty(), "");

#if defined(USE_IMGUI)
		_imguiDescriptorHeap.release();
#endif

		_renderTargets.clear();
		for (RenderPassTargets& backBuffer : _backBufferPassTargets)
			backBuffer = RenderPassTargets{};
		_backBufferRtvHeap.release();

		_swapChain.release();

		CloseHandle(_fenceEvent);
		for (uint32 i = 0; i < DK_COUNT_OF(_fences); ++i)
			_fences[i].release();

		_commandList = DKCommandList();
		_commandQueue.release();

		_device.release();
#if defined(_DK_DEBUG_)
		ReportLiveObjects();
#endif
	}

	bool CheckTearingSupport(RenderResourcePtr<IDXGIFactory4>& factory)
	{
		HRESULT hr;
		BOOL allowTearing = FALSE;
		{
			IDXGIFactory5* factory5 = nullptr;
			hr = factory->QueryInterface(IID_PPV_ARGS(&factory5));
			if (SUCCEEDED(hr))
			{
				hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
				factory5->Release();
			}
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
#if defined(_DK_DEBUG_)
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
			ID3D12Debug* debugController = nullptr;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				debugController->Release();
			}
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

#if defined(SUPPORT_HLGL_DEBUGBREAK)
		D3D12_FEATURE_DATA_DEBUG_BREAK debugBreakSupport = {};
		HRESULT debugBreakHr = _device->CheckFeatureSupport(D3D12_FEATURE_DEBUG_BREAK, &debugBreakSupport, sizeof(debugBreakSupport));
		if (FAILED(debugBreakHr) || debugBreakSupport.HaltSupported == false)
		{
			DK_ASSERT_LOG(false, "GPU DebugBreak halt is unsupported.");
			return false;
		}
#endif

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

		// Query the device limit used by shader compilation, including shader reloads.
		{
			D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};

			shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_10;

			hr = _device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
			while (hr == E_INVALIDARG && shaderModel.HighestShaderModel > D3D_SHADER_MODEL_6_0)
			{
				shaderModel.HighestShaderModel = static_cast<D3D_SHADER_MODEL>(static_cast<int>(shaderModel.HighestShaderModel) - 1);
				hr = _device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
			}

			if (FAILED(hr) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0)
			{
				DK_LOG("Shader Model 6.0 or higher is required (query HRESULT: 0x%08X).", static_cast<unsigned int>(hr));
				return false;
			}

			_shaderModel = shaderModel.HighestShaderModel;
			DK_LOG("Shader target model: %u.%u", static_cast<uint32>(_shaderModel) >> 4, static_cast<uint32>(_shaderModel) & 0xf);
		}

		// Create CommandQueue
		{
			D3D12_COMMAND_QUEUE_DESC cqDesc = {};
			cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			ID3D12CommandQueue* commandQueue;
			if (FAILED(_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue))))
				return false;

			_commandQueue = commandQueue;
#if defined(_DK_DEBUG_)
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

			_bindlessViewSize = _device->GetDescriptorHandleIncrementSize(gTextureBindlessDescriptorHeapType);

			_textureDescriptorHeap = textureDescriptorHeap;
#if defined(_DK_DEBUG_)
			_textureDescriptorHeap->SetName(L"BindlessTextureDescriptorHeap");
#endif
		}

		// Back-buffer views share a heap. XML-owned targets allocate their own views.
		{
			D3D12_DESCRIPTOR_HEAP_DESC heap{};
			heap.NumDescriptors = kBackBufferCount;
			heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			ID3D12DescriptorHeap* rtvHeap = nullptr;
			if (FAILED(_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&rtvHeap))))
				return false;

			_backBufferRtvHeap = rtvHeap;
			_renderTargetViewSize = _device->GetDescriptorHandleIncrementSize(heap.Type);
		}

		// Create SwapChain and BackBuffer RTV
		{
			DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.Width = width;
			swapChainDesc.Height = height;
			swapChainDesc.Format = gRenderTargetFormat;
			swapChainDesc.Stereo = FALSE;
			swapChainDesc.SampleDesc = sampleDesc;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = kBackBufferCount;
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
		}

		// CreateSwapChainForHwnd뒤에 호출되어야함
		// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
		// will be handled manually.
		hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		if (SUCCEEDED(hr) == false)
			return false;

#if defined(USE_IMGUI)
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
		io.ConfigDpiScaleFonts = true;
		io.ConfigDpiScaleViewports = true;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		ImFontConfig fontConfig;
		fontConfig.SizePixels = 15.0f;
		io.Fonts->AddFontDefault(&fontConfig);

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.FontSizeBase = 15.0f;
		style.WindowPadding = ImVec2(12.0f, 12.0f);
		style.FramePadding = ImVec2(8.0f, 5.0f);
		style.ItemSpacing = ImVec2(8.0f, 8.0f);
		style.CellPadding = ImVec2(0.0f, 3.0f);
		style.WindowRounding = 0.0f; // Match native detached window edges.
		style.FrameRounding = 4.0f;
		style.GrabRounding = 4.0f;
		style.TabRounding = 4.0f;
		style.ScrollbarSize = 12.0f;
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.085f, 0.10f, 1.0f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.12f, 0.14f, 1.0f);
		style.Colors[ImGuiCol_Text] = ImVec4(0.88f, 0.90f, 0.92f, 1.0f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.49f, 0.55f, 0.60f, 1.0f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.22f, 0.26f, 0.29f, 0.6f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.12f, 0.14f, 1.0f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.17f, 0.19f, 1.0f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.16f, 0.19f, 1.0f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.25f, 0.28f, 1.0f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.31f, 0.33f, 1.0f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.22f, 0.25f, 1.0f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.34f, 0.37f, 1.0f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.43f, 0.44f, 1.0f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.13f, 0.18f, 0.21f, 1.0f);
		style.Colors[ImGuiCol_HeaderHovered] = style.Colors[ImGuiCol_ButtonHovered];
		style.Colors[ImGuiCol_HeaderActive] = style.Colors[ImGuiCol_ButtonActive];
		style.Colors[ImGuiCol_Tab] = style.Colors[ImGuiCol_WindowBg];
		style.Colors[ImGuiCol_TabSelected] = ImVec4(0.16f, 0.26f, 0.28f, 1.0f);
		style.Colors[ImGuiCol_TabHovered] = style.Colors[ImGuiCol_ButtonHovered];
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.78f, 0.72f, 1.0f);
		style.Colors[ImGuiCol_SliderGrab] = style.Colors[ImGuiCol_CheckMark];
		style.Colors[ImGuiCol_TabSelectedOverline] = style.Colors[ImGuiCol_CheckMark];

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = gTextureBindlessDescriptorHeapType;
			desc.NumDescriptors = kImGuiDescriptorCount;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ID3D12DescriptorHeap* imguiHeap;
			if (_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiHeap)) != S_OK)
			{
				DK_ASSERT_LOG(false, "");
				return false;
			}
			_imguiDescriptorHeap = imguiHeap;

			if (ImGui_ImplWin32_Init(hwnd) == false)
				return false;
			ImGui_ImplDX12_InitInfo initInfo;
			initInfo.Device = _device.get();
			initInfo.CommandQueue = _commandQueue.get();
			// The backend also uses this count for flip-model viewport swap chains (minimum two).
			initInfo.NumFramesInFlight = Math::max(kFrameCount, kBackBufferCount);
			initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			initInfo.SrvDescriptorHeap = _imguiDescriptorHeap.get();
			initInfo.UserData = this;
			initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
			{
				RenderModule& renderer = *static_cast<RenderModule*>(info->UserData);
				const UINT stride = info->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (uint32 i = 0; i < kImGuiDescriptorCount; ++i)
				{
					if (renderer._imguiDescriptorUsed[i])
						continue;
					renderer._imguiDescriptorUsed[i] = true;
					*cpu = info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
					*gpu = info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
					cpu->ptr += static_cast<SIZE_T>(i) * stride;
					gpu->ptr += static_cast<UINT64>(i) * stride;
					return;
				}
				// The backend requires a valid descriptor; never alias a live slot.
				DK_ASSERT_LOG(false, "ImGui descriptor capacity exceeded");
				return;
			};
			initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE)
			{
				RenderModule& renderer = *static_cast<RenderModule*>(info->UserData);
				const UINT stride = info->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				const SIZE_T index = (cpu.ptr - info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr) / stride;
				assert(index < kImGuiDescriptorCount && renderer._imguiDescriptorUsed[index]);
				renderer._imguiDescriptorUsed[index] = false;
			};
			if (ImGui_ImplDX12_Init(&initInfo) == false)
				return false;
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
		ID3D12CommandAllocator* commandAllocator[kBackBufferCount] = { nullptr, };
		for (uint32 i = 0; i < kBackBufferCount; ++i)
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
			for (uint32 i = 0; i < kFrameCount; ++i)
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
				if (setting._bufferNameHash == resource._nameHash)
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

				for (const ShaderVariableReflection& variable : resource._variables)
				{
					RootConstant32BitParameterBindingInfo info;
					info._rootParameterIndex = rootIndex;
					info._offset = variable._offset;

					inoutPipeline._rootConstant32BitParameterMap.insert(DKPair<uint64, RootConstant32BitParameterBindingInfo>(variable._nameHash, DK::move(info)));
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
				case D3D_SIT_UAV_RWBYTEADDRESS:
				{
					const uint32 rootIndex = static_cast<uint32>(rootParameters.size());

					CD3DX12_ROOT_PARAMETER root;
					if (resource._type == D3D_SIT_CBUFFER)
						root.InitAsConstantBufferView(resource._register, resource._space, D3D12_SHADER_VISIBILITY_ALL);
					else if (resource._type == D3D_SIT_UAV_RWBYTEADDRESS)
						root.InitAsUnorderedAccessView(resource._register, resource._space, D3D12_SHADER_VISIBILITY_ALL);
					else
						root.InitAsShaderResourceView(resource._register, resource._space, D3D12_SHADER_VISIBILITY_ALL);
					rootParameters.push_back(root);

					ShaderParameter parameter;
					parameter._type = ShaderParameterType::Buffer;
					parameter._register = resource._register;
					parameter._space = resource._space;
					parameter._rootParameterIndex = rootIndex;

					inoutPipeline._shaderParameterMap.insert(DKPair<uint64, ShaderParameter>(resource._nameHash, DK::move(parameter)));
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

		ID3DBlob* signatureBlob = nullptr;
#if defined(_DK_DEBUG_)
		ID3DBlob* errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		RenderResourcePtr<ID3DBlob> errorLog = errorBlob;
#else
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, nullptr);
#endif
		RenderResourcePtr<ID3DBlob> signature = signatureBlob;
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "Root signature serialization failed: %s", errorBlob != nullptr ? static_cast<const char*>(errorBlob->GetBufferPointer()) : "No diagnostics");
			return false;
		}

		ID3D12RootSignature* rootSignature = nullptr;
		hr = _device->CreateRootSignature(0, signature->GetBufferPointer(), static_cast<uint32>(signature->GetBufferSize()), IID_PPV_ARGS(&rootSignature));
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "Root signature creation failed");
			return false;
		}

		inoutPipeline._rootSignature = rootSignature;
#if defined(_DK_DEBUG_)
		inoutPipeline._rootSignature->SetName(StringUtil::convertCtoWC(inoutPipeline._createInfo._pipelineName.c_str()).c_str());
#endif

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
				ShaderType::VertexShader, pipelineCreateInfo._defines,
				vertexShader, vertexShaderView, pipelineResources
			);
			if (success == false)
				return false;
			D3D12_SHADER_BYTECODE pixelShaderView = {};
			RenderResourcePtr<IDxcBlob> pixelShader = nullptr;
			success = shaderCompiler.compileShader(
				pipelineCreateInfo._pixelShaderPath.c_str(), pipelineCreateInfo._pixelShaderEntry.c_str(),
				ShaderType::PixelShader, pipelineCreateInfo._defines,
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
			depthStencilDesc.DepthFunc = depthStencilDesc.DepthEnable ? D3D12_COMPARISON_FUNC_GREATER : D3D12_COMPARISON_FUNC_NEVER;
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
			DK_ASSERT_LOG(pipelineCreateInfo._target != nullptr, "Graphics pipeline requires a render target");
			const RenderTargetDefinition& target = pipelineCreateInfo._target->_definition;
			for (uint32 i = 0; i < target._colorCount; ++i)
				psoDesc.RTVFormats[i] = target._colors[i]._format;
			psoDesc.SampleDesc = sampleDesc;
			psoDesc.SampleMask = 0xffffffff;
			psoDesc.RasterizerState = rasterizerDesc;
			psoDesc.BlendState = blendDesc;
			psoDesc.NumRenderTargets = target._colorCount;
			psoDesc.DepthStencilState = depthStencilDesc;
			psoDesc.DSVFormat = (depthStencilDesc.DepthEnable || depthStencilDesc.StencilEnable) ? target._depth._format : DXGI_FORMAT_UNKNOWN;
#if defined(SUPPORT_HLGL_DEBUGBREAK)
			psoDesc.Flags |= D3D12_PIPELINE_STATE_FLAG_DEBUG_BREAK_ALWAYS_HALT;
#endif

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
				ShaderType::ComputeShader, pipelineCreateInfo._defines,
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
#if defined(SUPPORT_HLGL_DEBUGBREAK)
			cpsoDesc.Flags |= D3D12_PIPELINE_STATE_FLAG_DEBUG_BREAK_ALWAYS_HALT;
#endif

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
			if (supportsRaytracing() == false)
			{
				inoutPipeline._type = Pipeline::Type::RAYTRACING;
#if defined(_DK_DEBUG_)
				inoutPipeline._createInfo = pipelineCreateInfo;
#endif
				return true;
			}
			if (pipelineCreateInfo._raygenEntry.empty() || pipelineCreateInfo._missEntry.empty() ||
				pipelineCreateInfo._closestEntry.empty() || pipelineCreateInfo._hitGroupExport.empty() ||
				pipelineCreateInfo._maxAttributeSize > D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES ||
				pipelineCreateInfo._maxTraceRecursionDepth > D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH)
			{
				DK_LOG("Invalid raytracing pipeline configuration.");
				return false;
			}
			inoutPipeline._type = Pipeline::Type::RAYTRACING;

			// 이 Scope Stack내(CreateStateObject호출까지) 유지되어야 해서 이 곳에서 Stack에 할당함
			RenderResourcePtr<IDxcBlob> raygenShaderBlob;
			RenderResourcePtr<IDxcBlob> missShaderBlob;
			RenderResourcePtr<IDxcBlob> cloesetShaderBlob;
			const DKStringW raygenEntry = StringUtil::convertCtoWC(pipelineCreateInfo._raygenEntry.c_str()).c_str();
			const DKStringW missEntry = StringUtil::convertCtoWC(pipelineCreateInfo._missEntry.c_str()).c_str();
			const DKStringW closestEntry = StringUtil::convertCtoWC(pipelineCreateInfo._closestEntry.c_str()).c_str();
			const DKStringW hitGroupExport = StringUtil::convertCtoWC(pipelineCreateInfo._hitGroupExport.c_str()).c_str();

			DKVector<D3D12_DXIL_LIBRARY_DESC> libraryDesc;
			DKVector<DKVector<D3D12_EXPORT_DESC>> exportDesc;
			{
				D3D12_SHADER_BYTECODE raygenShaderView = {};
				const bool success = shaderCompiler.compileShader(
					pipelineCreateInfo._raygenShaderPath.c_str(), "",
					ShaderType::Raytracing, pipelineCreateInfo._defines,
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
					ShaderType::Raytracing, pipelineCreateInfo._defines,
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
					ShaderType::Raytracing, pipelineCreateInfo._defines,
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
			hitGroup.HitGroupExport = hitGroupExport.c_str();
			hitGroup.ClosestHitShaderImport = closestEntry.c_str();
			hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

			// ShaderConfig
			D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
			shaderConfig.MaxPayloadSizeInBytes = pipelineCreateInfo._maxPayloadSize;
			shaderConfig.MaxAttributeSizeInBytes = pipelineCreateInfo._maxAttributeSize;

			// PipelineConfig
			D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
			pipelineConfig.MaxTraceRecursionDepth = pipelineCreateInfo._maxTraceRecursionDepth;

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
#if defined(SUPPORT_HLGL_DEBUGBREAK)
			D3D12_STATE_OBJECT_CONFIG debugConfig = {};
			debugConfig.Flags = D3D12_STATE_OBJECT_FLAG_DEBUG_BREAK_ALWAYS_HALT;
			D3D12_STATE_SUBOBJECT debugSubobject = {};
			debugSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG;
			debugSubobject.pDesc = &debugConfig;
			subobjects.push_back(debugSubobject);
#endif

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
			inoutPipeline._raytracing._stateObject = pso;

			// StateObject Properties
			ID3D12StateObjectProperties* psoProp;
			hr = inoutPipeline._raytracing._stateObject->QueryInterface(IID_PPV_ARGS(&psoProp));
			if (FAILED(hr))
			{
				DK_ASSERT_LOG(false, "Failed Raytracing QueryInterface");
				return false;
			}
			inoutPipeline._raytracing._properties = psoProp;
			if (createShaderBindingTable(pipelineCreateInfo, inoutPipeline._raytracing) == false)
				return false;
#if defined(_DK_DEBUG_)
			inoutPipeline._createInfo = pipelineCreateInfo;
			inoutPipeline._raytracing._stateObject->SetName(StringUtil::convertCtoWC(inoutPipeline._createInfo._pipelineName.c_str()).c_str());
#endif
		}
		else
		{
			DK_ASSERT_LOG(false, "올바르지 않은 Pipeline Type 지정");
			return false;
		}

		return true;
	}
	bool RenderModule::createShaderBindingTable(const Pipeline::CreateInfo& createInfo, RaytracingPipelineData& pipeline)
	{
		// One record per table; scene/material data is accessed through bindless resources.
		uint64 (*const alignUp)(uint64, uint64) = [](uint64 value, uint64 alignment)
		{
			return (value + alignment - 1) & ~(alignment - 1);
		};
		const uint64 recordSize = alignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		const uint64 tableSpacing = alignUp(recordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		const uint32 bufferSize = static_cast<uint32>(tableSpacing * 3);
		const DKStringW exports[] = {
			StringUtil::convertCtoWC(createInfo._raygenEntry.c_str()).c_str(),
			StringUtil::convertCtoWC(createInfo._missEntry.c_str()).c_str(),
			StringUtil::convertCtoWC(createInfo._hitGroupExport.c_str()).c_str()
		};
		const void* identifiers[3] = {};
		for (uint32 i = 0; i < 3; ++i)
		{
			identifiers[i] = pipeline._properties->GetShaderIdentifier(exports[i].c_str());
			if (identifiers[i] == nullptr)
			{
				DK_ASSERT_LOG(false, "Missing raytracing shader identifier.");
				return false;
			}
		}

		ShaderBindingTable sbt;
		sbt._buffer = createUploadBuffer(bufferSize, DK_DEBUG_NAME(L"Shader Binding Table"));
		if (sbt._buffer == nullptr || sbt._buffer->isValid() == false)
			return false;
		// A new, unpublished buffer can be filled immediately, including during reload.
		void* mapped = nullptr;
		D3D12_RANGE readRange = { 0, 0 };
		if (FAILED(sbt._buffer->_buffer->Map(0, &readRange, &mapped)))
			return false;
		memset(mapped, 0, bufferSize);
		for (uint32 i = 0; i < 3; ++i)
			DK::memcpy(static_cast<uint8*>(mapped) + tableSpacing * i, identifiers[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		sbt._buffer->_buffer->Unmap(0, nullptr);
		sbt._rayGeneration = { 0, recordSize, recordSize };
		sbt._miss = { tableSpacing, recordSize, recordSize };
		sbt._hitGroup = { tableSpacing * 2, recordSize, recordSize };
		pipeline._sbt = DK::move(sbt);
		return true;
	}

	bool RenderModule::createRenderPass(ShaderCompiler& shaderCompiler, const DKString& renderPassName, RenderPass::CreateInfo&& renderPassCreateInfo)
	{
		using FindResult = DKHashMap<uint64, RenderPass>::iterator;
		FindResult find = _renderPassMap.find(StringUtil::hash(renderPassName));

		if (find != _renderPassMap.end())
		{
			DK_ASSERT_LOG(false, "중복된 이름의 RenderPass가 있습니다.\nName: %s", renderPassName.c_str());
			return false;
		}
		using InsertResult = DKPair<DKHashMap<uint64, RenderPass>::iterator, bool>;
		InsertResult insertResult = _renderPassMap.insert(DKPair<uint64, RenderPass>(StringUtil::registerHash(renderPassName), RenderPass()));
		if (insertResult.second == false)
		{
			DK_ASSERT_LOG(false, "HashMap Insert에 실패!");
			return false;
		}

		using PipelineCreateInfoIter = DKVector<DKPair<DKString, Pipeline::CreateInfo>>;
		RenderPass& renderPass = insertResult.first->second;
		renderPass._binding = renderPassCreateInfo._binding;

		const uint32 pipelineCount = static_cast<uint32>(renderPassCreateInfo._pipelineArr.size());
		for (uint32 i = 0; i < pipelineCount; ++i)
		{
			const DKString& pipelineName = renderPassCreateInfo._pipelineArr[i].first;

			using PipelineFindResult = DKHashMap<uint64, Pipeline>::iterator;
			PipelineFindResult foundPipeline = renderPass._pipelineMap.find(StringUtil::hash(pipelineName));
			if (foundPipeline != renderPass._pipelineMap.end())
			{
				DK_ASSERT_LOG(false, "중복된 이름의 Pipeline(%s)이 같은 RenderPass(%s) 내에 있습니다.", pipelineName.c_str(), renderPassName.c_str());
				return false;
			}

			Pipeline::CreateInfo& pipelineCreateInfo = renderPassCreateInfo._pipelineArr[i].second;

			Pipeline newPipeline;
			if (createPipelineObjectState(shaderCompiler, pipelineCreateInfo, newPipeline) == false)
				return false;

			renderPass._pipelineMap.insert(DKPair<uint64, Pipeline>(StringUtil::registerHash(pipelineName), DK::move(newPipeline)));
		}

#if defined(_DK_DEBUG_)
		renderPass._gpuTimingIndex = registerGpuTiming(renderPassName);
#endif
		return true;
	}

	RenderPass* RenderModule::findRenderPass(const char* name)
	{
		const DKHashMap<uint64, RenderPass>::iterator entry = _renderPassMap.find(StringUtil::hash(name));
		return entry != _renderPassMap.end() ? &entry->second : nullptr;
	}

	const RenderPassInstance* RenderModule::findRenderPassInstance(const char* name) const
	{
		const DKHashMap<uint64, RenderPassInstance>::const_iterator entry = _renderPassInstances.find(StringUtil::hash(name));
		return entry != _renderPassInstances.end() ? &entry->second : nullptr;
	}

	bool RenderModule::createRenderPassInstance(const DKString& name, const DKString& definitionName, const RenderPassBinding& binding)
	{
		RenderPass* definition = findRenderPass(definitionName.c_str());
		if (definition == nullptr)
			return false;

		const RenderTarget* baseTarget = definition->_binding._target;
		if (baseTarget == nullptr || binding._target == nullptr)
			return false;

		const RenderTargetDefinition& baseLayout = baseTarget->_definition;
		const RenderTargetDefinition& layout = binding._target->_definition;
		if (baseLayout._colorCount != layout._colorCount || baseLayout._depth._format != layout._depth._format)
			return false;

		for (uint32 i = 0; i < layout._colorCount; ++i)
		{
			if (baseLayout._colors[i]._format != layout._colors[i]._format)
				return false;
		}

		const DKPair<DKHashMap<uint64, RenderPassInstance>::iterator, bool> result = _renderPassInstances.try_emplace(StringUtil::registerHash(name), RenderPassInstance{ definition, binding });
#if defined(_DK_DEBUG_)
		if (result.second)
			result.first->second._gpuTimingIndex = registerGpuTiming(name);
#endif
		return result.second;
	}

#pragma endregion

#if defined(_DK_DEBUG_)
	const bool RenderModule::reloadShader()
	{
		EnsureMainThread();
		// Called before recording any pipeline bindings for the new frame.
		if (gCurrentBindedRenderPass != nullptr || gCurrentBindedPipeline != nullptr)
			return false;
		ShaderCompiler shaderCompiler(_shaderModel);
		DKVector<DKPair<Pipeline*, Pipeline>> replacements;

		for (DKHashMap<uint64, RenderPass>::iterator iter = _renderPassMap.begin(); iter != _renderPassMap.end(); ++iter)
		{
			for (DKHashMap<uint64, Pipeline>::iterator pipelineIter = iter->second._pipelineMap.begin(); pipelineIter != iter->second._pipelineMap.end(); ++pipelineIter)
			{
				Pipeline replacement;
				if (createPipelineObjectState(shaderCompiler, pipelineIter->second._createInfo, replacement) == false)
					return false;
				replacements.push_back(DKPair<Pipeline*, Pipeline>(&pipelineIter->second, DK::move(replacement)));
			}
		}

		waitAllGPU();
		for (DKPair<Pipeline*, Pipeline>& replacement : replacements)
			*replacement.first = DK::move(replacement.second);

		DuckingEngine::getInstance().GetRaytracingRendererWritable().resetAccumulation();

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

	void RenderModule::waitForFence(uint32 frameIndex)
	{
		DK_ASSERT_LOG(_fences[frameIndex].get() != nullptr, "");

		const uint64 completed = _fences[frameIndex]->GetCompletedValue();
		DK_ASSERT_LOG(completed != UINT64_MAX, "");
		if (completed >= _fenceValues[frameIndex])
			return;

		if (
			FAILED(_fences[frameIndex]->SetEventOnCompletion(_fenceValues[frameIndex], _fenceEvent))
			|| WaitForSingleObject(_fenceEvent, INFINITE) != WAIT_OBJECT_0
			|| _fences[frameIndex]->GetCompletedValue() == UINT64_MAX)
		{
			DK_ASSERT_LOG(false, "D3D12 frame fence wait failed");
			return;
		}
	}

	void RenderModule::waitAllGPU()
	{
		for (uint32 i = 0; i < kFrameCount; ++i)
			waitForFence(i);
#if defined(_DK_DEBUG_)
		reportGpuAssert();
#endif
	}

	void RenderModule::resetCommandList()
	{
		ID3D12CommandAllocator* allocator = _commandList._commandAllocators[kCurrentBackBufferIndex].get();
		if (FAILED(allocator->Reset()) || FAILED(_commandList->Reset(allocator, nullptr)))
		{
			DK_ASSERT_LOG(false, "D3D12 command list reset failed");
		}

		_commandList._lastResetIndex = kCurrentBackBufferIndex;
	}

	void RenderModule::execute(const bool isPresent)
	{
#if defined(_DK_DEBUG_)
		if (_gpuAssertBuffer && _gpuAssertReadback && _gpuAssertResetUpload)
		{
			resourceBarrierTransition(*_gpuAssertBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
			_commandList->CopyBufferRegion(_gpuAssertReadback.get(), 0, _gpuAssertBuffer->_buffer.get(), 0, sizeof(GpuAssertRecord));
			resourceBarrierTransition(*_gpuAssertBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
			_commandList->CopyBufferRegion(_gpuAssertBuffer->_buffer.get(), 0, _gpuAssertResetUpload->_buffer.get(), 0, sizeof(GpuAssertRecord));
			resourceBarrierTransition(*_gpuAssertBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			_gpuAssertReadbackPending = true;
		}
#endif
		if (FAILED(_commandList->Close()))
		{
			DK_ASSERT_LOG(false, "D3D12 command list close failed");
		}

		DKVector<ID3D12CommandList*> commandLists;
		commandLists.push_back(_commandList._commandList.get());
		_commandQueue->ExecuteCommandLists(static_cast<uint32>(commandLists.size()), commandLists.data());

		if (isPresent)
		{
#if defined(USE_IMGUI)
			// Submit secondary viewports after the main draw list and before the frame fence.
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
#endif
			UINT syncInterval = 0; //gVSync ? 1 : 0;
			UINT presentFlags = 0; // CheckTearingSupport() && !gVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
			HRESULT hr = _swapChain->Present(syncInterval, presentFlags);
			DK_ASSERT_LOG(SUCCEEDED(hr), "Present 실패!");
			kCurrentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
		}

		const uint64 fenceValue = _fenceValues[kCurrentFrameIndex] + 1;
		if (FAILED(_commandQueue->Signal(_fences[kCurrentFrameIndex].get(), fenceValue)))
		{
			DK_ASSERT_LOG(false, "D3D12 queue signal failed");
		}

		_fenceValues[kCurrentFrameIndex] = fenceValue;

#if defined(_DK_DEBUG_)
		if (isPresent)
		{
			DK_ASSERT_LOG(_gpuTimingsPending == false && _submittedGpuTimingSamples.empty(), "Previous GPU pass timings are pending");
			_submittedGpuTimingSamples.swap(_recordedGpuTimingSamples);
			_gpuTimingsPending = true;
		}
#endif
		_frameSubmitted = true;
	}

	void RenderModule::waitFenceAndResetCommandList()
	{
		if (_frameSubmitted == false)
			return;

		waitForFence(kCurrentFrameIndex);
		_frameSubmitted = false;
#if defined(_DK_DEBUG_)
		readGpuTimings();
		reportGpuAssert();
#endif
	}

#if defined(_DK_DEBUG_)
	void RenderModule::reportGpuAssert()
	{
		if (_gpuAssertReadbackPending == false || _gpuAssertReadback == nullptr)
			return;
		void* mapped = nullptr;
		const D3D12_RANGE readRange = { 0, sizeof(GpuAssertRecord) };
		const HRESULT hr = _gpuAssertReadback->Map(0, &readRange, &mapped);
		DK_ASSERT_LOG(SUCCEEDED(hr), "GPU assert readback failed: 0x%08X", static_cast<unsigned int>(hr));
		if (FAILED(hr))
			return;
		GpuAssertRecord record;
		DK::memcpy(&record, mapped, sizeof(record));
		const D3D12_RANGE noWrites = { 0, 0 };
		_gpuAssertReadback->Unmap(0, &noWrites);
		_gpuAssertReadbackPending = false;
		if (record._failed)
		{
			OutputDebugStringA(formatGpuAssert(record).c_str());
			if (IsDebuggerPresent())
				__debugbreak();
		}
	}
#endif

	void RenderModule::resourceBarrierTransition(ITextureRef& texture, const D3D12_RESOURCE_STATES afterState)
	{
		DK_ASSERT_LOG(texture != nullptr, "");
		resourceBarrierTransition(texture->getTextureBuffer(), afterState);
	}
	void RenderModule::resourceBarrierTransition(IBufferRef& buffer, const D3D12_RESOURCE_STATES afterState)
	{
		DK_ASSERT_LOG(buffer != nullptr, "");
		resourceBarrierTransition(*buffer.get(), afterState);
	}
	void RenderModule::resourceBarrierTransition(IBuffer& buffer, const D3D12_RESOURCE_STATES afterState)
	{
		if (buffer._currentState == afterState || (afterState != D3D12_RESOURCE_STATE_COMMON && (buffer._currentState & afterState) == afterState))
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
		RenderResourcePtr<ID3D12Resource> buffer = createBufferInternalRaw(_device, alignedSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, DK_DEBUG_NAME(debugName));
		return IBufferRef(dk_new IBuffer(IBuffer::Type::UPLOAD, DK::move(buffer), size, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	IBufferRef RenderModule::createUploadBuffer(const uint32 size, const wchar_t* debugName)
	{
		RenderResourcePtr<ID3D12Resource> buffer = createBufferInternalRaw(_device, size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, DK_DEBUG_NAME(debugName));
		return IBufferRef(dk_new IBuffer(IBuffer::Type::UPLOAD, DK::move(buffer), size, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	IBufferRef RenderModule::createStructuredBuffer(const void* data, uint32 elementSize, uint32 elementCount, const wchar_t* debugName)
	{
		const uint64 bufferSize = uint64(elementSize) * elementCount;
		DK_ASSERT_LOG(elementSize > 0 && elementCount > 0 && bufferSize <= UINT32_MAX, "Invalid structured buffer size");
		return createDefaultBuffer(data, static_cast<uint32>(bufferSize), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, DK_DEBUG_NAME(debugName));
	}

	IBufferRef RenderModule::createDefaultBuffer(const void* data, const uint32 bufferSize, const D3D12_RESOURCE_STATES state, const wchar_t* debugName)
	{
		DK_ASSERT_LOG(data != nullptr, "DefaultBuffer 생성시에는 반드시 Data가 있어야합니다.");

		RenderResourcePtr<ID3D12Resource> buffer = createBufferInternalRaw(_device, bufferSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, DK_DEBUG_NAME(debugName));
		IBufferRef defaultBuffer(dk_new IBuffer(IBuffer::Type::DEFAULT, DK::move(buffer), bufferSize, D3D12_RESOURCE_STATE_COMMON));
		if (defaultBuffer == nullptr)
			return nullptr;

		if (data != nullptr)
		{
			IBufferRef uploadBuffer = createUploadBuffer(bufferSize, DK_DEBUG_NAME(debugName));
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
		IBufferRef outBuffer = createDefaultBuffer(data, bufferSizeInBytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, DK_DEBUG_NAME(debugName));
		if (outBuffer == nullptr)
			return nullptr;

		D3D12_VERTEX_BUFFER_VIEW view;
		view.BufferLocation = outBuffer->_buffer->GetGPUVirtualAddress();
		view.StrideInBytes = strideSize;
		view.SizeInBytes = bufferSizeInBytes;
		outView = DK::makeRef<D3D12_VERTEX_BUFFER_VIEW>(view);

		return DK::move(outBuffer);
	}
	IBufferRef RenderModule::createIndexBuffer(const uint32* data, const uint32 indexCount, IndexBufferViewRef& outView, const wchar_t* debugName)
	{
		uint32 bufferSizeInBytes = sizeof(uint32) * indexCount;
		IBufferRef outBuffer = createDefaultBuffer(data, bufferSizeInBytes, D3D12_RESOURCE_STATE_INDEX_BUFFER, DK_DEBUG_NAME(debugName));
		if (outBuffer == nullptr)
			return nullptr;

		D3D12_INDEX_BUFFER_VIEW view;
		view.BufferLocation = outBuffer->_buffer->GetGPUVirtualAddress();
		view.Format = DXGI_FORMAT_R32_UINT;
		view.SizeInBytes = bufferSizeInBytes;
		outView = DK::makeRef<D3D12_INDEX_BUFFER_VIEW>(view);

		return DK::move(outBuffer);
	}

	const bool RenderModule::allocateTextureSRV(ITexture& texture)
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
			if (_currentTextureSRV >= kMaxTextureSRVCount)
				return false;
			index = _currentTextureSRV++;
		}

		DK_ASSERT_LOG(index < kMaxTextureSRVCount, "TextureSRV의 최대 개수를 초과했습니다. TextureSRV를 더 이상 할당할 수 없습니다.");
		DK_ASSERT_LOG(index < TEXTUREBINDLESS_MAX_COUNT, "TextureSRV의 최대 개수를 초과했습니다. TextureSRV를 더 이상 할당할 수 없습니다.");

		D3D12_CPU_DESCRIPTOR_HANDLE textureDescriptorHeapHandle = _textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texture.getFormat();
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texture.getMipLevelCount();
		textureDescriptorHeapHandle.ptr += index * _bindlessViewSize;
		_device->CreateShaderResourceView(texture._textureBuffer._buffer.get(), &srvDesc, textureDescriptorHeapHandle);

		texture._textureSRVIndex = index;

		return true;
	}

	const bool RenderModule::allocateTextureUAV(ITexture& texture)
	{
		EnsureMainThread();
		DK_ASSERT_LOG(texture.getMipLevelCount() == 1, "UAV로 접근할 Texture는 MipLevel이 1개인 경우에만 허용됩니다.");

		uint32 index;
		if (_deletedTextureUAVArr.empty() == false)
		{
			index = _deletedTextureUAVArr.back();
			_deletedTextureUAVArr.pop_back();
		}
		else
		{
			if (_currentTextureUAV >= kMaxTextureUAVCount)
				return false;
			index = _currentTextureUAV++;
		}

		DK_ASSERT_LOG(index < kMaxTextureUAVCount, "TextureUAV의 최대 개수를 초과했습니다. TextureUAV를 더 이상 할당할 수 없습니다.");
		DK_ASSERT_LOG(index < TEXTUREBINDLESS_MAX_COUNT, "TextureUAV의 최대 개수를 초과했습니다. TextureUAV를 더 이상 할당할 수 없습니다.");

		D3D12_CPU_DESCRIPTOR_HANDLE textureDescriptorHeapHandle = _textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = texture.getFormat();
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0; // UAV로 접근할 Mip 레벨 지정
		textureDescriptorHeapHandle.ptr += (index + kMaxTextureSRVCount) * _bindlessViewSize;
		_device->CreateUnorderedAccessView(texture._textureBuffer._buffer.get(), nullptr, &uavDesc, textureDescriptorHeapHandle);

		texture._textureUAVIndex = index;

		return true;
	}
	class TextureRaw
	{
	public:
		uint32 _width;
		uint32 _height;
		uint32 _bitsPerPixel;
		byte* _data = nullptr;
		// WIC decode buffers are local CPU owners; UpdateSubresources copies them.
		TextureRaw() = default;
		TextureRaw(const TextureRaw&) = delete;
		TextureRaw& operator=(const TextureRaw&) = delete;
		~TextureRaw()
		{
			dk_delete_array _data;
		}
		DXGI_FORMAT _format;
	};
	bool loadImageDataFromFile(const char* fileName, _OUT_ TextureRaw& textureRaw)
	{
		HRESULT hr;

		static IWICImagingFactory* wicFactory;

		Microsoft::WRL::ComPtr<IWICBitmapDecoder> wicDecoder;
		Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> wicFrame;
		Microsoft::WRL::ComPtr<IWICFormatConverter> wicConverter;

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
			wicDecoder.GetAddressOf()                      // the wic decoder to be created
		);
		if (FAILED(hr))
			return false;

		hr = wicDecoder->GetFrame(0, wicFrame.GetAddressOf());
		if (FAILED(hr))
			return false;

		WICPixelFormatGUID pixelFormat;
		hr = wicFrame->GetPixelFormat(&pixelFormat);
		if (FAILED(hr))
			return false;

		// get size of image
		hr = wicFrame->GetSize(&textureRaw._width, &textureRaw._height);
		if (FAILED(hr))
			return false;

		DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

		if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

			if (convertToPixelFormat == GUID_WICPixelFormatDontCare)
				return false;

			dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

			hr = wicFactory->CreateFormatConverter(wicConverter.GetAddressOf());
			if (FAILED(hr))
				return false;

			BOOL canConvert = FALSE;
			hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
			if (FAILED(hr) || canConvert == FALSE)
				return false;

			hr = wicConverter->Initialize(wicFrame.Get(), convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
			if (FAILED(hr))
				return false;

			imageConverted = true;
		}

		textureRaw._bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // number of bits per pixel
		uint32 bytesPerRow = (textureRaw._width * textureRaw._bitsPerPixel) / 8; // number of bytes in each row of the image data
		uint32 imageSize = bytesPerRow * textureRaw._height; // total image size in bytes

		textureRaw._data = dk_new byte[imageSize];
		if (textureRaw._data == nullptr)
			return false;
		textureRaw._format = dxgiFormat;

		if (imageConverted)
		{
			hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, textureRaw._data);
			if (FAILED(hr))
			{
				// TextureRaw frees its malloc allocation on every return path.
				return false;
			}
		}
		else
		{
			hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, textureRaw._data);
			if (FAILED(hr))
			{
				// TextureRaw frees its malloc allocation on every return path.
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
	ITextureRef RenderModule::createTexture(const char* debugName, const uint32 width, const uint32 height, const byte* data, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES state, const bool createSRV, const bool createUAV)
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

			return createTexture(DK_DEBUG_NAME(debugName), width, height, &mipData, 1, format, flags, state, createSRV, createUAV);
		}

		return createTexture(DK_DEBUG_NAME(debugName), width, height, (D3D12_SUBRESOURCE_DATA*)nullptr, 1, format, flags, state, createSRV, createUAV);
	}
	ITextureRef RenderModule::createTexture(const char* debugName, const uint32 width, const uint32 height, const D3D12_SUBRESOURCE_DATA* initialData, const uint8 mipLevelCount, const DXGI_FORMAT format, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES state, const bool createSRV, const bool createUAV)
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

		if ((createSRV && (flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) || (createUAV && (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0))
		{
			DK_ASSERT_LOG(false, "Texture flags do not allow requested views");
			return nullptr;
		}

		const D3D12_RESOURCE_STATES initialState = initialData ? D3D12_RESOURCE_STATE_COPY_DEST : state;

#if defined(_DK_DEBUG_)
		ScopeStringW<DK_MAX_PATH> textureName(L"Texture(");
		textureName.append(StringUtil::convertCtoWC(debugName).c_str());
		textureName.append(L")");
#endif
		IBufferRef textureResource = createBuffer2DInternal(width, height, mipLevelCount, format, flags, D3D12_HEAP_TYPE_DEFAULT, initialState, nullptr, DK_DEBUG_NAME(textureName.c_str()));
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
			uploadName.append(StringUtil::convertCtoWC(debugName).c_str());
			uploadName.append(L")");
#endif
			IBufferRef uploadBuffer = createUploadBuffer(uploadSize, DK_DEBUG_NAME(uploadName.c_str()));
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
			resetCommandList();
		}

		ITextureRef texture(dk_new ITexture(debugName, mipLevelCount, format, DK::move(textureResource)));
		if (createSRV && allocateTextureSRV(*texture) == false)
			return nullptr;
		if (createUAV && allocateTextureUAV(*texture) == false)
			return nullptr;

		return texture;
	}
	static ITextureRef loadTexture2DDds(RenderModule& renderModule, const DKString& path, const DKString& fullPath)
	{
		struct DdsPixelFormat
		{
			uint32 _size, _flags, _fourCC, _rgbBitCount, _redMask, _greenMask, _blueMask, _alphaMask;
		};
		struct DdsHeader
		{
			uint32 _size, _flags, _height, _width, _pitch, _depth, _mipCount;
			uint32 _reserved[11];
			DdsPixelFormat _pixelFormat;
			uint32 _caps, _caps2, _caps3, _caps4, _reserved2;
		};
		struct DdsDx10Header
		{
			DXGI_FORMAT _format;
			D3D12_RESOURCE_DIMENSION _dimension;
			uint32 _miscFlags, _arraySize, _alphaMode;
		};
		static_assert(sizeof(DdsHeader) == 124 && sizeof(DdsDx10Header) == 20);
		constexpr uint32 ddsMagic = 0x20534444;
		constexpr uint32 dx10FourCC = 0x30315844;
		constexpr uint32 fourCCFlag = 0x4;
		FileReader reader(fullPath);
		uint32 magic = 0;
		DdsHeader header{};
		DdsDx10Header dx10{};
		if (reader.read(&magic, sizeof(magic)) == false || magic != ddsMagic || reader.read(&header, sizeof(header)) == false ||
			header._size != sizeof(header) || header._pixelFormat._size != sizeof(header._pixelFormat) ||
			(header._pixelFormat._flags & fourCCFlag) == 0 || header._pixelFormat._fourCC != dx10FourCC ||
			reader.read(&dx10, sizeof(dx10)) == false || dx10._dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
			dx10._arraySize != 1 || dx10._miscFlags != 0 || header._caps2 != 0 || header._depth > 1 ||
			header._width == 0 || header._height == 0 || header._width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
			header._height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
			(dx10._format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB && dx10._format != DXGI_FORMAT_R8G8B8A8_UNORM))
		{
			DK_ASSERT_LOG(false, "Expected a DX10 DDS with a single RGBA8 2D texture: %s", path.c_str());
			return nullptr;
		}

		uint32 maxMipCount = 1;
		for (uint32 side = std::max(header._width, header._height); side > 1; side >>= 1)
			++maxMipCount;
		const uint32 mipCount = std::max(1u, header._mipCount);
		if (mipCount > maxMipCount)
		{
			DK_ASSERT_LOG(false, "Invalid DDS mip count: %s", path.c_str());
			return nullptr;
		}

		DKVector<D3D12_SUBRESOURCE_DATA> subresources(mipCount);
		uint64 payloadBytes = 0;
		for (uint32 mip = 0; mip < mipCount; ++mip)
		{
			const uint32 width = std::max(1u, header._width >> mip);
			const uint32 height = std::max(1u, header._height >> mip);
			const LONG_PTR rowPitch = LONG_PTR(width) * 4;
			const LONG_PTR slicePitch = rowPitch * height;
			subresources[mip] = { nullptr, rowPitch, slicePitch };
			payloadBytes += slicePitch;
		}
		if (reader.size() != sizeof(magic) + sizeof(header) + sizeof(dx10) + payloadBytes)
		{
			DK_ASSERT_LOG(false, "Invalid DDS payload size: %s", path.c_str());
			return nullptr;
		}

		DKVector<byte> payload(static_cast<size_t>(payloadBytes));
		if (reader.read(payload.data(), payload.size()) == false)
		{
			DK_ASSERT_LOG(false, "DDS payload read failed: %s", path.c_str());
			return nullptr;
		}
		const byte* mipData = payload.data();
		for (D3D12_SUBRESOURCE_DATA& subresource : subresources)
		{
			subresource.pData = mipData;
			mipData += subresource.SlicePitch;
		}
		constexpr D3D12_RESOURCE_STATES shaderRead = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		return renderModule.createTexture(DK_DEBUG_NAME(path.c_str()), header._width, header._height, subresources.data(), static_cast<uint8>(mipCount), dx10._format, D3D12_RESOURCE_FLAG_NONE, shaderRead, true, false);
	}

	ITextureRef RenderModule::loadAndCreateTexture(const DKString& path)
	{
		DKHashMap<uint64, ITexture*>::iterator findResult = _textureContainer.find(StringUtil::registerHash(path));
		if (findResult != _textureContainer.end())
		{
			return findResult->second->shared_from_this();
		}

		ScopeString<DK_MAX_PATH> textureFullPath = GlobalPath::makeResourceFullPath(path);
		ITextureRef newTexture;
		DKString extension = FileUtil::extension(path);
		StringUtil::lower(extension);
		if (extension == ".dds")
		{
			newTexture = loadTexture2DDds(*this, path, FileUtil::fromWidePath(StringUtil::convertCtoWC(textureFullPath.c_str())));
		}
		else
		{
			TextureRaw textureRaw;
			if (loadImageDataFromFile(textureFullPath.c_str(), textureRaw) == false)
			{
				DK_ASSERT_LOG(false, "TextureData 로딩에 실패했습니다.");
				return nullptr;
			}
			newTexture = createTexture(DK_DEBUG_NAME(path.c_str()), textureRaw._width, textureRaw._height, textureRaw._data, textureRaw._format,
				D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true, false);
		}

		if (newTexture == nullptr)
			return nullptr;

		const DKPair<DKHashMap<uint64, ITexture*>::iterator, bool> insertResult = _textureContainer.try_emplace(StringUtil::registerHash(path), newTexture.get());
		if (insertResult.second == false)
		{
			DK_ASSERT_LOG(false, "HashMap Insert실패. 해시 자체의 오류일 수 있음");
			return nullptr;
		}

		newTexture->_cachePathHash = StringUtil::hash(path);
		return newTexture;
	}

	bool RenderModule::queueTextureRegion(const ITextureRef& texture, uint32 x, uint32 y, uint32 width, uint32 height, const void* data, uint32 rowPitch)
	{
		EnsureMainThread();
		if (texture == nullptr || data == nullptr || width == 0 || height == 0)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		const D3D12_RESOURCE_DESC targetDesc = texture->_textureBuffer._buffer->GetDesc();
		if (x >= targetDesc.Width || y >= targetDesc.Height || width > targetDesc.Width - x || height > targetDesc.Height - y)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		DKCommandList::TextureRegionPendingData pending;
		pending._target = texture;
		pending._x = x;
		pending._y = y;
		const CD3DX12_RESOURCE_DESC region = CD3DX12_RESOURCE_DESC::Tex2D(targetDesc.Format, width, height, 1, 1);
		UINT rows = 0;
		UINT64 rowBytes = 0;
		UINT64 uploadBytes = 0;
		_device->GetCopyableFootprints(&region, 0, 1, 0, &pending._footprint, &rows, &rowBytes, &uploadBytes);
		if (rowBytes > rowPitch || uploadBytes > UINT32_MAX)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		pending._source = createUploadBuffer(static_cast<uint32>(uploadBytes), DK_DEBUG_NAME(L"TextureRegion_Upload"));
		if (pending._source == nullptr)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		void* mapped = nullptr;
		const D3D12_RANGE noReads = { 0, 0 };
		if (FAILED(pending._source->_buffer->Map(0, &noReads, &mapped)))
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		for (UINT row = 0; row < rows; ++row)
		{
			DK::memcpy(static_cast<uint8*>(mapped) + pending._footprint.Offset + size_t(row) * pending._footprint.Footprint.RowPitch, static_cast<const uint8*>(data) + size_t(row) * rowPitch, static_cast<size_t>(rowBytes));
		}
		pending._source->_buffer->Unmap(0, nullptr);
		_commandList._textureRegionPendingArray.push_back(DK::move(pending));
		return true;
	}

	void RenderModule::deleteTexture(ITexture* texture)
	{
		// 이 곳에서 getSRV(), getUAV()를 사용하면 할당되지 않은 경우 내부 어썰트가 발생해서, 여기서만 예외적으로 SRV멤버변수를 직접 접근한다.
		if (texture->_textureSRVIndex != ITexture::kErrorTextureResourceViewIndex)
			deallocateTextureSRV(texture->getSRV());
		if (texture->_textureUAVIndex != ITexture::kErrorTextureResourceViewIndex)
			deallocateTextureUAV(texture->getUAV());

		if (texture->_cachePathHash != 0)
		{
			const DKHashMap<uint64, ITexture*>::iterator entry = _textureContainer.find(texture->_cachePathHash);
			DK_ASSERT_LOG(entry != _textureContainer.end() && entry->second == texture, "Texture cache ownership mismatch: %s", texture->_path.c_str());
			texture->_cachePathHash = 0;
			_textureContainer.erase(entry);
		}
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

#if defined(_DK_DEBUG_)
	RenderModule::GpuTimingScope::GpuTimingScope(RenderModule& renderModule, const DKString& name)
		: _renderModule(renderModule), _sampleIndex(renderModule._recordedGpuTimingSamples.size())
	{
		const uint32 parentIndex = renderModule._gpuTimingScopeStack.empty() ? UINT32_MAX : renderModule._recordedGpuTimingSamples[renderModule._gpuTimingScopeStack.back()];
		const uint32 timingIndex = renderModule.registerGpuTiming(name, parentIndex);
		renderModule.beginGpuTiming(timingIndex);
	}

	RenderModule::GpuTimingScope::~GpuTimingScope()
	{
		DK_ASSERT_LOG(_renderModule._gpuTimingScopeStack.empty() == false && _renderModule._gpuTimingScopeStack.back() == _sampleIndex, "GPU timing scopes must end in reverse order");
		_renderModule.finishGpuTiming();
	}

	uint32 RenderModule::registerGpuTiming(const DKString& name, uint32 parentIndex)
	{
		const uint64 nameHash = StringUtil::registerHash(name);
		for (uint32 timingIndex = 0; timingIndex < _gpuTimings.size(); ++timingIndex)
		{
			if (_gpuTimings[timingIndex]._parentIndex == parentIndex && _gpuTimings[timingIndex]._nameHash == nameHash)
				return timingIndex;
		}

		const uint32 timingIndex = static_cast<uint32>(_gpuTimings.size());
		GpuTiming timing;
		timing._nameHash = nameHash;
		timing._name = name;
		timing._parentIndex = parentIndex;
		if (parentIndex != UINT32_MAX)
		{
			DK_ASSERT_LOG(parentIndex < _gpuTimings.size(), "GPU timing parent was not registered");
			timing._depth = _gpuTimings[parentIndex]._depth + 1;
			_gpuTimings[parentIndex]._childIndices.push_back(timingIndex);
		}
		_gpuTimings.push_back(DK::move(timing));
		return timingIndex;
	}

	void RenderModule::resetGpuTimingPeaks()
	{
		for (GpuTiming& timing : _gpuTimings)
			timing._peakMilliseconds = 0.0;
		_gpuTotalPeakMilliseconds = 0.0;
	}

	void RenderModule::beginGpuTiming(uint32 timingIndex)
	{
		DK_ASSERT_LOG(timingIndex < _gpuTimings.size(), "GPU timing was not registered");
		const uint32 parentIndex = _gpuTimingScopeStack.empty() ? UINT32_MAX : _recordedGpuTimingSamples[_gpuTimingScopeStack.back()];
		DK_ASSERT_LOG(_gpuTimings[timingIndex]._parentIndex == parentIndex, "GPU timing scope parent does not match");
		const size_t sampleIndex = _recordedGpuTimingSamples.size();
		const size_t blockIndex = sampleIndex / GPU_TIMINGS_PER_BLOCK;
		if (blockIndex == _gpuTimestampBlocks.size())
		{
			GpuTimestampBlock& block = _gpuTimestampBlocks.emplace_back();
			D3D12_QUERY_HEAP_DESC queryDesc = {};
			queryDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			queryDesc.Count = GPU_TIMINGS_PER_BLOCK * 2;
			ID3D12QueryHeap* queryHeap = nullptr;
			const HRESULT queryResult = _device->CreateQueryHeap(&queryDesc, IID_PPV_ARGS(&queryHeap));
			DK_ASSERT_LOG(SUCCEEDED(queryResult), "GPU pass timestamp heap creation failed: 0x%08X", static_cast<uint32>(queryResult));
			block._queryHeap = queryHeap;

			const CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_READBACK);
			const CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(queryDesc.Count * sizeof(uint64));
			ID3D12Resource* readback = nullptr;
			const HRESULT readbackResult = _device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback));
			DK_ASSERT_LOG(SUCCEEDED(readbackResult), "GPU pass timestamp readback creation failed: 0x%08X", static_cast<uint32>(readbackResult));
			block._readback = readback;
		}

		const uint32 queryIndex = static_cast<uint32>(sampleIndex % GPU_TIMINGS_PER_BLOCK) * 2;
		_commandList->EndQuery(_gpuTimestampBlocks[blockIndex]._queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
		_recordedGpuTimingSamples.push_back(timingIndex);
		_gpuTimingScopeStack.push_back(sampleIndex);
	}

	void RenderModule::finishGpuTiming()
	{
		DK_ASSERT_LOG(_gpuTimingScopeStack.empty() == false, "GPU timing has no active scope");
		const size_t sampleIndex = _gpuTimingScopeStack.back();
		_gpuTimingScopeStack.pop_back();
		const size_t blockIndex = sampleIndex / GPU_TIMINGS_PER_BLOCK;
		const uint32 queryIndex = static_cast<uint32>(sampleIndex % GPU_TIMINGS_PER_BLOCK) * 2 + 1;
		_commandList->EndQuery(_gpuTimestampBlocks[blockIndex]._queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
	}

	void RenderModule::resolveGpuTimings()
	{
		DK_ASSERT_LOG(_gpuTimingScopeStack.empty(), "GPU timings must be resolved after all scopes have ended");
		if (_recordedGpuTimingSamples.empty())
			return;

		// ResolveQueryData may create an internal COPY_DEST buffer and emit warning #1328.
		// Suppress only that benign runtime warning for this call; keep other diagnostics enabled.
		// Windows SDK 10.0.19041 does not name CREATERESOURCE_STATE_IGNORED yet.
		D3D12_MESSAGE_ID ignoredMessage = static_cast<D3D12_MESSAGE_ID>(1328);
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = 1;
		filter.DenyList.pIDList = &ignoredMessage;
		ID3D12InfoQueue* infoQueue = nullptr;
		bool filterPushed = false;
		if (SUCCEEDED(_device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
			filterPushed = SUCCEEDED(infoQueue->PushStorageFilter(&filter));

		for (size_t firstSample = 0; firstSample < _recordedGpuTimingSamples.size(); firstSample += GPU_TIMINGS_PER_BLOCK)
		{
			GpuTimestampBlock& block = _gpuTimestampBlocks[firstSample / GPU_TIMINGS_PER_BLOCK];
			const uint32 sampleCount = static_cast<uint32>(std::min(_recordedGpuTimingSamples.size() - firstSample, size_t(GPU_TIMINGS_PER_BLOCK)));
			_commandList->ResolveQueryData(block._queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, sampleCount * 2, block._readback.get(), 0);
		}

		if (filterPushed)
			infoQueue->PopStorageFilter();
		if (infoQueue != nullptr)
			infoQueue->Release();
	}

	void RenderModule::readGpuTimings()
	{
		if (_gpuTimingsPending == false)
			return;

		for (GpuTiming& timing : _gpuTimings)
		{
			timing._milliseconds = 0.0;
			timing._executionCount = 0;
		}

		const double millisecondsPerTick = 1000.0 / double(_gpuTimestampFrequency);
		for (size_t firstSample = 0; firstSample < _submittedGpuTimingSamples.size(); firstSample += GPU_TIMINGS_PER_BLOCK)
		{
			GpuTimestampBlock& block = _gpuTimestampBlocks[firstSample / GPU_TIMINGS_PER_BLOCK];
			const size_t sampleCount = std::min(_submittedGpuTimingSamples.size() - firstSample, size_t(GPU_TIMINGS_PER_BLOCK));
			const D3D12_RANGE readRange = { 0, sampleCount * 2 * sizeof(uint64) };
			void* address = nullptr;
			const HRESULT mapResult = block._readback.get()->Map(0, &readRange, &address);
			DK_ASSERT_LOG(SUCCEEDED(mapResult), "GPU pass timestamp readback failed: 0x%08X", static_cast<uint32>(mapResult));
			const uint64* ticks = static_cast<const uint64*>(address);
			for (size_t sample = 0; sample < sampleCount; ++sample)
			{
				const uint64 begin = ticks[sample * 2];
				const uint64 end = ticks[sample * 2 + 1];
				DK_ASSERT_LOG(end >= begin, "GPU pass timestamp order is invalid");
				GpuTiming& timing = _gpuTimings[_submittedGpuTimingSamples[firstSample + sample]];
				timing._milliseconds += double(end - begin) * millisecondsPerTick;
				++timing._executionCount;
			}
			const D3D12_RANGE writeRange = { 0, 0 };
			block._readback.get()->Unmap(0, &writeRange);
		}

		_gpuTotalMilliseconds = 0.0;
		for (GpuTiming& timing : _gpuTimings)
		{
			timing._peakMilliseconds = Math::max(timing._peakMilliseconds, timing._milliseconds);
			if (timing._depth == 0)
				_gpuTotalMilliseconds += timing._milliseconds;
		}
		_gpuTotalPeakMilliseconds = Math::max(_gpuTotalPeakMilliseconds, _gpuTotalMilliseconds);

		_submittedGpuTimingSamples.clear();
		_gpuTimingsPending = false;
		_hasGpuTimings = true;
	}
#endif

	void RenderModule::preRender()
	{
#if defined(_DK_DEBUG_)
		_blockUpload = true;
		_blockCopy = true;
#endif

		if (gSerializeRender == false && _frameSubmitted)
		{
			if (_commandList._lastResetIndex == kCurrentBackBufferIndex)
				waitFenceAndResetCommandList();
			resetCommandList();
		}

		for (const DKCommandList::TextureRegionPendingData& pending : _commandList._textureRegionPendingArray)
		{
			IBuffer& target = pending._target->getTextureBuffer();
			resourceBarrierTransition(target, D3D12_RESOURCE_STATE_COPY_DEST);
			D3D12_TEXTURE_COPY_LOCATION source = {};
			source.pResource = pending._source->_buffer.get();
			source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			source.PlacedFootprint = pending._footprint;
			D3D12_TEXTURE_COPY_LOCATION destination = {};
			destination.pResource = target._buffer.get();
			destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			_commandList->CopyTextureRegion(&destination, pending._x, pending._y, 0, &source, nullptr);
			resourceBarrierTransition(target, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		for (const DKCommandList::CopyResourcePendingData& pending : _commandList._copyResourcePendingArray)
		{
			resourceBarrierTransition(*pending._source.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrierTransition(*pending._target.get(), D3D12_RESOURCE_STATE_COPY_DEST);
			_commandList->CopyResource(pending._target->_buffer.get(), pending._source->_buffer.get());
			if(pending._afterState != static_cast<D3D12_RESOURCE_STATES>(DKCommandList::CopyResourcePendingData::INVALID_AFTER_STATE))
				resourceBarrierTransition(*pending._target.get(), pending._afterState);
		}
	}

	void RenderModule::beginRenderPass(RenderPass& pass)
	{
#if defined(_DK_DEBUG_)
		beginGpuTiming(pass._gpuTimingIndex);
#endif
		beginRenderPass(pass, pass._binding);
	}

	void RenderModule::beginRenderPass(const RenderPassInstance& instance)
	{
		DK_ASSERT_LOG(instance._definition != nullptr, "Render pass instance has no definition");
#if defined(_DK_DEBUG_)
		beginGpuTiming(instance._gpuTimingIndex);
#endif
		beginRenderPass(*instance._definition, instance._binding);
	}

	void RenderModule::beginRenderPass(RenderPass& pass, const RenderPassBinding& binding)
	{
		DK_ASSERT_LOG(gCurrentBindedRenderPass == nullptr && gCurrentBindedPipeline == nullptr, "A render pass or pipeline is already active");
		gCurrentBindedRenderPass = &pass;

		const RenderPassTargets* targets = nullptr;
		if (binding._target != nullptr)
		{
			const RenderTargetDefinition& definition = binding._target->_definition;
			targets = definition._size == RenderTargetDefinition::Size::SwapChain
				? &_backBufferPassTargets[kCurrentBackBufferIndex]
				: &binding._target->_targets;
			bindRenderPassTargets(*targets, binding._load, definition._depth._format);
		}

		DK_ASSERT_LOG(binding._store != RenderPassStoreAction::ShaderRead || targets != nullptr, "Render pass has no output to transition");
		_passTargetsToTransition = binding._store == RenderPassStoreAction::ShaderRead ? targets : nullptr;
	}

	void RenderModule::bindRenderPassTargets(const RenderPassTargets& targets, RenderPassLoadAction load, DXGI_FORMAT depthFormat)
	{
		DK_ASSERT_LOG(targets._width > 0 && targets._height > 0, "Render pass targets have an invalid extent");
		DK_ASSERT_LOG(targets._colorCount <= DK_COUNT_OF(targets._colors), "Too many render pass color attachments");

		const D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(targets._width), static_cast<float>(targets._height), 0, 1 };
		const D3D12_RECT scissor = { 0, 0, static_cast<LONG>(targets._width), static_cast<LONG>(targets._height) };
		_commandList->RSSetViewports(1, &viewport);
		_commandList->RSSetScissorRects(1, &scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		const float clearColor[4] = {};
		for (uint32 i = 0; i < targets._colorCount; ++i)
		{
			const RenderPassAttachment& color = targets._colors[i];
			DK_ASSERT_LOG(color._texture != nullptr && color._view.ptr != 0, "Invalid color attachment");
			resourceBarrierTransition(color._texture->getTextureBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			rtvs[i] = color._view;
			if (load == RenderPassLoadAction::Clear)
				_commandList->ClearRenderTargetView(color._view, clearColor, 0, nullptr);
		}

		const RenderPassAttachment& depth = targets._depth;
		if (depth._texture != nullptr)
		{
			DK_ASSERT_LOG(depth._view.ptr != 0, "Invalid depth attachment");
			resourceBarrierTransition(depth._texture->getTextureBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
			if (load == RenderPassLoadAction::Clear)
			{
				const D3D12_CLEAR_FLAGS flags = depthFormat == DXGI_FORMAT_D24_UNORM_S8_UINT || depthFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
					? D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL : D3D12_CLEAR_FLAG_DEPTH;
				_commandList->ClearDepthStencilView(depth._view, flags, 0.0f, 0, 0, nullptr);
			}
		}
		_commandList->OMSetRenderTargets(targets._colorCount, rtvs, FALSE, depth._texture != nullptr ? &depth._view : nullptr);
	}

	void RenderModule::finishRenderPass()
	{
		DK_ASSERT_LOG(gCurrentBindedRenderPass != nullptr && gCurrentBindedPipeline == nullptr, "Render pass must end after its pipelines");
		if (_passTargetsToTransition != nullptr)
		{
			const RenderPassTargets& targets = *_passTargetsToTransition;
			const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			for (uint32 i = 0; i < targets._colorCount; ++i)
				resourceBarrierTransition(targets._colors[i]._texture->getTextureBuffer(), state);
			if (targets._depth._texture != nullptr)
				resourceBarrierTransition(targets._depth._texture->getTextureBuffer(), state);
			_passTargetsToTransition = nullptr;
		}
#if defined(_DK_DEBUG_)
		finishGpuTiming();
#endif
		gCurrentBindedRenderPass = nullptr;
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
			_commandList->SetPipelineState1(pipeline._raytracing._stateObject.get());
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

#if defined(_DK_DEBUG_)
		DKHashMap<uint64, ShaderParameter>::const_iterator debugHangParameter = pipeline._shaderParameterMap.find(StringUtil::hash("GpuAssertBuffer"));
		if (debugHangParameter != pipeline._shaderParameterMap.end())
		{
			if (_gpuAssertBuffer == nullptr || _gpuAssertBuffer->isValid() == false)
				return false;

			const uint32 rootIndex = debugHangParameter->second._rootParameterIndex;
			const D3D12_GPU_VIRTUAL_ADDRESS address = _gpuAssertBuffer->_buffer->GetGPUVirtualAddress();
			if (type == Pipeline::Type::GRAPHIC)
				_commandList->SetGraphicsRootUnorderedAccessView(rootIndex, address);
			else
				_commandList->SetComputeRootUnorderedAccessView(rootIndex, address);
		}
#endif

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
		uint32 (*const divideRoundUp)(uint32, uint32) = [](uint32 count, uint32 groupSize) -> uint32
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
#if defined(_DK_DEBUG_)
		resolveGpuTimings();
#endif

#if defined(USE_IMGUI)
		ID3D12DescriptorHeap* heaps[] = { _imguiDescriptorHeap.get(), };
		_commandList->SetDescriptorHeaps(DK_COUNT_OF(heaps), heaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList._commandList.get());
#endif // USE_IMGUI

		resourceBarrierTransition(_backBufferPassTargets[kCurrentBackBufferIndex]._colors[0]._texture->getTextureBuffer(), D3D12_RESOURCE_STATE_PRESENT);


		if (gSerializeRender == false)
			waitFenceAndResetCommandList();

		_commandList._prevUploadResourcePendingArray.clear();
		_commandList._prevCopyResourcePendingArray.clear();
		_commandList._prevTextureRegionPendingArray.clear();

		for (const DKCommandList::UploadResourcePendingData& pending : _commandList._uploadResourcePendingArray)
		{
			if (pending._target.get() == nullptr)
				continue;

			DK_ASSERT_LOG(pending.getData() != nullptr, "nullptr인 data를 upload요청하려고합니다.");
			DK_ASSERT_LOG(pending._target->_type == IBuffer::Type::UPLOAD && pending._target->isValid(), "Upload버퍼에 대해서만 호출 가능합니다.");

			void* address = nullptr;
			HRESULT hr = pending._target.get()->_buffer->Map(0, nullptr, &address);
			DK_ASSERT_LOG(SUCCEEDED(hr), "Map에 실패하였습니다.");
			DK::memcpy(address, pending.getData(), pending._target->_bufferSize);
			pending._target.get()->_buffer->Unmap(0, nullptr);
		}
		_commandList._prevUploadResourcePendingArray.swap(_commandList._uploadResourcePendingArray);
		_commandList._prevCopyResourcePendingArray.swap(_commandList._copyResourcePendingArray);
		_commandList._prevTextureRegionPendingArray.swap(_commandList._textureRegionPendingArray);

		execute(true);

		if (gSerializeRender)
		{
			waitFenceAndResetCommandList();
			resetCommandList();
		}

#if defined(_DK_DEBUG_)
		_blockUpload = false;
		_blockCopy = false;
#endif
	}

    bool RenderModule::supportsRaytracing() const
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = {};
        return SUCCEEDED(const_cast<ID3D12Device8*>(_device.get())->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options))) &&
            options.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    }

    IBufferRef RenderModule::createAccelerationBuffer(const uint64 size, const D3D12_RESOURCE_STATES state)
    {
        if (size == 0 || size > UINT32_MAX)
            return nullptr;
        CD3DX12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        ID3D12Resource* resource = nullptr;
        if (FAILED(_device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&resource))))
            return nullptr;
        return IBufferRef(dk_new IBuffer(IBuffer::Type::DEFAULT, RenderResourcePtr<ID3D12Resource>(resource), static_cast<uint32>(size), state));
    }

    bool RenderModule::buildRaytracingScene(const DKVector<RaytracingGeometry>& geometries, RaytracingAccelerationStructure& result)
    {
        const uint32 count = static_cast<uint32>(geometries.size());
        if (count == 0 || count > RaytracingRenderer::kRaytracingDescriptorCount)
            return false;
        DKVector<D3D12_RAYTRACING_GEOMETRY_DESC> descriptions(count);
        DKVector<D3D12_RAYTRACING_INSTANCE_DESC> instances(count);
        result._bottomLevels.reserve(count);
        result._bottomLevelScratch.reserve(count);
        for (uint32 i = 0; i < count; ++i)
        {
            const RaytracingGeometry& geometry = geometries[i];
            if (geometry._vertices == nullptr || geometry._indices == nullptr || geometry._material == nullptr ||
                geometry._vertices->isValid() == false || geometry._indices->isValid() == false || geometry._material->isValid() == false)
                return false;
            D3D12_RAYTRACING_GEOMETRY_DESC& desc = descriptions[i];
            desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            desc.Triangles.VertexBuffer = { geometry._vertices->_buffer->GetGPUVirtualAddress(), geometry._vertexStride };
            desc.Triangles.VertexCount = geometry._vertexCount;
            desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            desc.Triangles.IndexBuffer = geometry._indices->_buffer->GetGPUVirtualAddress();
            desc.Triangles.IndexCount = geometry._indexCount;
            desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
            inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            inputs.NumDescs = 1;
            inputs.pGeometryDescs = &desc;
            inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
            _device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
            IBufferRef blas = createAccelerationBuffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
            IBufferRef scratch = createAccelerationBuffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_COMMON);
            if (blas == nullptr || scratch == nullptr)
                return false;
            instances[i].InstanceID = i;
            instances[i].InstanceMask = 0xff;
            instances[i].AccelerationStructure = blas->_buffer->GetGPUVirtualAddress();
			instances[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
            const float* world = reinterpret_cast<const float*>(&geometry._world);
            for (uint32 row = 0; row < 3; ++row)
                for (uint32 column = 0; column < 4; ++column)
                    instances[i].Transform[row][column] = world[column * 4 + row];
            result._bottomLevels.push_back(DK::move(blas));
            result._bottomLevelScratch.push_back(DK::move(scratch));
        }
        result._instances = createUploadBuffer(count * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), DK_DEBUG_NAME(L"TLAS Instances"));
        if (result._instances == nullptr || result._instances->isValid() == false)
            return false;
        void* mapped = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        if (FAILED(result._instances->_buffer->Map(0, &readRange, &mapped)))
            return false;
        DK::memcpy(mapped, instances.data(), count * sizeof(instances[0]));
        result._instances->_buffer->Unmap(0, nullptr);
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
        tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        tlasInputs.NumDescs = count;
        tlasInputs.InstanceDescs = result._instances->_buffer->GetGPUVirtualAddress();
        tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
        _device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &info);
        result._result = createAccelerationBuffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
        result._scratch = createAccelerationBuffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_COMMON);
        if (result._result == nullptr || result._scratch == nullptr)
            return false;

        // Allocate everything before recording commands. Descriptors are replaced only after a GPU wait by the caller.
        for (uint32 i = 0; i < count; ++i)
        {
            const RaytracingGeometry& geometry = geometries[i];
            resourceBarrierTransition(*geometry._vertices, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            resourceBarrierTransition(*geometry._indices, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_INDEX_BUFFER);
            const IBufferRef buffers[] = { geometry._vertices, geometry._indices, geometry._material };
            const uint32 counts[] = { geometry._vertexCount, geometry._indexCount, 1 };
            const uint32 strides[] = { geometry._vertexStride, sizeof(uint32), geometry._materialStride };
            for (uint32 table = 0; table < 3; ++table)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
                view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view.Buffer.NumElements = counts[table];
                view.Buffer.StructureByteStride = strides[table];
                D3D12_CPU_DESCRIPTOR_HANDLE handle = _textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
                handle.ptr += static_cast<SIZE_T>(kMaxTextureSRVCount + kMaxTextureUAVCount + table * RaytracingRenderer::kRaytracingDescriptorCount + i) * _bindlessViewSize;
                _device->CreateShaderResourceView(buffers[table]->_buffer.get(), &view, handle);
            }
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build = {};
            build.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            build.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            build.Inputs.NumDescs = 1;
            build.Inputs.pGeometryDescs = &descriptions[i];
            build.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            build.DestAccelerationStructureData = result._bottomLevels[i]->_buffer->GetGPUVirtualAddress();
            build.ScratchAccelerationStructureData = result._bottomLevelScratch[i]->_buffer->GetGPUVirtualAddress();
            resourceBarrierTransition(*result._bottomLevelScratch[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            _commandList->BuildRaytracingAccelerationStructure(&build, 0, nullptr);
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(result._bottomLevels[i]->_buffer.get());
            _commandList->ResourceBarrier(1, &barrier);
        }
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build = {};
        build.Inputs = tlasInputs;
        build.DestAccelerationStructureData = result._result->_buffer->GetGPUVirtualAddress();
        build.ScratchAccelerationStructureData = result._scratch->_buffer->GetGPUVirtualAddress();
        resourceBarrierTransition(*result._scratch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        _commandList->BuildRaytracingAccelerationStructure(&build, 0, nullptr);
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(result._result->_buffer.get());
        _commandList->ResourceBarrier(1, &barrier);
        DK_LOG("DXR scene built: %u static mesh instances", count);
        return true;
    }

    void RenderModule::copyTextureToRenderTarget(ITextureRef& source, ITexture& destination)
    {
        IBuffer& target = destination.getTextureBuffer();
        resourceBarrierTransition(source, D3D12_RESOURCE_STATE_COPY_SOURCE);
        resourceBarrierTransition(target, D3D12_RESOURCE_STATE_COPY_DEST);
        _commandList->CopyResource(target._buffer.get(), source->getTextureBuffer()._buffer.get());
        resourceBarrierTransition(target, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

	void RenderModule::dispatchRays(const uint32 width, const uint32 height, const uint32 depth)
	{
		if (width == 0 || height == 0 || depth == 0)
			return;
		if (gCurrentBindedPipeline == nullptr || gCurrentBindedPipeline->_type != Pipeline::Type::RAYTRACING)
		{
			DK_ASSERT_LOG(false, "dispatchRays requires a raytracing pipeline.");
			return;
		}
		ShaderBindingTable& sbt = gCurrentBindedPipeline->_raytracing._sbt;
		if (sbt._buffer == nullptr || sbt._buffer->isValid() == false)
		{
			DK_ASSERT_LOG(false, "Raytracing pipeline has no shader binding table.");
			return;
		}
		const D3D12_GPU_VIRTUAL_ADDRESS address = sbt._buffer->_buffer->GetGPUVirtualAddress();
		D3D12_DISPATCH_RAYS_DESC desc = {};
		desc.RayGenerationShaderRecord = { address + sbt._rayGeneration._offset, sbt._rayGeneration._size };
		desc.MissShaderTable = { address + sbt._miss._offset, sbt._miss._size, sbt._miss._stride };
		desc.HitGroupTable = { address + sbt._hitGroup._offset, sbt._hitGroup._size, sbt._hitGroup._stride };
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		_commandList->DispatchRays(&desc);
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
		// isValid 체크라고 봐도됨
		if (getTextureBuffer().isValid() == false)
			return;

#if defined(_DK_DEBUG_)
		if (DuckingEngine::getInstance().GetRenderModule()._isDestroyed)
		{
			DK_ASSERT_LOG(false, "Must called before rendermodule destroy!!!\nPath: %s", _path.c_str());
			return;
		}
#endif
		DuckingEngine::getInstance().GetRenderModuleWritable().deleteTexture(this);
	}

}
