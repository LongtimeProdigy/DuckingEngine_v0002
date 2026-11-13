#include "stdafx.h"
#include "Resource/Material/GrassTiles.h"
#include "SceneManager.h"
#include "FileUtil.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ModelProperty.h"
#include "MeshUtil.h"

namespace DK
{
	void SceneManager::loadSkyDome()
	{
		// createSphere
		// VertexBuffer
		const uint32 tessellationY = 10;
		DK_ASSERT_LOG(tessellationY % 2 == 0, "Sphere의 TessellationY는 반드시 짝수여야합니다.");
		const uint32 tessellationX = 10;
		const float radius = 1.0f; // DK::Math::kFloatMax;

		DKVector<float3> positionArr;
		DKVector<float3> normalArr;
		DKVector<float2> uvArr;
		DKVector<uint32> indexArr;
		const bool success = MeshUtil::createSphere(tessellationX, tessellationY, radius, positionArr, normalArr, uvArr, indexArr);
		DK_ASSERT_LOG(success, "");

		//RenderResourcePtr<ID3D12Resource> vertexBuffer;
		//RenderResourcePtr<ID3D12Resource> indexBuffer;
		//VertexBufferViewRef vertexBufferView;
		//IndexBufferViewRef indexBufferView;
		//RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		//const bool vertexBufferSuccess = renderModule.createVertexBuffer(positionArr.data(), sizeof(decltype(positionArr[0])), static_cast<uint32>(positionArr.size()), vertexBuffer, vertexBufferView, L"SkyeDome_VertexBuffer");
		//const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBuffer, indexBufferView, L"SkyDome_IndexBuffer");

		//_skyDome._mesh._vertexBufferView = vertexBufferView;
		//_skyDome._mesh._indexBufferView = indexBufferView;
		//_skyDome._mesh._indexCount = indexArr.size();
	}

