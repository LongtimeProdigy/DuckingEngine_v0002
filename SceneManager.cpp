#include "stdafx.h"
#include "SceneManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ModelProperty.h"

namespace DK
{
	static SceneManager::Terrain loadNormalTerrain()
	{
		/*
		* 128*128Unit == 1Sector
		* Vertex Distance == 1M
		*/
		const uint32 horizontalVertexCountPerSector = 4096;
		const uint32 verticalVertexCountPerSector = 4096;
		const float distancePerVertex = 0.125f;

		struct TerrainVertex
		{
			TerrainVertex(const float2& position, const float2 uv)
				: _position(position)
				, _uv(uv)
			{}

			float2 _position;
			float2 _uv;
		};

		DKVector<TerrainVertex> vertexArr;
		vertexArr.reserve(verticalVertexCountPerSector * horizontalVertexCountPerSector);
		for (uint32 i = 0; i < verticalVertexCountPerSector; ++i)
		{
			const float positionY = distancePerVertex * i;
			const float v = i / (float)(verticalVertexCountPerSector - 1);
			for (uint32 j = 0; j < horizontalVertexCountPerSector; ++j)
			{
				const float positionX = distancePerVertex * j;
				const float u = j / (float)(horizontalVertexCountPerSector - 1);

				TerrainVertex vertex(float2(positionX, positionY), float2(u, v));
				vertexArr.push_back(DK::move(vertex));
			}
		}

		DKVector<uint32> indexArr;
		const uint32 totalFaceCount = (horizontalVertexCountPerSector - 1) * (verticalVertexCountPerSector - 1);
		indexArr.reserve(totalFaceCount);
		for (uint32 i = 0; i < verticalVertexCountPerSector - 1; ++i)
		{
			uint32 verticalVertexIndex = i * verticalVertexCountPerSector;
			uint32 verticalVertexIndex1 = (i + 1) * verticalVertexCountPerSector;
			for (uint32 j = 0; j < horizontalVertexCountPerSector - 1; ++j)
			{
				// Face 1
				indexArr.push_back(verticalVertexIndex + j);
				indexArr.push_back(verticalVertexIndex1 + j);
				indexArr.push_back(verticalVertexIndex1 + j + 1);

				// Face 2
				indexArr.push_back(verticalVertexIndex1 + j + 1);
				indexArr.push_back(verticalVertexIndex + j + 1);
				indexArr.push_back(verticalVertexIndex + j);
			}
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		VertexBufferViewRef vertexBufferView;
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView);

		IndexBufferViewRef indexBufferView;
		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView);

		const ModelPropertyRef modelProperty = DuckingEngine::getInstance().GetResourceManagerWritable().loadModelProperty("Terrian/ModelProperty/TerrainStandard.xml");
		const DKVector<MaterialDefinition>& materialDefinitionArr = modelProperty->get_materialDefinitionArr();
		DK_ASSERT_LOG(materialDefinitionArr.size() == 1, "Terrian Material은 현재 Material 1개만 지원합니다.");
		Material* newMaterial = Material::createMaterial(materialDefinitionArr[0]);

