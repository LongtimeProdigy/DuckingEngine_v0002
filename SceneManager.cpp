#include "stdafx.h"
#include "SceneManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ModelProperty.h"
#include "MeshUtil.h"

namespace DK
{
	const float2 SceneManager::ClipMapTerrain::TILE_SCALE = float2::Identity;

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

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(positionArr.data(), sizeof(decltype(positionArr[0])), static_cast<uint32>(positionArr.size()), vertexBufferView, L"SkyeDome_VertexBuffer");
		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, L"SkyDome_IndexBuffer");

		_skyDome._mesh._vertexBufferView = vertexBufferView;
		_skyDome._mesh._indexBufferView = indexBufferView;
		_skyDome._mesh._indexCount = indexArr.size();
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
	static const bool createPrimitiveBuffer(const DKVector<float2>& vertexArr, const DKVector<uint32>& indexArr, VertexBufferViewRef& vertexBufferView, IndexBufferViewRef& indexBufferView)
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView, L"TerrainMesh_VertexBuffer");
		if (vertexBufferSuccess == false)
			return false;

		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, L"TerrainMesh_IndexBuffer");
		if (indexBufferSuccess == false)
			return false;

		return true;
	}

	void SceneManager::loadOcean()
	{
		DKVector<float2> vertexArr;
		DKVector<uint32> indexArr;
		createSquareMesh(SceneManager::Ocean::OCEAN_LENGTH, float2::Identity, vertexArr, indexArr);

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		const bool success = createPrimitiveBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		_ocean._mesh = SceneManager::Mesh(vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()));

		_ocean._initialSpectrumConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createUploadBuffer(sizeof(Ocean::OceanParams), L"_initialSpectrumConstantBuffer");

		// TODO : h0, ht 둘 다 R32, G32만 사용중인데, Bindless연결하려다보니 B32, A32까지 만들었다. 추후에 개선하자
		for (uint32 i = 0; i < RenderModule::kFrameCount; i++)
		{
			_ocean._h0[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
				"OceanH0", SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
				DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				true, true);
			_ocean._ht[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
				"OceanHt", SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
				DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				true, true);
		}
	}

	static SceneManager::Mesh loadLevel_ClipMap_Cross()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION * 8);
		uint32 n = 0;
		// horizontal vertices
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION * 2; i++)
		{
			vertexArr[n++] = float2(i - float(SceneManager::ClipMapTerrain::TILE_RESOLUTION), 0);
			vertexArr[n++] = float2(i - float(SceneManager::ClipMapTerrain::TILE_RESOLUTION), 1);
		}
		const uint32 start_of_vertical = n;

		// vertical vertices
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION * 2; i++)
		{
			vertexArr[n++] = float2(0, i - float(SceneManager::ClipMapTerrain::TILE_RESOLUTION));
			vertexArr[n++] = float2(1, i - float(SceneManager::ClipMapTerrain::TILE_RESOLUTION));
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(SceneManager::ClipMapTerrain::TILE_RESOLUTION * 24 + 6);
		n = 0;
		// horizontal indices
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::TILE_RESOLUTION * 2 + 1; i++)
		{
			uint32 bl = i * 2 + 0;
			uint32 tl = i * 2 + 1;
			uint32 br = i * 2 + 2;
			uint32 tr = i * 2 + 3;

			indexArr[n++] = bl;
			indexArr[n++] = tl;
			indexArr[n++] = tr;
			indexArr[n++] = tr;
			indexArr[n++] = br;
			indexArr[n++] = bl;
		}
		// vertical indices
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::TILE_RESOLUTION * 2 + 1; i++)
		{
			if (i == SceneManager::ClipMapTerrain::TILE_RESOLUTION)
				continue;

			uint32 bl = i * 2 + 0;
			uint32 br = i * 2 + 1;
			uint32 tl = i * 2 + 2;
			uint32 tr = i * 2 + 3;

			indexArr[n++] = start_of_vertical + bl;
			indexArr[n++] = start_of_vertical + tl;
			indexArr[n++] = start_of_vertical + tr;
			indexArr[n++] = start_of_vertical + tr;
			indexArr[n++] = start_of_vertical + br;
			indexArr[n++] = start_of_vertical + bl;
		}
		DK_ASSERT_LOG(n == indexArr.size(), "Size가 안맞습니다.");

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::Mesh loadLevel_ClipMap_Tile()
	{
		DKVector<float2> vertexArr;
		DKVector<uint32> indexArr;
		createSquareMesh(SceneManager::ClipMapTerrain::TILE_RESOLUTION, float2::Identity, vertexArr, indexArr);

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()));
	}
	static SceneManager::Mesh loadLevel_ClipMap_Filter()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION * 8);
		uint32 offset = SceneManager::ClipMapTerrain::TILE_RESOLUTION;
		uint32 n = 0;
		// X
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(offset + i + 1, 0);
			vertexArr[n++] = float2(offset + i + 1, 1);
		}
		// Z
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(1, offset + i + 1);
			vertexArr[n++] = float2(0, offset + i + 1);
		}
		// -X
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 1);
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 0);
		}
		// -Y
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(0, static_cast<float>((offset + i)) * -1);
			vertexArr[n++] = float2(1, static_cast<float>((offset + i)) * -1);
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(SceneManager::ClipMapTerrain::TILE_RESOLUTION * 24);
		n = 0;
		for (uint32 i = 0; i < SceneManager::ClipMapTerrain::TILE_RESOLUTION * 4; i++)
		{
			uint32 arm = i / SceneManager::ClipMapTerrain::TILE_RESOLUTION;
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

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(vertexBufferView, indexBufferView, indexArr.size());
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

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(vertexBufferView, indexBufferView, indexArr.size());
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

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	void SceneManager::loadLevel()
	{
		// 출처: https://mikejsavage.co.uk/blog/geometry-clipmaps.html
		// TileMap
		_clipmapTerrain._cross = loadLevel_ClipMap_Cross();
		_clipmapTerrain._tile = loadLevel_ClipMap_Tile();
		_clipmapTerrain._filter = loadLevel_ClipMap_Filter();
		_clipmapTerrain._trim = loadLevel_ClipMap_Trim();
		_clipmapTerrain._seam = loadLevel_ClipMap_Seam();

		ScopeString<DK_MAX_BUFFER> terrainModelPropertyPath(ConstPath::gTerrainModelProperty);
		terrainModelPropertyPath.append("/TerrainClipMap.xml");
		const ModelPropertyRef modelProperty = DuckingEngine::getInstance().GetResourceManagerWritable().loadModelProperty(terrainModelPropertyPath.c_str());
		const DKVector<MaterialDefinition>& materialDefinitionArr = modelProperty->get_materialDefinitionArr();
		DK_ASSERT_LOG(materialDefinitionArr.size() == 1, "Terrian Material은 현재 Material 1개만 지원합니다.");
		Material* newMaterial = Material::createMaterial(materialDefinitionArr[0]);
		_clipmapTerrain._material.assign(newMaterial);

		// Cross(1) + (tile(4) + Filter(1) + Trim(1) + Seam(1)) * (level count)
		const uint32 tileCountPerClipMap = 1 + (4 * 4 + 1 + 1 + 1) * ClipMapTerrain::NUM_CLIPMAP_LEVELS;
		_clipmapTerrain._terrainConstantBuffer.reserve(tileCountPerClipMap);
		for (uint32 i = 0; i < tileCountPerClipMap; ++i)
		{
			IBuffer* buffer = DuckingEngine::getInstance().GetRenderModuleWritable().createUploadBuffer(sizeof(TerrainMeshConstantBuffer), L"TerrainMesh_CBuffer");
			_clipmapTerrain._terrainConstantBuffer.push_back(buffer);
		}
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
		indexArr[0] = 0; indexArr[1] = 2; indexArr[2] = 1;
		indexArr[3] = 0; indexArr[4] = 3; indexArr[5] = 2;
		return indexArr;
	}
	void SceneManager::loadPostProcess()
	{
		const DKVector<float2> vertexArr = getScreenPlaneVertexBufferData();
		const DKVector<uint32> indexArr = getScreenPlaneIndexBufferData();

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView, L"PostProcess_VertexBuffer");
		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, L"PostProcess_IndexBuffer");

		_postProcess._mesh._vertexBufferView = vertexBufferView;
		_postProcess._mesh._indexBufferView = indexBufferView;
		_postProcess._mesh._indexCount = indexArr.size();
	}
	void SceneManager::loadGbuffer()
	{
		const DKVector<float2> vertexArr = getScreenPlaneVertexBufferData();
		const DKVector<uint32> indexArr = getScreenPlaneIndexBufferData();

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView, L"PostProcess_VertexBuffer");
		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, L"PostProcess_IndexBuffer");

		_gBuffer._mesh._vertexBufferView = vertexBufferView;
		_gBuffer._mesh._indexBufferView = indexBufferView;
		_gBuffer._mesh._indexCount = indexArr.size();
	}
}
