#include "stdafx.h"
#include "SceneManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ModelProperty.h"

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

		float degreeY = (180.0f / tessellationY) * DK::Math::kToRadian;
		float degreeX = (360.0f / tessellationX) * DK::Math::kToRadian;

		DKVector<float3> vertexArr;
		{
			vertexArr.resize((tessellationY - 1) * tessellationX + 2);	// 맨 위, 맨 아래 점 2개는 따로 +로 추가s
			uint32 n = 0;
			vertexArr[n++] = float3(0, radius, 0);
			for (uint32 i = 1; i < tessellationY; ++i)
			{
				float positionY = radius * DK::Math::sin(DK::Math::Half_PI - degreeY * i);
				float radiusXZ = radius * DK::Math::cos(DK::Math::Half_PI - degreeY * i);
				for (uint32 j = 0; j < tessellationX; ++j)
				{
					float positionX = radiusXZ * DK::Math::cos(degreeX * j);
					float positionZ = radiusXZ * DK::Math::sin(degreeX * j);

					vertexArr[n++] = float3(positionX, positionY, positionZ);
				}
			}
			vertexArr[n++] = float3(0, -radius, 0);
			DK_ASSERT_LOG(n == vertexArr.size(), "Sphere VertexCount가 올바르지 않음");
		}

		// IndexBuffer
		// 맨위 삼각형 + 중간라인 + 맨 마지막 점
		const uint32 indexCount = (tessellationX * 3) + (tessellationX * 6) * (tessellationY - 2) + (tessellationX * 3);
		DKVector<uint32> indexArr;
		{
			indexArr.resize(indexCount);
			uint32 n = 0;
			for (uint32 i = 0; i < tessellationY; ++i)
			{
				for (uint32 j = 0; j < tessellationX; ++j)
				{
					uint32 innerCircleVetexIndex0 = i == 0 ? 0 : 1 + (i - 1) * tessellationX + j;
					uint32 innerCircleVetexIndex1 = j == tessellationX - 1 ? 1 + (i - 1) * tessellationX : 1 + (i - 1) * tessellationX + j + 1;
					uint32 outerCircleVertexIndex0 = 1 + i * tessellationX + j;
					uint32 outerCircleVertexIndex1 = i == tessellationY - 1 ? vertexArr.size() - 1 : j == tessellationX - 1 ? 1 + i * tessellationX : 1 + i * tessellationX + j + 1;

					if (i != tessellationY - 1)
					{
						indexArr[n++] = innerCircleVetexIndex0;
						indexArr[n++] = outerCircleVertexIndex0;
						indexArr[n++] = outerCircleVertexIndex1;
					}

					if (i != 0)
					{
						indexArr[n++] = innerCircleVetexIndex0;
						indexArr[n++] = outerCircleVertexIndex1;
						indexArr[n++] = innerCircleVetexIndex1;
					}
				}
			}
			DK_ASSERT_LOG(n == indexArr.size(), "Sphere IndexCount가 올바르지 않음");
		}

		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView, L"SkyeDome_VertexBuffer");
		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, L"SkyDome_IndexBuffer");

		_skyDome._mesh._vertexBufferView = vertexBufferView;
		_skyDome._mesh._indexBufferView = indexBufferView;
		_skyDome._mesh._indexCount = indexArr.size();
	}

	static bool createTerrainMeshBuffer(const DKVector<float2>& vertexArr, DKVector<uint32>& indexArr, VertexBufferViewRef& vertexBufferView, IndexBufferViewRef& indexBufferView)
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
	static SceneManager::Mesh loadLevel_ClipMap_Cross()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(SceneManager::SceneManager::PATCH_VERT_RESOLUTION * 8);
		size_t n = 0;
		// horizontal vertices
		for (uint32 i = 0; i < SceneManager::PATCH_VERT_RESOLUTION * 2; i++)
		{
			vertexArr[n++] = float2(i - float(SceneManager::TILE_RESOLUTION), 0);
			vertexArr[n++] = float2(i - float(SceneManager::TILE_RESOLUTION), 1);
		}
		uint32 start_of_vertical = n;
		// vertical vertices
		for (uint32 i = 0; i < SceneManager::PATCH_VERT_RESOLUTION * 2; i++)
		{
			vertexArr[n++] = float2(0, i - float(SceneManager::TILE_RESOLUTION));
			vertexArr[n++] = float2(1, i - float(SceneManager::TILE_RESOLUTION));
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(SceneManager::TILE_RESOLUTION * 24 + 6);
		n = 0;
		// horizontal indices
		for (uint32 i = 0; i < SceneManager::TILE_RESOLUTION * 2 + 1; i++)
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
		for (uint32 i = 0; i < SceneManager::TILE_RESOLUTION * 2 + 1; i++)
		{
			if (i == SceneManager::TILE_RESOLUTION)
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
		return SceneManager::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::Mesh loadLevel_ClipMap_Tile()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(SceneManager::PATCH_VERT_RESOLUTION * SceneManager::PATCH_VERT_RESOLUTION);
		uint32 n = 0;
		for (uint32 y = 0; y < SceneManager::PATCH_VERT_RESOLUTION; y++)
		{
			for (uint32 x = 0; x < SceneManager::PATCH_VERT_RESOLUTION; x++)
				vertexArr[n++] = float2(x, y);
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(SceneManager::TILE_RESOLUTION * SceneManager::TILE_RESOLUTION * 6);
		n = 0;
		for (uint32 y = 0; y < SceneManager::TILE_RESOLUTION; y++)
		{
			uint32 yPos0 = y * SceneManager::PATCH_VERT_RESOLUTION;
			uint32 yPos1 = (y + 1) * SceneManager::PATCH_VERT_RESOLUTION;
			for (uint32 x = 0; x < SceneManager::TILE_RESOLUTION; x++) {
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
		return SceneManager::Mesh(vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()));
	}
	static SceneManager::Mesh loadLevel_ClipMap_Filter()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(SceneManager::PATCH_VERT_RESOLUTION * 8);
		uint32 offset = SceneManager::TILE_RESOLUTION;
		uint32 n = 0;
		// X
		for (uint32 i = 0; i < SceneManager::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(offset + i + 1, 0);
			vertexArr[n++] = float2(offset + i + 1, 1);
		}
		// Z
		for (uint32 i = 0; i < SceneManager::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(1, offset + i + 1);
			vertexArr[n++] = float2(0, offset + i + 1);
		}
		// -X
		for (uint32 i = 0; i < SceneManager::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 1);
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 0);
		}
		// -Y
		for (uint32 i = 0; i < SceneManager::PATCH_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(0, static_cast<float>((offset + i)) * -1);
			vertexArr[n++] = float2(1, static_cast<float>((offset + i)) * -1);
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(SceneManager::TILE_RESOLUTION * 24);
		n = 0;
		for (uint32 i = 0; i < SceneManager::TILE_RESOLUTION * 4; i++)
		{
			uint32 arm = i / SceneManager::TILE_RESOLUTION;
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
		return SceneManager::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::Mesh loadLevel_ClipMap_Trim()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize((SceneManager::CLIPMAP_VERT_RESOLUTION * 2 + 1) * 2);
		uint32 n = 0;
		// vertical part of L
		for (uint32 i = 0; i < SceneManager::CLIPMAP_VERT_RESOLUTION + 1; i++) 
		{
			vertexArr[n++] = float2(0, SceneManager::CLIPMAP_VERT_RESOLUTION - i);
			vertexArr[n++] = float2(1, SceneManager::CLIPMAP_VERT_RESOLUTION - i);
		}
		uint32 start_of_horizontal = n;
		// horizontal part of L
		for (uint32 i = 0; i < SceneManager::CLIPMAP_VERT_RESOLUTION; i++)
		{
			vertexArr[n++] = float2(i + 1, 0);
			vertexArr[n++] = float2(i + 1, 1);
		}
		// move to center (for Rotation)
		for (float2& v : vertexArr)
			v = v - float2(0.5f * (SceneManager::CLIPMAP_VERT_RESOLUTION + 1), 0.5f * (SceneManager::CLIPMAP_VERT_RESOLUTION + 1));
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize((SceneManager::CLIPMAP_VERT_RESOLUTION * 2 - 1) * 6);
		n = 0;
		for (uint32 i = 0; i < SceneManager::CLIPMAP_VERT_RESOLUTION; i++)
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
		for (uint32 i = 0; i < SceneManager::CLIPMAP_VERT_RESOLUTION - 1; i++)
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
		return SceneManager::Mesh(vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::Mesh loadLevel_ClipMap_Seam()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(SceneManager::CLIPMAP_VERT_RESOLUTION * 4);
		for (uint32 i = 0; i < SceneManager::CLIPMAP_VERT_RESOLUTION; i++)
		{
			vertexArr[SceneManager::CLIPMAP_VERT_RESOLUTION * 0 + i] = float2(i, 0);
			vertexArr[SceneManager::CLIPMAP_VERT_RESOLUTION * 1 + i] = float2(SceneManager::CLIPMAP_VERT_RESOLUTION, i);
			vertexArr[SceneManager::CLIPMAP_VERT_RESOLUTION * 2 + i] = float2(SceneManager::CLIPMAP_VERT_RESOLUTION - i, SceneManager::CLIPMAP_VERT_RESOLUTION);
			vertexArr[SceneManager::CLIPMAP_VERT_RESOLUTION * 3 + i] = float2(0, SceneManager::CLIPMAP_VERT_RESOLUTION - i);
		}

		DKVector<uint32> indexArr;
		indexArr.resize(SceneManager::CLIPMAP_VERT_RESOLUTION * 6);
		uint32 n = 0;
		for (uint32 i = 0; i < SceneManager::CLIPMAP_VERT_RESOLUTION * 4; i += 2)
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
		const uint32 tileCountPerClipMap = 1 + (4 * 4 + 1 + 1 + 1) * NUM_CLIPMAP_LEVELS;
		_clipmapTerrain._terrainConstantBuffer.reserve(tileCountPerClipMap);
		for (uint32 i = 0; i < tileCountPerClipMap; ++i)
		{
			IBuffer* buffer = DuckingEngine::getInstance().GetRenderModuleWritable().createUploadBuffer(sizeof(TerrainMeshConstantBuffer), L"TerrainMesh_CBuffer");
			_clipmapTerrain._terrainConstantBuffer.push_back(buffer);
		}
	}
	void SceneManager::loadPostProcess()
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(4);
		vertexArr[0] = float2(-1, -1);
		vertexArr[1] = float2(1, -1);
		vertexArr[2] = float2(1, 1);
		vertexArr[3] = float2(-1, 1);

		DKVector<uint32> indexArr;
		indexArr.resize(6);
		indexArr[0] = 0; indexArr[1] = 2; indexArr[2] = 1;
		indexArr[3] = 0; indexArr[4] = 3; indexArr[5] = 2;

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView, L"PostProcess_VertexBuffer");
		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, L"PostProcess_IndexBuffer");

		_postProcess._mesh._vertexBufferView = vertexBufferView;
		_postProcess._mesh._indexBufferView = indexBufferView;
		_postProcess._mesh._indexCount = indexArr.size();
	}
}