		return SceneManager::Terrain(vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()), newMaterial);
	}

	static bool createTerrainMeshBuffer(const DKVector<float3>& vertexArr, DKVector<uint32>& indexArr, VertexBufferViewRef& vertexBufferView, IndexBufferViewRef& indexBufferView)
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView);
		if (vertexBufferSuccess == false)
			return false;

		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView);
		if (indexBufferSuccess == false)
			return false;

		return true;
	}
	static SceneManager::ClipMapTerrain::Mesh loadLevel_ClipMap_Cross()
	{
		DKVector<float3> vertexArr;
		vertexArr.resize(PATCH_VERT_RESOLUTION * 8);
		size_t n = 0;
		// horizontal vertices
		for (uint32 i = 0; i < PATCH_VERT_RESOLUTION * 2; i++)
		{
			vertexArr[n++] = float3(i - float(TILE_RESOLUTION), 0, 0);
			vertexArr[n++] = float3(i - float(TILE_RESOLUTION), 0, 1);
		}
		uint32 start_of_vertical = n;
		// vertical vertices
		for (uint32 i = 0; i < PATCH_VERT_RESOLUTION * 2; i++)
		{
			vertexArr[n++] = float3(0, 0, i - float(TILE_RESOLUTION));
			vertexArr[n++] = float3(1, 0, i - float(TILE_RESOLUTION));
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(TILE_RESOLUTION * 24 + 6);
		n = 0;
		// horizontal indices
		for (uint32 i = 0; i < TILE_RESOLUTION * 2 + 1; i++)
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
		for (uint32 i = 0; i < TILE_RESOLUTION * 2 + 1; i++)
		{
			if (i == TILE_RESOLUTION)
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
		createTerrainMeshBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::ClipMapTerrain::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::ClipMapTerrain::Mesh loadLevel_ClipMap_Tile()
	{
		DKVector<float3> vertexArr;
		vertexArr.resize(PATCH_VERT_RESOLUTION * PATCH_VERT_RESOLUTION);
		uint32 n = 0;
		for (uint32 y = 0; y < PATCH_VERT_RESOLUTION; y++)
		{
			for (uint32 x = 0; x < PATCH_VERT_RESOLUTION; x++)
				vertexArr[n++] = float3(x, 0, y);
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(TILE_RESOLUTION * TILE_RESOLUTION * 6);
		n = 0;
		for (uint32 y = 0; y < TILE_RESOLUTION; y++)
		{
			uint32 yPos0 = y * PATCH_VERT_RESOLUTION;
			uint32 yPos1 = (y + 1) * PATCH_VERT_RESOLUTION;
			for (uint32 x = 0; x < TILE_RESOLUTION; x++) {
				indexArr[n++] = yPos0 + x;
				indexArr[n++] = yPos1 + x;
				indexArr[n++] = yPos1 + x + 1;
				indexArr[n++] = yPos1 + x + 1;
				indexArr[n++] = yPos0 + x + 1;
				indexArr[n++] = yPos0 + x;
			}
		}
		DK_ASSERT_LOG(n == indexArr.size(), "Size가 안맞습니다.");

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createTerrainMeshBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::ClipMapTerrain::Mesh(vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()));
	}
	static SceneManager::ClipMapTerrain::Mesh loadLevel_ClipMap_Filter()
	{
		DKVector<float3> vertexArr;
		vertexArr.resize(PATCH_VERT_RESOLUTION * 8);
		uint32 offset = TILE_RESOLUTION;
		uint32 n = 0;
		// X
		for (uint32 i = 0; i < PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float3(offset + i + 1, 0, 0);
			vertexArr[n++] = float3(offset + i + 1, 0, 1);
		}
		// Z
		for (uint32 i = 0; i < PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float3(1, 0, offset + i + 1);
			vertexArr[n++] = float3(0, 0, offset + i + 1);
		}
		// -X
		for (uint32 i = 0; i < PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float3(static_cast<float>((offset + i)) * -1, 0, 1);
			vertexArr[n++] = float3(static_cast<float>((offset + i)) * -1, 0, 0);
		}
		// -Y
		for (uint32 i = 0; i < PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float3(0, 0, static_cast<float>((offset + i)) * -1);
			vertexArr[n++] = float3(1, 0, static_cast<float>((offset + i)) * -1);
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(TILE_RESOLUTION * 24);
		n = 0;
		for (uint32 i = 0; i < TILE_RESOLUTION * 4; i++)
		{
			uint32 arm = i / TILE_RESOLUTION;
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
		createTerrainMeshBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::ClipMapTerrain::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::ClipMapTerrain::Mesh loadLevel_ClipMap_Trim()
	{
		DKVector<float3> vertexArr;
		vertexArr.resize((CLIPMAP_VERT_RESOLUTION * 2 + 1) * 2);
		uint32 n = 0;
		// vertical part of L
		for (uint32 i = 0; i < CLIPMAP_VERT_RESOLUTION + 1; i++) 
		{
			vertexArr[n++] = float3(0, 0, CLIPMAP_VERT_RESOLUTION - i);
			vertexArr[n++] = float3(1, 0, CLIPMAP_VERT_RESOLUTION - i);
		}
		uint32 start_of_horizontal = n;
		// horizontal part of L
		for (uint32 i = 0; i < CLIPMAP_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float3(i + 1, 0, 0);
			vertexArr[n++] = float3(i + 1, 0, 1);
		}
		// move to center (for Rotation)
		for (float3& v : vertexArr) {
			v = v - float3(0.5f * (CLIPMAP_VERT_RESOLUTION + 1), 0, 0.5f * (CLIPMAP_VERT_RESOLUTION + 1));
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize((CLIPMAP_VERT_RESOLUTION * 2 - 1) * 6);
		n = 0;
		for (uint32 i = 0; i < CLIPMAP_VERT_RESOLUTION; i++)
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
		for (uint32 i = 0; i < CLIPMAP_VERT_RESOLUTION - 1; i++)
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
		createTerrainMeshBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::ClipMapTerrain::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::ClipMapTerrain::Mesh loadLevel_ClipMap_Seam()
	{
		DKVector<float3> vertexArr;
		vertexArr.resize(CLIPMAP_VERT_RESOLUTION * 4);
		for (uint32 i = 0; i < CLIPMAP_VERT_RESOLUTION; i++)
		{
			vertexArr[CLIPMAP_VERT_RESOLUTION * 0 + i] = float3(i, 0, 0);
			vertexArr[CLIPMAP_VERT_RESOLUTION * 1 + i] = float3(CLIPMAP_VERT_RESOLUTION, 0, i);
			vertexArr[CLIPMAP_VERT_RESOLUTION * 2 + i] = float3(CLIPMAP_VERT_RESOLUTION - i, 0, CLIPMAP_VERT_RESOLUTION);
			vertexArr[CLIPMAP_VERT_RESOLUTION * 3 + i] = float3(0, 0, CLIPMAP_VERT_RESOLUTION - i);
		}

		DKVector<uint32> indexArr;
		indexArr.resize(CLIPMAP_VERT_RESOLUTION * 6);
		uint32 n = 0;
		for (uint32 i = 0; i < CLIPMAP_VERT_RESOLUTION * 4; i += 2)
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
		createTerrainMeshBuffer(vertexArr, indexArr, vertexBufferView, indexBufferView);
		return SceneManager::ClipMapTerrain::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	void SceneManager::loadLevel()
	{
		// 출처: https://mikejsavage.co.uk/blog/geometry-clipmaps.html
		// TileMap
		ClipMapTerrain::Mesh cross = loadLevel_ClipMap_Cross();
		ClipMapTerrain::Mesh tile = loadLevel_ClipMap_Tile();
		ClipMapTerrain::Mesh filter = loadLevel_ClipMap_Filter();
		ClipMapTerrain::Mesh trim = loadLevel_ClipMap_Trim();
		ClipMapTerrain::Mesh seam = loadLevel_ClipMap_Seam();

		const ModelPropertyRef modelProperty = DuckingEngine::getInstance().GetResourceManagerWritable().loadModelProperty("Terrian/ModelProperty/TerrainStandard.xml");
		const DKVector<MaterialDefinition>& materialDefinitionArr = modelProperty->get_materialDefinitionArr();
		DK_ASSERT_LOG(materialDefinitionArr.size() == 1, "Terrian Material은 현재 Material 1개만 지원합니다.");
		Material* newMaterial = Material::createMaterial(materialDefinitionArr[0]);

		ClipMapTerrain terrain(DK::move(tile), DK::move(filter), DK::move(trim), DK::move(cross), DK::move(seam), newMaterial);

		// Cross(1) + tile(NUM_CLIPMAP_LEVELS * 4 * 4)
		const uint32 tileCountPerClipMap = 1 + (4 * 4 + 1 + 1 + 1) * NUM_CLIPMAP_LEVELS;
		terrain._terrainConstantBuffer.reserve(tileCountPerClipMap);
		for (uint32 i = 0; i < tileCountPerClipMap; ++i)
		{
			IBuffer* buffer = DuckingEngine::getInstance().GetRenderModuleWritable().createUploadBuffer(sizeof(TerrainMeshConstantBuffer));
			terrain._terrainConstantBuffer.push_back(buffer);
		}

		_clipmapTerrainContainer.push_back(DK::move(terrain));
	}
}
