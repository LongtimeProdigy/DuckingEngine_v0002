#include "stdafx.h"
#include "SceneManager.h"
#include "SceneObjectManager.h"
#include "FileUtil.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ModelProperty.h"
#include "MeshUtil.h"
#include "Resource/Material/GrassTiles.h"

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
		//const bool vertexBufferSuccess = renderModule.createVertexBuffer(positionArr.data(), sizeof(decltype(positionArr[0])), static_cast<uint32>(positionArr.size()), vertexBuffer, vertexBufferView, DK_DEBUG_NAME(L"SkyeDome_VertexBuffer"));
		//const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBuffer, indexBufferView, DK_DEBUG_NAME(L"SkyDome_IndexBuffer"));

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
		vertexBuffer = renderModule.createVertexBuffer(vertexArr.data(), sizeof(decltype(vertexArr[0])), static_cast<uint32>(vertexArr.size()), vertexBufferView, DK_DEBUG_NAME(L"TerrainMesh_VertexBuffer"));
		if (vertexBuffer == nullptr)
			return false;

		indexBuffer = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView, DK_DEBUG_NAME(L"TerrainMesh_IndexBuffer"));
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

		_ocean._initialSpectrumConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createConstantBuffer(sizeof(Ocean::OceanParams), DK_DEBUG_NAME(L"_initialSpectrumConstantBuffer"));

		// TODO : h0, ht 둘 다 R32, G32만 사용중인데, Bindless연결하려다보니 B32, A32까지 만들었다. 추후에 개선하자
		for (uint32 i = 0; i < kFrameCount; ++i)
		{
#if defined(_DK_DEBUG_)
			ScopeString<DK_MAX_BUFFER> tempString;
			StringUtil::itoa(i, tempString.data(), tempString.capacity());
#endif

			{
#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> debugString;
				debugString.append("OceanH0_");
				debugString.append(tempString.c_str());
#endif
				_ocean._h0[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					DK_DEBUG_NAME(debugString.c_str()), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i * kFrameCount, tempString.data(), tempString.capacity());
				debugString.append("OceanHt_");
				debugString.append(tempString.c_str());
#endif
				_ocean._ht[i * kFrameCount] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					DK_DEBUG_NAME(debugString.c_str()), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i * kFrameCount + 1, tempString.data(), tempString.capacity());
				debugString.append("OceanHt_");
				debugString.append(tempString.c_str());
#endif
				_ocean._ht[i * kFrameCount + 1] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					DK_DEBUG_NAME(debugString.c_str()), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i, tempString.data(), tempString.capacity());
				debugString.append("OceanHeight_");
				debugString.append(tempString.c_str());
