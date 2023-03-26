#include "stdafx.h"
#include "TextureManager.h"

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
//TextureFormat ConvertTextureFormatFromDXGIFormat(DXGI_FORMAT& dxgiFormat)
//{
//	// #todo- DXGI 열거형 그대로 TextureFormat을 만들었기 때문에 지금은 그냥 static_cast합니다.
//	// 나중에 직접 TextureFormat을 지정하면 그때는 삭제해야합니다.
//	return static_cast<TextureFormat>(dxgiFormat);
//}
#endif

#include "DuckingEngine.h"
#include "RenderModule.h"

namespace DK
{
	wchar_t* CharToWChar(const char* pstrSrc)
	{
		int nLen = static_cast<int>(strlen(pstrSrc)) + 1;

		wchar_t* pwstr = (LPWSTR)malloc(sizeof(wchar_t) * nLen);

		size_t cn;
		mbstowcs_s(&cn, pwstr, nLen, pstrSrc, nLen);

		return pwstr;
	}
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

			hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&wicFactory)
			);
			if (FAILED(hr)) return false;
		}

		const wchar_t* wTexturePath = CharToWChar(fileName);
		hr = wicFactory->CreateDecoderFromFilename(
			wTexturePath,                    // Image we want to load in
			NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
			GENERIC_READ,                    // We want to read from this file
			WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
			&wicDecoder                      // the wic decoder to be created
		);
		dk_delete_array(wTexturePath);
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

	bool TextureManager::initialize()
	{
		if (DuckingEngine::getInstance().GetRenderModuleWritable().createTextureBindlessDescriptorHeap(_textureDescriptorHeap) == false)
			return false;

		return true;
	}

	const ITextureRef& TextureManager::createTexture(const DKString& texturePath)
	{
		DKHashMap<DKString, ITextureRef>::iterator findResult = _textureContainer.find(texturePath);
		if (findResult != _textureContainer.end())
		{
			return findResult->second;
		}

		ScopeString<DK_MAX_PATH> textureFullPath = GlobalPath::makeResourceFullPath(texturePath);

		TextureRaw textureRaw;
		const bool loadingTextureSuccess = loadImageDataFromFile(textureFullPath.c_str(), textureRaw);
		if (loadingTextureSuccess == false)
		{
			DK_ASSERT_LOG(false, "TextureData 로딩에 실패했습니다.");
			// #todo- Null RefPtr을 static하게 만들고 그거 반환해야함
		}

		ITexture::TextureSRVType index = static_cast<ITexture::TextureSRVType>(_textureContainer.size());
		if (_deletedTextureSRV.empty() == false)
		{
			index = _deletedTextureSRV.back();
			_deletedTextureSRV.pop_back();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE handle = _textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		if (DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(textureRaw, handle, index) == false)
		{
			DK_ASSERT_LOG(false, "TextureData 로딩에 실패했습니다.");
			// #todo- Null RefPtr을 static하게 만들고 그거 반환해야함
		}

		using InsertResult = DKPair<DKHashMap<DKString, ITextureRef>::iterator, bool>;
		InsertResult insertResult = _textureContainer.insert(DKPair<DKString, ITextureRef>(texturePath, dk_new ITexture(texturePath, index)));
		if (insertResult.second == false)
		{
			DK_ASSERT_LOG(false, "HashMap Insert실패. 해시 자체의 오류일 수 있음");
			// #todo- Null RefPtr을 static하게 만들고 그거 반환해야함
		}

		return insertResult.first->second;
	}
}