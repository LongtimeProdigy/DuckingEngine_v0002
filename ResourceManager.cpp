#include "stdafx.h"
#include "ResourceManager.h"

#pragma region Lib
#include "FBXLoader.h"
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
uint GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
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
		return -1;
	}
}
//TextureFormat ConvertTextureFormatFromDXGIFormat(DXGI_FORMAT& dxgiFormat)
//{
//	// #todo- DXGI 열거형 그대로 TextureFormat을 만들었기 때문에 지금은 그냥 static_cast합니다.
//	// 나중에 직접 TextureFormat을 지정하면 그때는 삭제해야합니다.
//	return static_cast<TextureFormat>(dxgiFormat);
//}
#endif
#pragma endregion

#include "Model.h"
#include "Skeleton.h"
#include "Animation.h"

const bool ResourceManager::LoadMesh(const char* modelPath, ModelRef& outModel)
{
	typedef std::pair<std::unordered_map<const char*, ModelRef>::iterator, bool> InsertResult;
	InsertResult success = _modelContainer.insert(std::pair<const char*, ModelRef>(modelPath, dk_new Model));
	if (success.second == false)
	{
		outModel = success.first->second;
		return true;
	}

	FBXLoader* loader = new FBXLoader;
	ModelRef& model = success.first->second;
	if (loader->LoadFBXMeshFromFile(modelPath, *model.get()) == false)
	{
		DK_ASSERT_LOG(true, "Model Loading Failed. path: %s", modelPath);
		_modelContainer.erase(modelPath);
		return false;
	}

	outModel = success.first->second;
	return true;
}

const bool ResourceManager::LoadSkeleton(const char* skeletonPath, const ModelRef& model, SkeletonRef& outSkeleton)
{
	typedef std::pair<std::unordered_map<const char*, SkeletonRef>::iterator, bool> InsertResult;
	InsertResult success = _skeletonContainer.insert(std::pair<const char*, SkeletonRef>(skeletonPath, dk_new Skeleton));
	if (success.second == false)
	{
		outSkeleton = success.first->second;
		return true;
	}

	FBXLoader* loader = new FBXLoader;
	SkeletonRef& skeleton = success.first->second;
	if (loader->LoadFBXSkeletonFromFile(skeletonPath, model, *skeleton.get()) == false)
	{
		DK_ASSERT_LOG(true, "Skeleton Loading Failed. path: %s", skeletonPath);
		_modelContainer.erase(skeletonPath);
		return false;
	}

	outSkeleton = success.first->second;
	return true;
}
const bool ResourceManager::LoadAnimation(const char* animationPath, AnimationRef& outAnimation)
{
	typedef std::pair<std::unordered_map<const char*, AnimationRef>::iterator, bool> InsertResult;
	InsertResult success = _animationContainer.insert(std::pair<const char*, AnimationRef>(animationPath, dk_new Animation));
	if (success.second == false)
	{
		outAnimation = success.first->second;
		return true;
	}

	FBXLoader* loader = new FBXLoader;
	AnimationRef& animation = success.first->second;
	if (loader->LoadFBXAnimationFromFile(animationPath, *animation.get()) == false)
	{
		DK_ASSERT_LOG(true, "Animation Loading Failed. path: %s", animationPath);
		_modelContainer.erase(animationPath);
		return false;
	}

	outAnimation = success.first->second;
	return true;
}

wchar_t* CharToWChar(const char* pstrSrc)
{
	int nLen = static_cast<int>(strlen(pstrSrc)) + 1;

	wchar_t* pwstr = (LPWSTR)malloc(sizeof(wchar_t) * nLen);

	size_t cn;
	mbstowcs_s(&cn, pwstr, nLen, pstrSrc, nLen);

	return pwstr;
}
bool LoadImageDataFromFile(const char* fileName, _OUT_ TextureRaw& textureRaw)
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
	uint bytesPerRow = (textureRaw._width * textureRaw._bitsPerPixel) / 8; // number of bytes in each row of the image data
	uint imageSize = bytesPerRow * textureRaw._height; // total image size in bytes

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
const TextureRawRef ResourceManager::LoadTextureRaw(const char* texturePath)
{
	auto success = _textureRawContainer.find(texturePath);
	if (success != _textureRawContainer.end())
	{
		return success->second;
	}

	// 텍스쳐 로딩
	TextureRaw* textureRaw = dk_new TextureRaw;
	const bool loadingTextureSuccess = LoadImageDataFromFile(texturePath, *textureRaw);
	CHECK_BOOL_AND_CUSTOMRETURN(loadingTextureSuccess, nullptr);

	using InsertResult = std::pair<std::unordered_map<const char*, TextureRawRef>::iterator, bool>;
	InsertResult insertResult = _textureRawContainer.insert(std::pair<const char*, TextureRawRef>(texturePath, std::move(textureRaw)));

	if (insertResult.second == false)
	{
		DK_ASSERT_LOG(true, "Texture Loading Failed. path: %s", texturePath);
		_textureRawContainer.erase(texturePath);
		return nullptr;
	}

	return insertResult.first->second;
}