	static const bool createSquareMesh(const uint32 resolution, const float2 resolutionScale, DKVector<float2>& vertexArr, DKVector<uint32>& indexArr)
	{
		const uint32 vertResolution = resolution + 1;

		vertexArr.resize(vertResolution * vertResolution);
		uint32 n = 0;
		for (uint32 y = 0; y < vertResolution; y++)
		{
			for (uint32 x = 0; x < vertResolution; x++)
				vertexArr[n++] = float2(x, y) * resolutionScale;
		}

		indexArr.resize(resolution * resolution * 6);
		n = 0;
		for (uint32 y = 0; y < resolution; y++)
		{
			uint32 yPos0 = y * vertResolution;
			uint32 yPos1 = (y + 1) * vertResolution;
			for (uint32 x = 0; x < resolution; x++) {
				indexArr[n++] = yPos0 + x;
				indexArr[n++] = yPos1 + x;
				indexArr[n++] = yPos1 + x + 1;
				indexArr[n++] = yPos1 + x + 1;
				indexArr[n++] = yPos0 + x + 1;
				indexArr[n++] = yPos0 + x;
			}
		}
		DK_ASSERT_LOG(n == indexArr.size(), "Size가 안맞습니다.");

		return true;
	}
	static const bool createPrimitiveBuffer(const DKVector<float2>& vertexArr, const DKVector<uint32>& indexArr, IBufferRef& vertexBuffer, IBufferRef& indexBuffer, VertexBufferViewRef& vertexBufferView, IndexBufferViewRef& indexBufferView)
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		vertexBuffer = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView, L"TerrainMesh_VertexBuffer");
		if (vertexBuffer == nullptr)
			return false;

		indexBuffer = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, L"TerrainMesh_IndexBuffer");
		if (indexBuffer == nullptr)
			return false;

		return true;
	}

	void SceneManager::loadOcean()
	{
		DKVector<float2> vertexArr;
		DKVector<uint32> indexArr;
		createSquareMesh(Ocean::OCEAN_LENGTH, static_cast<float>(Ocean::OCEAN_N) / static_cast<float>(Ocean::OCEAN_LENGTH), vertexArr, indexArr);

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		const bool success = createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView);
		_ocean._mesh = SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()));

		_ocean._initialSpectrumConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createConstantBuffer(sizeof(Ocean::OceanParams), L"_initialSpectrumConstantBuffer");

		// TODO : h0, ht 둘 다 R32, G32만 사용중인데, Bindless연결하려다보니 B32, A32까지 만들었다. 추후에 개선하자
		for (uint32 i = 0; i < kFrameCount; ++i)
		{
			ScopeString<DK_MAX_BUFFER> tempString;
			StringUtil::itoa(i, tempString.data(), tempString.capacity());

			{
				ScopeString<DK_MAX_BUFFER> debugString;
				debugString.append("OceanH0_");
				debugString.append(tempString.c_str());
				_ocean._h0[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					debugString.c_str(), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i * kFrameCount, tempString.data(), tempString.capacity());
				debugString.append("OceanHt_");
				debugString.append(tempString.c_str());
				_ocean._ht[i * kFrameCount] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					debugString.c_str(), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i * kFrameCount + 1, tempString.data(), tempString.capacity());
				debugString.append("OceanHt_");
				debugString.append(tempString.c_str());
				_ocean._ht[i * kFrameCount + 1] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					debugString.c_str(), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i, tempString.data(), tempString.capacity());
				debugString.append("OceanHeight_");
				debugString.append(tempString.c_str());
				_ocean._height[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					debugString.c_str(), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i, tempString.data(), tempString.capacity());
				debugString.append("OceanNormal_");
				debugString.append(tempString.c_str());
				_ocean._normal[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					debugString.c_str(), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					true, true);
			}
		}
	}

	static SceneManager::Mesh loadLevel_ClipMap_Cross(const uint32 patchVertexResolution, const uint32 tileResolution)
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(patchVertexResolution * 8);
		uint32 n = 0;
		// horizontal vertices
		for (uint32 i = 0; i < patchVertexResolution * 2; i++)
		{
			vertexArr[n++] = float2(i - float(tileResolution), 0);
			vertexArr[n++] = float2(i - float(tileResolution), 1);
		}
		const uint32 start_of_vertical = n;

		// vertical vertices
		for (uint32 i = 0; i < patchVertexResolution * 2; i++)
		{
			vertexArr[n++] = float2(0, i - float(tileResolution));
			vertexArr[n++] = float2(1, i - float(tileResolution));
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(tileResolution * 24 + 6);
		n = 0;
		// horizontal indices
		for (uint32 i = 0; i < tileResolution * 2 + 1; i++)
		{
			const uint32 bl = i * 2 + 0;
			const uint32 tl = i * 2 + 1;
			const uint32 br = i * 2 + 2;
			const uint32 tr = i * 2 + 3;

			indexArr[n++] = bl;
			indexArr[n++] = tl;
			indexArr[n++] = tr;
			indexArr[n++] = tr;
			indexArr[n++] = br;
			indexArr[n++] = bl;
		}
		// vertical indices
		for (uint32 i = 0; i < tileResolution * 2 + 1; i++)
		{
			if (i == tileResolution)
				continue;

			const uint32 bl = i * 2 + 0;
			const uint32 br = i * 2 + 1;
			const uint32 tl = i * 2 + 2;
			const uint32 tr = i * 2 + 3;

			indexArr[n++] = start_of_vertical + bl;
			indexArr[n++] = start_of_vertical + tl;
			indexArr[n++] = start_of_vertical + tr;
			indexArr[n++] = start_of_vertical + tr;
			indexArr[n++] = start_of_vertical + br;
			indexArr[n++] = start_of_vertical + bl;
		}
		DK_ASSERT_LOG(n == indexArr.size(), "Size가 안맞습니다.");

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size(), vertexArr);
	}
	static SceneManager::Mesh loadLevel_ClipMap_Tile(const uint32 tileResolution)
	{
		DKVector<float2> vertexArr;
		DKVector<uint32> indexArr;
		createSquareMesh(tileResolution, float2::Identity, vertexArr, indexArr);

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()), vertexArr);
	}
	static SceneManager::Mesh loadLevel_ClipMap_Filter(const uint32 patchVertexResolution, const uint32 tileResolution)
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(patchVertexResolution * 8);
		uint32 offset = tileResolution;
		uint32 n = 0;
		// X
		for (uint32 i = 0; i < patchVertexResolution; i++)
		{
			vertexArr[n++] = float2(offset + i + 1, 0);
			vertexArr[n++] = float2(offset + i + 1, 1);
		}
		// Z
		for (uint32 i = 0; i < patchVertexResolution; i++)
		{
			vertexArr[n++] = float2(1, offset + i + 1);
			vertexArr[n++] = float2(0, offset + i + 1);
		}
		// -X
		for (uint32 i = 0; i < patchVertexResolution; i++)
		{
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 1);
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 0);
		}
		// -Y
		for (uint32 i = 0; i < patchVertexResolution; i++)
		{
			vertexArr[n++] = float2(0, static_cast<float>((offset + i)) * -1);
			vertexArr[n++] = float2(1, static_cast<float>((offset + i)) * -1);
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(tileResolution * 24);
		n = 0;
		for (uint32 i = 0; i < tileResolution * 4; i++)
		{
			uint32 arm = i / tileResolution;
			uint32 bl = (arm + i) * 2 + 0;
			uint32 tl = (arm + i) * 2 + 1;
			uint32 br = (arm + i) * 2 + 2;
			uint32 tr = (arm + i) * 2 + 3;
			if (arm % 2 == 0)
			{
				indexArr[n++] = bl;
				indexArr[n++] = tl;
				indexArr[n++] = tr;
				indexArr[n++] = tr;
				indexArr[n++] = br;
				indexArr[n++] = bl;
			}
			else
			{
				indexArr[n++] = br;
				indexArr[n++] = bl;
				indexArr[n++] = tl;
				indexArr[n++] = br;
				indexArr[n++] = tl;
				indexArr[n++] = tr;
			}
		}
		DK_ASSERT_LOG(n == indexArr.size(), "Size가 안맞습니다.");

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size(), vertexArr);
	}
	static SceneManager::Mesh loadLevel_ClipMap_Trim()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize((SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 2 + 1) * 2);
		uint32 n = 0;
		// vertical part of L
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION + 1; i++)
		{
			vertexArr[n++] = float2(0, SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION - i);
			vertexArr[n++] = float2(1, SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION - i);
		}
		uint32 start_of_horizontal = n;
		// horizontal part of L
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(i + 1, 0);
			vertexArr[n++] = float2(i + 1, 1);
		}
		// move to center (for Rotation)
		for (float2& v : vertexArr)
			v = v - float2(0.5f * (SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION + 1), 0.5f * (SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION + 1));
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize((SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 2 - 1) * 6);
		n = 0;
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION; i++)
		{
			uint32 bl = (i + 1) * 2 + 0;
			uint32 tl = (i + 0) * 2 + 0;
			uint32 tr = (i + 0) * 2 + 1;
			uint32 br = (i + 1) * 2 + 1;

			indexArr[n++] = bl;
			indexArr[n++] = tl;
			indexArr[n++] = tr;

			indexArr[n++] = tr;
			indexArr[n++] = br;
			indexArr[n++] = bl;

		}
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION - 1; i++)
		{
			uint32 bl = start_of_horizontal + (i + 1) * 2 + 0;
			uint32 tl = start_of_horizontal + (i + 0) * 2 + 0;
			uint32 tr = start_of_horizontal + (i + 0) * 2 + 1;
			uint32 br = start_of_horizontal + (i + 1) * 2 + 1;

			indexArr[n++] = bl;
			indexArr[n++] = tl;
			indexArr[n++] = tr;

			indexArr[n++] = tr;
			indexArr[n++] = br;
			indexArr[n++] = bl;
		}
		DK_ASSERT_LOG(n == indexArr.size(), "Size가 안맞습니다.");

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size(), vertexArr);
	}
	static SceneManager::Mesh loadLevel_ClipMap_Seam()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 4);
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION; i++)
		{
			vertexArr[SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 0 + i] = float2(i, 0);
			vertexArr[SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 1 + i] = float2(SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION, i);
			vertexArr[SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 2 + i] = float2(SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION - i, SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION);
			vertexArr[SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 3 + i] = float2(0, SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION - i);
		}

		DKVector<uint32> indexArr;
		indexArr.resize(SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 6);
		uint32 n = 0;
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::CLIPMAP_VERT_RESOLUTION * 4; i += 2)
		{
			indexArr[n++] = i;
			indexArr[n++] = i + 1;
			indexArr[n++] = i + 2;
		}
		// make the last triangle wrap around
		indexArr[indexArr.size() - 1] = 0;
		DK_ASSERT_LOG(n == indexArr.size(), "Size가 안맞습니다.");

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size(), vertexArr);
	}
	void SceneManager::loadLevel(const DKString& terrainArchive)
	{
		_clipmapTerrain._terrainConstantBuffer.clear();
		_clipmapTerrain._streamer.stop();

		const ScopeString<DK_MAX_PATH> archivePath = GlobalPath::makeResourceFullPath(terrainArchive.c_str());
		if (_clipmapTerrain._chunkArchive.open(FileUtil::fromWidePath(StringUtil::convertCtoWC(archivePath.c_str()))) == false)
		{
			DK_ASSERT_LOG(false, "Terrain archive load failed: %s", archivePath.c_str());
			return;
		}
		DK_LOG("Terrain archive: %u chunks", _clipmapTerrain._chunkArchive.header()._chunkCount);

		initializeTerrainStreaming();

		// 출처: https://mikejsavage.co.uk/blog/geometry-clipmaps.html
		// TileMap
		_clipmapTerrain._cross = loadLevel_ClipMap_Cross(SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION, SceneManager::ClipMapTerrain::TILE_RESOLUTION);
		_clipmapTerrain._tile = loadLevel_ClipMap_Tile(SceneManager::ClipMapTerrain::TILE_RESOLUTION);
		_clipmapTerrain._filter = loadLevel_ClipMap_Filter(SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION, SceneManager::ClipMapTerrain::TILE_RESOLUTION);
		_clipmapTerrain._trim = loadLevel_ClipMap_Trim();
		_clipmapTerrain._seam = loadLevel_ClipMap_Seam();

		ScopeString<DK_MAX_BUFFER> terrainModelPropertyPath(ConstPath::gTerrainModelProperty);
		terrainModelPropertyPath.append("/TerrainClipMap.xml");
		const ModelPropertyRef modelProperty = DuckingEngine::getInstance().GetResourceManagerWritable().loadModelProperty(terrainModelPropertyPath.c_str());
		const DKVector<MaterialDefinition>& materialDefinitionArr = modelProperty->get_materialDefinitionArr();
		DK_ASSERT_LOG(materialDefinitionArr.size() == 1, "Terrian Material은 현재 Material 1개만 지원합니다.");
		MaterialDefinition terrainDefinition = materialDefinitionArr[0];
		if (_clipmapTerrain._streamingConstants._terrainStreamEnabled)
		{
			for (MaterialParameterDefinition& parameter : terrainDefinition._parameters)
			{
				if (parameter._name == "_heightTexture")
					parameter._value = "Texture/0x00000000.png";
			}
		}

		Material* newMaterial = Material::createMaterial(terrainDefinition);
		_clipmapTerrain._material.assign(newMaterial);
		if (_clipmapTerrain._streamingConstants._terrainStreamEnabled == 0)
		{
			for (const Ptr<MaterialParameter>& parameter : newMaterial->_parameterArr)
			{
				if (parameter->getParameterName() == "_heightTexture" && parameter->getType() == MaterialParameterType::TEXTURE)
				{
					const MaterialParameterTexture* heightParameter = static_cast<const MaterialParameterTexture*>(parameter.get());
					const D3D12_RESOURCE_DESC description = const_cast<ID3D12Resource*>(heightParameter->_value->getTextureBuffer().operator->())->GetDesc();
					_clipmapTerrain._streamingConstants._terrainCellsX = static_cast<uint32>(description.Width - 1);
					_clipmapTerrain._streamingConstants._terrainCellsZ = description.Height - 1;
					break;
				}
			}
			_clipmapTerrain._streamingBuffer->upload(&_clipmapTerrain._streamingConstants);
		}

		// Cross(1) + (tile(4) + Filter(1) + Trim(1) + Seam(1)) * (level count)
		const uint32 tileCountPerClipMap = 1 + (4 * 4 + 1 + 1 + 1) * ClipMapTerrain::NUM_CLIPMAP_LEVELS;
		_clipmapTerrain._terrainConstantBuffer.reserve(tileCountPerClipMap);
		for (uint32 i = 0; i < tileCountPerClipMap; ++i)
		{
			IBufferRef buffer = DuckingEngine::getInstance().GetRenderModuleWritable().createConstantBuffer(sizeof(TerrainMeshConstantBuffer), L"TerrainMesh_CBuffer");
			_clipmapTerrain._terrainConstantBuffer.push_back(buffer);
		}

		constexpr uint32 bladeIndices[] = { 0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4, 4, 5, 6 };
		uint32 grassIndices[ClipMapTerrain::GRASS_INDEX_COUNT];
		for (uint32 blade = 0; blade < ClipMapTerrain::GRASS_BLADE_COUNT; ++blade)
		{
			for (uint32 index = 0; index < DK_COUNT_OF(bladeIndices); ++index)
				grassIndices[blade * DK_COUNT_OF(bladeIndices) + index] = blade * 7 + bladeIndices[index];
		}
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		_clipmapTerrain._grassIndexBuffer = renderModule.createIndexBuffer(grassIndices, ClipMapTerrain::GRASS_INDEX_COUNT, _clipmapTerrain._grassIndexBufferView, L"GrassPatch_IndexBuffer");
	}

	static DKVector<float2> getScreenPlaneVertexBufferData()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(4);
		vertexArr[0] = float2(-1, -1);
		vertexArr[1] = float2(1, -1);
		vertexArr[2] = float2(1, 1);
		vertexArr[3] = float2(-1, 1);
		return vertexArr;
	}
	static DKVector<uint32> getScreenPlaneIndexBufferData()
	{
		DKVector<uint32> indexArr;
		indexArr.resize(6);
		indexArr[0] = 0;
		indexArr[1] = 2;
		indexArr[2] = 1;
		indexArr[3] = 0;
		indexArr[4] = 3;
		indexArr[5] = 2;
		return indexArr;
	}

    void SceneManager::initializeTerrainStreaming()
    {
        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        ClipMapTerrain& terrain = _clipmapTerrain;
        ClipMapTerrain::StreamingConstants& constants = terrain._streamingConstants;
        constants = {};
        terrain._streamingBuffer = rm.createConstantBuffer(sizeof(constants), L"TerrainStreaming_CBuffer");
        terrain._streamingBuffer->upload(&constants);
        if (terrain._chunkArchive.isOpen() == false || terrain._chunkArchive.header()._chunkCells < 2)
        {
            DK_ASSERT_LOG(false, "Terrain streaming requires an open archive with at least two cells per chunk");
            return;
        }

        const ArchiveHeader& header = terrain._chunkArchive.header();
        constants._terrainSampleSpacing = header._sampleSpacing;
        constants._terrainHeightOffset = header._heightOffset;
        constants._terrainHeightScale = header._heightScale;
        constants._terrainCellsX = header._width - 1;
        constants._terrainCellsZ = header._height - 1;
        constants._terrainChunkCells = header._chunkCells;
        constants._terrainDetailRadius = 128.0f * header._sampleSpacing;
        constants._terrainTilesX = (header._width - 2) / header._chunkCells + 1;
        constants._terrainTilesZ = (header._height - 2) / header._chunkCells + 1;
        constants._terrainTailLod = overviewLod(header, 256);
        constants._terrainCacheSide = header._chunkCells + 1;
        const uint32 slotCount = constants._terrainTailLod * ClipMapTerrain::CACHE_SLOTS_PER_LOD;
        constants._terrainCacheRows = (slotCount + constants._terrainCacheColumns - 1) / constants._terrainCacheColumns;
        TerrainOverview overview;
        if (terrain._chunkArchive.loadOverview(constants._terrainTailLod, overview) == false)
            return;

        constants._terrainTailWidth = overview._width;
        constants._terrainTailHeightPixels = overview._height;
        const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        const uint32 cacheWidth = constants._terrainCacheSide * constants._terrainCacheColumns;
        const uint32 cacheHeight = constants._terrainCacheSide * constants._terrainCacheRows;
        if (cacheWidth > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || cacheHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        {
            DK_ASSERT_LOG(false, "Terrain cache exceeds the texture size limit");
            return;
        }

        terrain._heightCache = rm.createTexture("TerrainHeightCache", cacheWidth, cacheHeight, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16_UNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._normalCache = rm.createTexture("TerrainNormalCache", cacheWidth, cacheHeight, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16G16_SNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._pageTable = rm.createTexture("TerrainPageTable", constants._terrainPageTableSide, constants._terrainPageTableSide * constants._terrainTailLod, static_cast<const byte*>(nullptr), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._tailHeight = rm.createTexture("TerrainTailHeight", overview._width, overview._height, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16_UNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._tailNormal = rm.createTexture("TerrainTailNormal", overview._width, overview._height, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16G16_SNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        if (terrain._heightCache == nullptr || terrain._normalCache == nullptr || terrain._pageTable == nullptr || terrain._tailHeight == nullptr || terrain._tailNormal == nullptr)
        {
            DK_ASSERT_LOG(false, "Terrain texture creation failed");
            return;
        }

        DKVector<DKArray<float, 4>> pages(size_t(constants._terrainPageTableSide) * constants._terrainPageTableSide * constants._terrainTailLod);
        const bool heightQueued = rm.queueTextureRegion(terrain._tailHeight, 0, 0, overview._width, overview._height, overview._heights.data(), overview._width * 2);
        const bool normalQueued = rm.queueTextureRegion(terrain._tailNormal, 0, 0, overview._width, overview._height, overview._normals.data(), overview._width * 4);
        const bool pagesQueued = rm.queueTextureRegion(terrain._pageTable, 0, 0, constants._terrainPageTableSide, constants._terrainPageTableSide * constants._terrainTailLod, pages.data(), constants._terrainPageTableSide * 16);
        if (heightQueued == false || normalQueued == false || pagesQueued == false || terrain._streamer.start(terrain._chunkArchive, constants._terrainTailLod, slotCount) == false)
        {
            DK_ASSERT_LOG(false, "Terrain streaming initialization failed");
            return;
        }

        constants._terrainStreamEnabled = 1;
        constants._terrainHeightCache = terrain._heightCache->getSRV();
        constants._terrainNormalCache = terrain._normalCache->getSRV();
        constants._terrainPageTable = terrain._pageTable->getSRV();
        constants._terrainTailHeight = terrain._tailHeight->getSRV();
        constants._terrainTailNormal = terrain._tailNormal->getSRV();
        terrain._cacheSlots.assign(slotCount, {});
        terrain._streamingBuffer->upload(&constants);
    }

    void SceneManager::prepareTerrainStreaming(double cameraX, double cameraZ, float deltaTime)
    {
        ClipMapTerrain& terrain = _clipmapTerrain;
        ClipMapTerrain::StreamingConstants& constants = terrain._streamingConstants;
        if (constants._terrainStreamEnabled == 0)
            return;

        for (ClipMapTerrain::CacheSlot& slot : terrain._cacheSlots)
            slot._fade = Math::min(slot._fade + Math::max(deltaTime, 0.0f) * 4.0f, 1.0f);

        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        const ArchiveHeader& header = terrain._chunkArchive.header();
        const DKVector<TileRequest> wanted = selectPages(header, cameraX, cameraZ, constants._terrainDetailRadius, constants._terrainTailLod, ClipMapTerrain::CACHE_SLOTS_PER_LOD);

        TilePage page;
        for (uint32 uploads = 0; uploads < 4 && terrain._streamer.takeReady(page); ++uploads)
        {
            if (page._loaded == false)
            {
                DK_ASSERT_LOG(false, "Terrain page load failed: lod=%u, tile=(%u, %u)", page._request._lod, page._request._x, page._request._z);
                return;
            }

			// 이번 프레임에 필요하지 않게 되어버린 page는 날려버림
            if (std::find(wanted.begin(), wanted.end(), page._request) == wanted.end())
                continue;

            const DKVector<ClipMapTerrain::CacheSlot>::iterator first = terrain._cacheSlots.begin() + page._request._lod * ClipMapTerrain::CACHE_SLOTS_PER_LOD;
            const DKVector<ClipMapTerrain::CacheSlot>::iterator last = first + ClipMapTerrain::CACHE_SLOTS_PER_LOD;
            const DKVector<ClipMapTerrain::CacheSlot>::iterator slot = std::find_if(first, last, [&](const ClipMapTerrain::CacheSlot& value)
				{
					return value._valid == false || std::find(wanted.begin(), wanted.end(), value._request) == wanted.end();
				}
			);
            if (slot == last)
            {
                continue;
            }

            const uint32 index = static_cast<uint32>(slot - terrain._cacheSlots.begin());
            slot->_valid = false;
            slot->_fade = 0;
            const uint32 x = (index % constants._terrainCacheColumns) * constants._terrainCacheSide;
            const uint32 y = (index / constants._terrainCacheColumns) * constants._terrainCacheSide;
            const MipTile& tile = page._tile;
            const bool heightQueued = rm.queueTextureRegion(terrain._heightCache, x, y, tile._side, tile._side, tile._heights.data(), tile._side * 2);
            const bool normalQueued = rm.queueTextureRegion(terrain._normalCache, x, y, tile._side, tile._side, tile._normals.data(), tile._side * 4);
            if (heightQueued == false || normalQueued == false)
            {
                DK_ASSERT_LOG(false, "Terrain page upload failed");
                return;
            }
            slot->_request = page._request;
            slot->_valid = true;
        }

		// PageTable 구성 및 업로드
        const uint32 tableSide = constants._terrainPageTableSide;
        DKVector<DKArray<float, 4>> pages(size_t(tableSide) * tableSide * constants._terrainTailLod);
        for (uint32 index = 0; index < terrain._cacheSlots.size(); ++index)
        {
            const ClipMapTerrain::CacheSlot& slot = terrain._cacheSlots[index];
            if (slot._valid && std::find(wanted.begin(), wanted.end(), slot._request) != wanted.end())
            {
                const uint32 x = slot._request._x % tableSide;
                const uint32 y = slot._request._z % tableSide + slot._request._lod * tableSide;
                pages[size_t(y) * tableSide + x] = { float(index + 1), slot._fade, float(slot._request._x), float(slot._request._z) };
            }
        }
        if (rm.queueTextureRegion(terrain._pageTable, 0, 0, tableSide, tableSide * constants._terrainTailLod, pages.data(), tableSide * sizeof(DKArray<float, 4>)) == false)
        {
            DK_ASSERT_LOG(false, "Terrain page table upload failed");
            return;
        }

		// 필요한 page 로딩 요청
        DKVector<TileRequest> missing;
        for (const TileRequest& request : wanted)
        {
			const uint32 x = request._x % tableSide;
			const uint32 y = request._z % tableSide + request._lod * tableSide;
            const DKArray<float, 4>& resident = pages[size_t(y) * tableSide + x];
            if ((resident[0] == 0 || resident[2] != float(request._x) || resident[3] != float(request._z)))
            {
                missing.push_back(request);
            }
        }
        terrain._streamer.request(missing);

        constants._terrainAnchorX = static_cast<int32>(std::floor(cameraX / header._sampleSpacing));
        constants._terrainAnchorZ = static_cast<int32>(std::floor(cameraZ / header._sampleSpacing));
        constants._terrainCameraX = static_cast<float>(cameraX - double(constants._terrainAnchorX) * header._sampleSpacing);
        constants._terrainCameraZ = static_cast<float>(cameraZ - double(constants._terrainAnchorZ) * header._sampleSpacing);
        constants._terrainCameraWorldX = static_cast<float>(cameraX);
        constants._terrainCameraWorldZ = static_cast<float>(cameraZ);
        const double grassCellMeters = double(GRASS_TILE_METERS) / GRASS_TILE_GRID_SIDE;
        const double anchorX = double(constants._terrainAnchorX) * header._sampleSpacing;
        const double anchorZ = double(constants._terrainAnchorZ) * header._sampleSpacing;
        constants._terrainGrassAnchorX = static_cast<int32>(std::floor(anchorX / grassCellMeters));
        constants._terrainGrassAnchorZ = static_cast<int32>(std::floor(anchorZ / grassCellMeters));
        constants._terrainGrassOffsetX = static_cast<float>(anchorX - double(constants._terrainGrassAnchorX) * grassCellMeters);
        constants._terrainGrassOffsetZ = static_cast<float>(anchorZ - double(constants._terrainGrassAnchorZ) * grassCellMeters);
        terrain._streamingBuffer->upload(&constants);
    }

	void SceneManager::loadPostProcess()
	{
		const DKVector<float2> vertexArr = getScreenPlaneVertexBufferData();
		const DKVector<uint32> indexArr = getScreenPlaneIndexBufferData();

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		if (createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView) == false)
			return;

		_postProcess._mesh._vertexBuffer = vertexBuffer;
		_postProcess._mesh._indexBuffer = indexBuffer;
		_postProcess._mesh._vertexBufferView = vertexBufferView;
		_postProcess._mesh._indexBufferView = indexBufferView;
		_postProcess._mesh._indexCount = indexArr.size();
	}
	void SceneManager::loadGbuffer()
	{
		const DKVector<float2> vertexArr = getScreenPlaneVertexBufferData();
		const DKVector<uint32> indexArr = getScreenPlaneIndexBufferData();

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		if (createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView) == false)
			return;

		_gBuffer._mesh._vertexBuffer = vertexBuffer;
		_gBuffer._mesh._indexBuffer = indexBuffer;
		_gBuffer._mesh._vertexBufferView = vertexBufferView;
		_gBuffer._mesh._indexBufferView = indexBufferView;
		_gBuffer._mesh._indexCount = indexArr.size();
	}
}