#endif
				_ocean._height[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					DK_DEBUG_NAME(debugString.c_str()), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
					DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					true, true);
			}

			{
#if defined(_DK_DEBUG_)
				ScopeString<DK_MAX_BUFFER> debugString;
				StringUtil::itoa(i, tempString.data(), tempString.capacity());
				debugString.append("OceanNormal_");
				debugString.append(tempString.c_str());
#endif
				_ocean._normal[i] = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture(
					DK_DEBUG_NAME(debugString.c_str()), SceneManager::Ocean::OCEAN_N, SceneManager::Ocean::OCEAN_N, nullptr, 1,
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
	static bool readOptionalLevelFloat(const TiXmlElement& element, const char* name, float& value)
	{
		const int result = element.QueryFloatAttribute(name, &value);
		return result == TIXML_NO_ATTRIBUTE || (result == TIXML_SUCCESS && std::isfinite(value));
	}

	bool SceneManager::loadLevel(const DKString& levelPath)
	{
		const ScopeString<DK_MAX_PATH> fullPath = GlobalPath::makeResourceFullPath(levelPath);
		FileReader reader;
		DKString text;
		if (reader.open(fullPath.c_str()) == false || reader.readText(text) == false)
		{
			DK_ASSERT_LOG(false, "Level file load failed: %s", fullPath.c_str());
			return false;
		}

		TiXmlDocument document;
		document.Parse(text.c_str());
		const TiXmlElement* level = document.FirstChildElement("Level");
		const TiXmlElement* terrain = level == nullptr ? nullptr : level->FirstChildElement("Terrain");
		if (document.Error() || terrain == nullptr)
		{
			DK_ASSERT_LOG(false, "Invalid level terrain definition: %s", fullPath.c_str());
			return false;
		}

		const char* terrainArchive = terrain->Attribute("Archive");
		const char* terrainModelProperty = terrain->Attribute("ModelProperty");
		const char* terrainHeightTexture = terrain->Attribute("HeightTexture");
		if (terrainArchive == nullptr || terrainArchive[0] == '\0' || terrainModelProperty == nullptr || terrainModelProperty[0] == '\0' || terrainHeightTexture == nullptr || terrainHeightTexture[0] == '\0')
		{
			DK_ASSERT_LOG(false, "Terrain needs an archive, model property and height texture: %s", fullPath.c_str());
			return false;
		}

		float2 terrainWorldOrigin(0.0f, 0.0f);
		if (readOptionalLevelFloat(*terrain, "OriginX", terrainWorldOrigin.x) == false || readOptionalLevelFloat(*terrain, "OriginZ", terrainWorldOrigin.y) == false)
		{
			DK_ASSERT_LOG(false, "Terrain origin must be finite: %s", fullPath.c_str());
			return false;
		}

		if (loadTerrain(terrainArchive, terrainModelProperty, terrainHeightTexture, terrainWorldOrigin) == false)
			return false;

		_grass = {};
		const TiXmlElement* grass = level->FirstChildElement("Grass");
		if (grass != nullptr)
		{
			const char* placementMap = grass->Attribute("PlacementMap");
			if (placementMap == nullptr || placementMap[0] == '\0')
			{
				DK_ASSERT_LOG(false, "Grass needs a placement map: %s", fullPath.c_str());
				return false;
			}
			int tileGridSide = GRASS_TILE_GRID_SIDE_DEFAULT;
			if (grass->Attribute("TileGridSide") != nullptr && (grass->QueryIntAttribute("TileGridSide", &tileGridSide) != TIXML_SUCCESS || tileGridSide < 32 || tileGridSide > 128 || tileGridSide % 32 != 0))
			{
				DK_ASSERT_LOG(false, "Grass TileGridSide must be a multiple of 32 between 32 and 128: %s", fullPath.c_str());
				return false;
			}
			const ClipMapTerrain::StreamingConstants& terrainConstants = _clipmapTerrain._streamingConstants;
			_grass._grassMapOriginX = terrainConstants._terrainOriginWorldX;
			_grass._grassMapOriginZ = terrainConstants._terrainOriginWorldZ;
			_grass._grassMapWidth = terrainConstants._terrainCellCountX * terrainConstants._terrainSampleWorldSpacing;
			_grass._grassMapDepth = terrainConstants._terrainCellCountZ * terrainConstants._terrainSampleWorldSpacing;
			if (readOptionalLevelFloat(*grass, "MapOriginX", _grass._grassMapOriginX) == false ||
				readOptionalLevelFloat(*grass, "MapOriginZ", _grass._grassMapOriginZ) == false ||
				readOptionalLevelFloat(*grass, "MapWidth", _grass._grassMapWidth) == false ||
				readOptionalLevelFloat(*grass, "MapDepth", _grass._grassMapDepth) == false ||
				_grass._grassMapWidth <= 0.0f || _grass._grassMapDepth <= 0.0f)
			{
				DK_ASSERT_LOG(false, "Grass map needs a finite origin and positive extent: %s", fullPath.c_str());
				return false;
			}
			if (loadGrass(placementMap, static_cast<uint32>(tileGridSide)) == false)
				return false;
		}

		for (const TiXmlElement* placements = level->FirstChildElement("StaticMeshPlacements"); placements != nullptr; placements = placements->NextSiblingElement("StaticMeshPlacements"))
		{
			const char* placementPath = placements->Attribute("Path");
			if (placementPath == nullptr || placementPath[0] == '\0')
			{
				DK_ASSERT_LOG(false, "Static mesh placements need a path: %s", fullPath.c_str());
				return false;
			}
			if (DuckingEngine::getInstance().GetSceneObjectManagerWritable().loadStaticMeshPlacements(placementPath) == false)
				return false;
		}
		return true;
	}

	bool SceneManager::loadTerrain(const DKString& terrainArchive, const DKString& modelPropertyPath, const DKString& heightTexturePath, const float2& terrainWorldOrigin)
	{
		_clipmapTerrain._terrainConstantBuffer.clear();
		_clipmapTerrain._streamer.stop();

		const ScopeString<DK_MAX_PATH> archivePath = GlobalPath::makeResourceFullPath(terrainArchive.c_str());
		if (_clipmapTerrain._chunkArchive.open(FileUtil::fromWidePath(StringUtil::convertCtoWC(archivePath.c_str()))) == false)
		{
			DK_ASSERT_LOG(false, "Terrain archive load failed: %s", archivePath.c_str());
			return false;
		}
		DK_LOG("Terrain archive: %u chunks", _clipmapTerrain._chunkArchive.header()._chunkCount);

		initializeTerrainStreaming(terrainWorldOrigin);

		// 출처: https://mikejsavage.co.uk/blog/geometry-clipmaps.html
		// TileMap
		_clipmapTerrain._cross = loadLevel_ClipMap_Cross(SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION, SceneManager::ClipMapTerrain::TILE_RESOLUTION);
		_clipmapTerrain._tile = loadLevel_ClipMap_Tile(SceneManager::ClipMapTerrain::TILE_RESOLUTION);
		_clipmapTerrain._filter = loadLevel_ClipMap_Filter(SceneManager::ClipMapTerrain::PATCH_VERT_RESOLUTION, SceneManager::ClipMapTerrain::TILE_RESOLUTION);
		_clipmapTerrain._trim = loadLevel_ClipMap_Trim();
		_clipmapTerrain._seam = loadLevel_ClipMap_Seam();

		const ModelPropertyRef modelProperty = DuckingEngine::getInstance().GetResourceManagerWritable().loadModelProperty(modelPropertyPath);
		if (modelProperty == nullptr)
			return false;
		const DKVector<MaterialDefinition>& materials = modelProperty->get_materialDefinitionArr();
		if (materials.size() != 1)
		{
			DK_ASSERT_LOG(false, "Terrain requires one material: %s", modelPropertyPath.c_str());
			return false;
		}
		_clipmapTerrain._material.assign(Material::createMaterial(materials[0]));
		if (_clipmapTerrain._material.get() == nullptr)
			return false;

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		ClipMapTerrain::StreamingConstants& constants = _clipmapTerrain._streamingConstants;
		_clipmapTerrain._heightTexture = nullptr;
		if (constants._terrainStreamingEnabled == 0)
		{
			_clipmapTerrain._heightTexture = renderModule.loadAndCreateTexture(heightTexturePath);
			if (_clipmapTerrain._heightTexture == nullptr)
				return false;
			const D3D12_RESOURCE_DESC description = const_cast<ID3D12Resource*>(_clipmapTerrain._heightTexture->getTextureBuffer().operator->())->GetDesc();
			if (description.Width < 2 || description.Height < 2)
			{
				DK_ASSERT_LOG(false, "Invalid terrain height texture: %s", heightTexturePath.c_str());
				return false;
			}
			constants._terrainFullHeightTextureSRV = _clipmapTerrain._heightTexture->getSRV();
			constants._terrainCellCountX = static_cast<uint32>(description.Width - 1);
			constants._terrainCellCountZ = description.Height - 1;
			_clipmapTerrain._streamingBuffer->upload(&constants);
		}

		// Cross(1) + (tile(4) + Filter(1) + Trim(1) + Seam(1)) * (level count)
		const uint32 tileCountPerClipMap = 1 + (4 * 4 + 1 + 1 + 1) * ClipMapTerrain::NUM_CLIPMAP_LEVELS;
		_clipmapTerrain._terrainConstantBuffer.reserve(tileCountPerClipMap);
		for (uint32 i = 0; i < tileCountPerClipMap; ++i)
		{
			IBufferRef buffer = renderModule.createConstantBuffer(sizeof(TerrainMeshConstantBuffer), DK_DEBUG_NAME(L"TerrainMesh_CBuffer"));
			if (buffer == nullptr)
				return false;
			_clipmapTerrain._terrainConstantBuffer.push_back(buffer);
		}
		return true;
	}

	bool SceneManager::loadGrass(const DKString& placementTexturePath, uint32 tileGridSide)
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		_grass._tileGridSide = tileGridSide;
		_grass._placementTexture = renderModule.loadAndCreateTexture(placementTexturePath);
		if (_grass._placementTexture == nullptr)
			return false;

		constexpr uint32 bladeIndices[] = { 0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4, 4, 5, 6 };
		uint32 grassIndices[Grass::GRASS_INDEX_COUNT];
		for (uint32 blade = 0; blade < Grass::GRASS_BLADE_COUNT; ++blade)
		{
			for (uint32 index = 0; index < DK_COUNT_OF(bladeIndices); ++index)
				grassIndices[blade * DK_COUNT_OF(bladeIndices) + index] = blade * 7 + bladeIndices[index];
		}
		_grass._indexBuffer = renderModule.createIndexBuffer(grassIndices, Grass::GRASS_INDEX_COUNT, _grass._indexBufferView, DK_DEBUG_NAME(L"GrassPatch_IndexBuffer"));
		return _grass._indexBuffer != nullptr;
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

    void SceneManager::initializeTerrainStreaming(const float2& terrainWorldOrigin)
    {
        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        ClipMapTerrain& terrain = _clipmapTerrain;
        ClipMapTerrain::StreamingConstants& constants = terrain._streamingConstants;
        constants = {};
        constants._terrainOriginWorldX = terrainWorldOrigin.x;
        constants._terrainOriginWorldZ = terrainWorldOrigin.y;
        terrain._streamingBuffer = rm.createConstantBuffer(sizeof(constants), DK_DEBUG_NAME(L"TerrainStreaming_CBuffer"));
        if (terrain._chunkArchive.isOpen() == false || terrain._chunkArchive.header()._chunkCells < 2)
        {
			terrain._streamingBuffer->upload(&constants);
            DK_ASSERT_LOG(false, "Terrain streaming requires an open archive with at least two cells per chunk");
            return;
        }

		auto overviewLod = [](const ArchiveHeader& header, uint32 maxCells)->uint32
		{
			uint32 lod = 1;
			while (((std::max(header._width, header._height) - 2) >> lod) + 1 > maxCells)
			{
				++lod;
			}
			return std::max(lod, 3u);
		};

        const ArchiveHeader& header = terrain._chunkArchive.header();
        constants._terrainSampleWorldSpacing = header._sampleSpacing;
        constants._terrainBaseWorldHeight = header._heightOffset;
        constants._terrainWorldHeightRange = header._heightScale;
        constants._terrainCellCountX = header._width - 1;
        constants._terrainCellCountZ = header._height - 1;
        constants._terrainChunkCellCountPerSide = header._chunkCells;
        constants._terrainLod0TileCountX = (header._width - 2) / header._chunkCells + 1;
        constants._terrainLod0TileCountZ = (header._height - 2) / header._chunkCells + 1;
        constants._terrainOverviewLod = overviewLod(header, 256);
        constants._terrainCacheSlotTexelCountPerSide = header._chunkCells + 1;
        const uint32 slotCount = constants._terrainOverviewLod * ClipMapTerrain::CACHE_SLOTS_PER_LOD;
        constants._terrainCacheSlotRowCount = (slotCount + constants._terrainCacheSlotColumnCount - 1) / constants._terrainCacheSlotColumnCount;
        TerrainOverview overview;
        if (terrain._chunkArchive.loadOverview(constants._terrainOverviewLod, overview) == false)
            return;

        constants._terrainOverviewSampleCountX = overview._width;
        constants._terrainOverviewSampleCountZ = overview._height;
        const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        const uint32 cacheWidth = constants._terrainCacheSlotTexelCountPerSide * constants._terrainCacheSlotColumnCount;
        const uint32 cacheHeight = constants._terrainCacheSlotTexelCountPerSide * constants._terrainCacheSlotRowCount;
        if (cacheWidth > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || cacheHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        {
            DK_ASSERT_LOG(false, "Terrain cache exceeds the texture size limit");
            return;
        }

        terrain._heightCache = rm.createTexture(DK_DEBUG_NAME("TerrainHeightCache"), cacheWidth, cacheHeight, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16_UNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._normalCache = rm.createTexture(DK_DEBUG_NAME("TerrainNormalCache"), cacheWidth, cacheHeight, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16G16_SNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._pageTable = rm.createTexture(DK_DEBUG_NAME("TerrainPageTable"), constants._terrainPageTableEntryCountPerSide, constants._terrainPageTableEntryCountPerSide * constants._terrainOverviewLod, static_cast<const byte*>(nullptr), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._tailHeight = rm.createTexture(DK_DEBUG_NAME("TerrainTailHeight"), overview._width, overview._height, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16_UNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._tailNormal = rm.createTexture(DK_DEBUG_NAME("TerrainTailNormal"), overview._width, overview._height, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16G16_SNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        if (terrain._heightCache == nullptr || terrain._normalCache == nullptr || terrain._pageTable == nullptr || terrain._tailHeight == nullptr || terrain._tailNormal == nullptr)
        {
            DK_ASSERT_LOG(false, "Terrain texture creation failed");
            return;
        }

        DKVector<DKArray<float, 4>> pages(size_t(constants._terrainPageTableEntryCountPerSide) * constants._terrainPageTableEntryCountPerSide * constants._terrainOverviewLod);
        const bool heightQueued = rm.queueTextureRegion(terrain._tailHeight, 0, 0, overview._width, overview._height, overview._heights.data(), overview._width * 2);
        const bool normalQueued = rm.queueTextureRegion(terrain._tailNormal, 0, 0, overview._width, overview._height, overview._normals.data(), overview._width * 4);
        const bool pagesQueued = rm.queueTextureRegion(terrain._pageTable, 0, 0, constants._terrainPageTableEntryCountPerSide, constants._terrainPageTableEntryCountPerSide * constants._terrainOverviewLod, pages.data(), constants._terrainPageTableEntryCountPerSide * 16);
        if (heightQueued == false || normalQueued == false || pagesQueued == false || terrain._streamer.start(terrain._chunkArchive, constants._terrainOverviewLod, slotCount) == false)
        {
            DK_ASSERT_LOG(false, "Terrain streaming initialization failed");
            return;
        }

        constants._terrainStreamingEnabled = 1;
        constants._terrainHeightCacheSRV = terrain._heightCache->getSRV();
        constants._terrainNormalCacheSRV = terrain._normalCache->getSRV();
        constants._terrainPageTableSRV = terrain._pageTable->getSRV();
        constants._terrainOverviewHeightSRV = terrain._tailHeight->getSRV();
        constants._terrainOverviewNormalSRV = terrain._tailNormal->getSRV();
        terrain._cacheSlots.assign(slotCount, {});
        terrain._streamingBuffer->upload(&constants);
    }

    void SceneManager::prepareTerrainStreaming(double cameraX, double cameraZ, float deltaTime)
    {
        ClipMapTerrain& terrain = _clipmapTerrain;
        ClipMapTerrain::StreamingConstants& constants = terrain._streamingConstants;
        if (terrain._streamingBuffer == nullptr)
            return;

        const double archiveCameraX = cameraX - constants._terrainOriginWorldX;
        const double archiveCameraZ = cameraZ - constants._terrainOriginWorldZ;
        constants._terrainAnchorSampleX = static_cast<int32>(std::floor(archiveCameraX / constants._terrainSampleWorldSpacing));
        constants._terrainAnchorSampleZ = static_cast<int32>(std::floor(archiveCameraZ / constants._terrainSampleWorldSpacing));
        constants._terrainCameraAnchorRelativeWorldX = static_cast<float>(archiveCameraX - double(constants._terrainAnchorSampleX) * constants._terrainSampleWorldSpacing);
        constants._terrainCameraAnchorRelativeWorldZ = static_cast<float>(archiveCameraZ - double(constants._terrainAnchorSampleZ) * constants._terrainSampleWorldSpacing);
        constants._terrainCameraWorldX = static_cast<float>(cameraX);
        constants._terrainCameraWorldZ = static_cast<float>(cameraZ);
        if (constants._terrainStreamingEnabled == 0)
        {
            terrain._streamingBuffer->upload(&constants);
            return;
        }

        for (ClipMapTerrain::CacheSlot& slot : terrain._cacheSlots)
            slot._fade = Math::min(slot._fade + Math::max(deltaTime, 0.0f) * 4.0f, 1.0f);

        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        const ArchiveHeader& header = terrain._chunkArchive.header();
        const DKVector<TileRequest> wanted = selectPages(header, archiveCameraX, archiveCameraZ, constants._terrainLod0WorldRadius, constants._terrainOverviewLod, ClipMapTerrain::CACHE_SLOTS_PER_LOD);

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
            const uint32 x = (index % constants._terrainCacheSlotColumnCount) * constants._terrainCacheSlotTexelCountPerSide;
            const uint32 y = (index / constants._terrainCacheSlotColumnCount) * constants._terrainCacheSlotTexelCountPerSide;
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
        const uint32 tableSide = constants._terrainPageTableEntryCountPerSide;
        DKVector<DKArray<float, 4>> pages(size_t(tableSide) * tableSide * constants._terrainOverviewLod);
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
        if (rm.queueTextureRegion(terrain._pageTable, 0, 0, tableSide, tableSide * constants._terrainOverviewLod, pages.data(), tableSide * sizeof(DKArray<float, 4>)) == false)
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