//bool ResourceManager::CreateSkinnedMeshComponent(const char* modelPath, SkinnedMeshComponent* outModel)
//{
//	// #todo- ModelProperty 구현할 것
//	struct ModelProperty
//	{
//		const char* vertexShaderPath;
//		const char* pixelShaderPath;
//
//		// #todo- 메터리얼 파라미터는.. 가변적으로 해야할듯?
//		// 일단 현재는 diffuse하나만
//		const char* diffuseTexture;
//	};
//	std::vector<ModelProperty> modelPropertyRaws;	// Per Submesh Count
//
//	Model* modelData = nullptr;
//	// #todo- 원래는 캐릭터 설정 파일(어피어런스)에서 모델파일 경로를 얻어와야합니다.
//	std::unordered_map<const char*, const Model*>::iterator foundModelData = _modelContainer.find(modelPath);
//	if (foundModelData != _modelContainer.end())
//	{
//		modelData = const_cast<Model*>(foundModelData->second);
//	}
//	else
//	{
//		FBXLoader* loader = new FBXLoader;
//		if (loader->LoadFBXMeshFromFile(modelPath, modelData) == false)
//		{
//			DK_ASSERT_LOG(true, "모델 로딩에 실패했습니다. path: %s", modelPath);
//			return false;
//		}
//
//		std::pair<const char*, const Model*> modelPair(modelPath, modelData);
//		_modelContainer.insert(modelPair);
//	}
//
//	RenderModule* rm = _application->GetRenderModuleWriable();
//
//	uint subMeshCount = static_cast<uint>(outModel->_subMeshes.size());
//	for (uint i = 0; i < subMeshCount; ++i)
//	{
//		ModelProperty currentModelPropertyRaw;
//#if 1
//		if (i >= modelPropertyRaws.size())
//		{
//			// Create Defaut ModelProperty SubMesh Material
//			currentModelPropertyRaw.vertexShaderPath = "Shader/VertexShader.hlsl";
//			currentModelPropertyRaw.pixelShaderPath = "Shader/PixelShader.hlsl";
//			currentModelPropertyRaw.diffuseTexture = "Resource/FBX/7_npc_ip_kezhanlaoban_001.png";
//		}
//		else
//		{
//			currentModelPropertyRaw = modelPropertyRaws[i];
//		}
//#else
//#endif
//
//		// #todo- 셰이더를.. 모델이 들고 있는 게 맞나..?
//		// 메터리얼을.. 파이프라인에 연결하는 방식이 맞는듯..
//		const uint vertexShaderIndex = rm->LoadVertexShader(currentModelPropertyRaw.vertexShaderPath);
//		CHECK_BOOL_AND_RETURN(vertexShaderIndex != RenderModule::kLoadShaderErrorCode);
//		const uint pixelShaderIndex = rm->LoadPixelShader(currentModelPropertyRaw.pixelShaderPath);
//		CHECK_BOOL_AND_RETURN(pixelShaderIndex != RenderModule::kLoadShaderErrorCode);
//
//		// #todo- Submesh단위 렌더링? Mesh단위 렌더링에 따라 PSO가 결정지어져야 할 수도 있다.
//		// 현재는 Mesh 단위 렌더링을 채택한다.
//		// PSO 생성
//		const int psoIndex = rm->LoadPipelineStateObject(MaterialType::CHARACTER, vertexShaderIndex, pixelShaderIndex);
//		CHECK_BOOL_AND_RETURN(psoIndex != -1);
//
//		SubMesh* submesh = outModel->_subMeshes[i];
//		submesh->_vertexBufferIndex = rm->LoadVertexBuffer(submesh->_vertices);
//		submesh->_indexBufferIndex = rm->LoadPixelBuffer(submesh->_indices);
//		submesh->_pipelineStateObjectIndex = psoIndex;
//
//		// 설명자 힙 제작
//		uint descriptorHeapIndex = rm->CreateDescriptorHeap();
//		if (descriptorHeapIndex == -1)
//		{
//			DK_ASSERT_LOG(false, "Character Submesh의 DescriptorHeap을 만드는 도중에 실패했습니다.");
//			return false;
//		}
//		submesh->_descriptorHeapIndex = descriptorHeapIndex;
//
//		const bool success = _application->GetTextureManager()->LoadTextureSRV(currentModelPropertyRaw.diffuseTexture);
//		if (success == false)
//		{
//			DK_ASSERT_LOG(false, "Texture Load에 실패했습니다. Path: %s\nDefaultTexture를 사용합니다.", currentModelPropertyRaw.diffuseTexture);
//		}
//	}
//
//	return true;
//}