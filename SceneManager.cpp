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
#include "Resource/Material/TerrainShared.h"

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

		_ocean._initialSpectrumConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createConstantBuffer(sizeof(OceanParams), DK_DEBUG_NAME(L"_initialSpectrumConstantBuffer"));

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

	static SceneManager::Mesh loadLevel_ClipMap_Cross(const uint32 patchVertexCountPerSide, const uint32 patchCellCountPerSide)
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(patchVertexCountPerSide * 8);
		uint32 n = 0;
		// horizontal vertices
		for (uint32 i = 0; i < patchVertexCountPerSide * 2; i++)
		{
			vertexArr[n++] = float2(i - float(patchCellCountPerSide), 0);
			vertexArr[n++] = float2(i - float(patchCellCountPerSide), 1);
		}
		const uint32 start_of_vertical = n;

		// vertical vertices
		for (uint32 i = 0; i < patchVertexCountPerSide * 2; i++)
		{
			vertexArr[n++] = float2(0, i - float(patchCellCountPerSide));
			vertexArr[n++] = float2(1, i - float(patchCellCountPerSide));
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(patchCellCountPerSide * 24 + 6);
		n = 0;
		// horizontal indices
		for (uint32 i = 0; i < patchCellCountPerSide * 2 + 1; i++)
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
		for (uint32 i = 0; i < patchCellCountPerSide * 2 + 1; i++)
		{
			if (i == patchCellCountPerSide)
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
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size());
	}
	static SceneManager::Mesh loadLevel_ClipMap_Patch(const uint32 patchCellCountPerSide)
	{
		DKVector<float2> vertexArr;
		DKVector<uint32> indexArr;
		createSquareMesh(patchCellCountPerSide, float2::Identity, vertexArr, indexArr);

		IBufferRef vertexBuffer;
		IBufferRef indexBuffer;
		VertexBufferViewRef vertexBufferView;
		IndexBufferViewRef indexBufferView;
		createPrimitiveBuffer(vertexArr, indexArr, vertexBuffer, indexBuffer, vertexBufferView, indexBufferView);
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()));
	}
	static SceneManager::Mesh loadLevel_ClipMap_Filter(const uint32 patchVertexCountPerSide, const uint32 patchCellCountPerSide)
	{
		DKVector<float2> vertexArr;
		vertexArr.resize(patchVertexCountPerSide * 8);
		uint32 offset = patchCellCountPerSide;
		uint32 n = 0;
		// X
		for (uint32 i = 0; i < patchVertexCountPerSide; i++)
		{
			vertexArr[n++] = float2(offset + i + 1, 0);
			vertexArr[n++] = float2(offset + i + 1, 1);
		}
		// Z
		for (uint32 i = 0; i < patchVertexCountPerSide; i++)
		{
			vertexArr[n++] = float2(1, offset + i + 1);
			vertexArr[n++] = float2(0, offset + i + 1);
		}
		// -X
		for (uint32 i = 0; i < patchVertexCountPerSide; i++)
		{
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 1);
			vertexArr[n++] = float2(static_cast<float>((offset + i)) * -1, 0);
		}
		// -Y
		for (uint32 i = 0; i < patchVertexCountPerSide; i++)
		{
			vertexArr[n++] = float2(0, static_cast<float>((offset + i)) * -1);
			vertexArr[n++] = float2(1, static_cast<float>((offset + i)) * -1);
		}
		DK_ASSERT_LOG(n == vertexArr.size(), "Size가 안맞습니다.");

		DKVector<uint32> indexArr;
		indexArr.resize(patchCellCountPerSide * 24);
		n = 0;
		for (uint32 i = 0; i < patchCellCountPerSide * 4; i++)
		{
			uint32 arm = i / patchCellCountPerSide;
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
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size());
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
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size());
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
		return SceneManager::Mesh(DK::move(vertexBuffer), DK::move(indexBuffer), vertexBufferView, indexBufferView, indexArr.size());
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

		const char* terrainWorld = terrain->Attribute("World");
		const char* terrainModelProperty = terrain->Attribute("ModelProperty");
		if (terrainWorld == nullptr || terrainWorld[0] == '\0' || terrainModelProperty == nullptr || terrainModelProperty[0] == '\0')
		{
			DK_ASSERT_LOG(false, "Terrain needs a world manifest and model property: %s", fullPath.c_str());
			return false;
		}

		float2 terrainWorldOrigin(0.0f, 0.0f);
		if (readOptionalLevelFloat(*terrain, "OriginX", terrainWorldOrigin.x) == false || readOptionalLevelFloat(*terrain, "OriginZ", terrainWorldOrigin.y) == false)
		{
			DK_ASSERT_LOG(false, "Terrain origin must be finite: %s", fullPath.c_str());
			return false;
		}

		if (loadTerrain(terrainWorld, terrainModelProperty, terrainWorldOrigin) == false)
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
			const TerrainStreamingConstants& terrainConstants = _clipmapTerrain._streamingConstants;
			_grass._grassMapOriginX = terrainConstants._terrainOriginWorldX + terrainConstants._terrainMinimumSampleX * terrainConstants._terrainSampleWorldSpacing;
			_grass._grassMapOriginZ = terrainConstants._terrainOriginWorldZ + terrainConstants._terrainMinimumSampleZ * terrainConstants._terrainSampleWorldSpacing;
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
			if (readOptionalLevelFloat(*grass, "HeightMaskStrength", _grass._grassHeightMaskStrength) == false || _grass._grassHeightMaskStrength < 0.0f || _grass._grassHeightMaskStrength > 1.0f)
			{
				DK_ASSERT_LOG(false, "Grass HeightMaskStrength must be between 0 and 1: %s", fullPath.c_str());
				return false;
			}
			if (readOptionalLevelFloat(*grass, "AlbedoScale", _grass._grassAlbedoScale) == false || _grass._grassAlbedoScale < 0.0f)
			{
				DK_ASSERT_LOG(false, "Grass AlbedoScale must be nonnegative: %s", fullPath.c_str());
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

	bool SceneManager::loadTerrain(const DKString& terrainWorld, const DKString& modelPropertyPath, const float2& terrainWorldOrigin)
	{
		static_assert(ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE >= 4 && ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE % 2 == 0, "Terrain clipmap patch cell count must be even and at least four cells");
		static_assert(ClipMapTerrain::NUM_CLIPMAP_LEVELS > 0 && ClipMapTerrain::NUM_CLIPMAP_LEVELS < 31 && ClipMapTerrain::BASE_VERTEX_SPACING > 0.0f, "Terrain clipmap levels and vertex spacing must be positive and fit the grid shift range");
		_clipmapTerrain._terrainConstantBuffer.clear();
		_clipmapTerrain._streamer.stop();

		const ScopeString<DK_MAX_PATH> worldPath = GlobalPath::makeResourceFullPath(terrainWorld.c_str());
		if (_clipmapTerrain._world.open(worldPath.c_str()) == false || initializeTerrainStreaming(terrainWorldOrigin) == false)
			return false;

		// 출처: https://mikejsavage.co.uk/blog/geometry-clipmaps.html
		// Clipmap meshes
		_clipmapTerrain._cross = loadLevel_ClipMap_Cross(SceneManager::ClipMapTerrain::PATCH_VERTEX_COUNT_PER_SIDE, SceneManager::ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE);
		_clipmapTerrain._patch = loadLevel_ClipMap_Patch(SceneManager::ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE);
		_clipmapTerrain._filter = loadLevel_ClipMap_Filter(SceneManager::ClipMapTerrain::PATCH_VERTEX_COUNT_PER_SIDE, SceneManager::ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE);
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

		// Cross(1) + (patch(4) + Filter(1) + Trim(1) + Seam(1)) * (level count)
		const uint32 meshCountPerClipMap = 1 + (4 * 4 + 1 + 1 + 1) * ClipMapTerrain::NUM_CLIPMAP_LEVELS;
		_clipmapTerrain._terrainConstantBuffer.reserve(meshCountPerClipMap);
		for (uint32 i = 0; i < meshCountPerClipMap; ++i)
		{
			IBufferRef buffer = DuckingEngine::getInstance().GetRenderModuleWritable().createConstantBuffer(sizeof(TerrainMeshConstantBuffer), DK_DEBUG_NAME(L"TerrainMesh_CBuffer"));
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
		static_assert(DK_COUNT_OF(bladeIndices) == GRASS_BLADE_INDEX_COUNT);
		uint32 grassIndices[Grass::GRASS_INDEX_COUNT];
		for (uint32 blade = 0; blade < GRASS_BLADE_COUNT; ++blade)
		{
			for (uint32 index = 0; index < DK_COUNT_OF(bladeIndices); ++index)
				grassIndices[blade * DK_COUNT_OF(bladeIndices) + index] = blade * GRASS_BLADE_VERTEX_COUNT + bladeIndices[index];
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

    bool SceneManager::initializeTerrainStreaming(const float2& terrainWorldOrigin)
    {
        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        ClipMapTerrain& terrain = _clipmapTerrain;
        TerrainStreamingConstants& constants = terrain._streamingConstants;
        const TerrainWorld& world = terrain._world;

        constants = {};
        constants._terrainOriginWorldX = terrainWorldOrigin.x;
        constants._terrainOriginWorldZ = terrainWorldOrigin.y;
        constants._terrainSampleWorldSpacing = world._sampleWorldSpacing;
        constants._terrainBaseWorldHeight = world._baseWorldHeight;
        constants._terrainWorldHeightRange = world._worldHeightRange;
        constants._terrainCellCountX = world._sampleCountX - 1;
        constants._terrainCellCountZ = world._sampleCountZ - 1;
        constants._terrainMinimumSampleX = world._minimumSampleX;
        constants._terrainMinimumSampleZ = world._minimumSampleZ;
        constants._terrainChunkCellCountPerSide = world._chunkCellCountPerSide;
        constants._terrainSectionCellCountPerSide = world._sectionCellCountPerSide;
        constants._terrainSectionCountX = world._sectionCountX;
        constants._terrainSectionCountZ = world._sectionCountZ;
        constants._terrainOverviewLod = world._overviewLod;
        const uint32 overviewSampleStep = 1u << world._overviewLod;
        if (world._sectionCellCountPerSide % overviewSampleStep != 0)
        {
            DK_ASSERT_LOG(false, "Terrain section cell count must be divisible by the overview sample step");
            return false;
        }
        constants._terrainSectionOverviewSampleCountPerSide = world._sectionCellCountPerSide / overviewSampleStep + 1;
        constants._terrainOverviewAtlasTexelCountX = constants._terrainSectionOverviewSampleCountPerSide * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
        constants._terrainOverviewAtlasTexelCountY = constants._terrainSectionOverviewSampleCountPerSide * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
        constants._terrainCacheSlotTexelCountPerSide = world._chunkCellCountPerSide + 1;
        const uint32 cacheSlotCount = world._overviewLod * ClipMapTerrain::CACHE_SLOTS_PER_LOD;
        if (constants._terrainCacheSlotColumnCount == 0 || cacheSlotCount % constants._terrainCacheSlotColumnCount != 0)
        {
            DK_ASSERT_LOG(false, "Terrain cache slot count must be divisible by the cache column count");
            return false;
        }
        constants._terrainCacheSlotRowCount = cacheSlotCount / constants._terrainCacheSlotColumnCount;
        const uint32 cacheAtlasTexelCountX = constants._terrainCacheSlotTexelCountPerSide * constants._terrainCacheSlotColumnCount;
        const uint32 cacheAtlasTexelCountY = constants._terrainCacheSlotTexelCountPerSide * constants._terrainCacheSlotRowCount;
        if (std::isfinite(constants._terrainLod0WorldRadius) == false || constants._terrainLod0WorldRadius <= 0.0f)
        {
            DK_ASSERT_LOG(false, "Terrain streaming LOD zero radius must be finite and positive");
            return false;
        }
        DK_ASSERT_LOG(world._chunkCellCountPerSide > 0 && world._overviewLod > 0 && world._overviewLod < 32, "Terrain page table requires a valid chunk size and streaming LOD count");
        const uint32 coarsestTileLod0CellCountPerSide = world._chunkCellCountPerSide << (world._overviewLod - 1);
        DK_ASSERT_LOG(world._sectionCellCountPerSide > 0 && world._sectionCellCountPerSide % coarsestTileLod0CellCountPerSide == 0, "Terrain section must contain a whole grid of tiles at every streaming LOD");
        constants._terrainPageTableWidth = world._sectionCellCountPerSide / world._chunkCellCountPerSide;
        const uint32 sectionPageTableRowCount = terrainPageTableLodRowOffset(constants._terrainPageTableWidth, world._overviewLod);
        const uint32 tableHeight = sectionPageTableRowCount * TERRAIN_SECTION_SLOT_COUNT;
#if defined(_DK_DEBUG_)
        const double clipmapRadius = double(ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE * 2 - 2) * ClipMapTerrain::BASE_VERTEX_SPACING * (1u << (ClipMapTerrain::NUM_CLIPMAP_LEVELS - 1));
        if (cacheAtlasTexelCountX > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || cacheAtlasTexelCountY > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION
			|| constants._terrainPageTableWidth > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || tableHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION
			|| constants._terrainOverviewAtlasTexelCountX > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || double(world._sectionCellCountPerSide) * world._sampleWorldSpacing * 2.0 > clipmapRadius)
        {
            DK_ASSERT_LOG(false, "Terrain section layout exceeds the cache or clipmap coverage");
            return false;
        }
#endif

        const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        terrain._streamingBuffer = rm.createConstantBuffer(sizeof(constants), DK_DEBUG_NAME(L"TerrainStreaming_CBuffer"));
        terrain._heightCache = rm.createTexture(DK_DEBUG_NAME("TerrainHeightCache"), cacheAtlasTexelCountX, cacheAtlasTexelCountY, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16_UNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._normalCache = rm.createTexture(DK_DEBUG_NAME("TerrainNormalCache"), cacheAtlasTexelCountX, cacheAtlasTexelCountY, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16G16_SNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._pageTable = rm.createTexture(DK_DEBUG_NAME("TerrainPageTable"), constants._terrainPageTableWidth, tableHeight, static_cast<const byte*>(nullptr), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._overviewHeightAtlas = rm.createTexture(DK_DEBUG_NAME("TerrainSectionHeight"), constants._terrainOverviewAtlasTexelCountX, constants._terrainOverviewAtlasTexelCountY, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16_UNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        terrain._overviewNormalAtlas = rm.createTexture(DK_DEBUG_NAME("TerrainSectionNormal"), constants._terrainOverviewAtlasTexelCountX, constants._terrainOverviewAtlasTexelCountY, static_cast<const byte*>(nullptr), DXGI_FORMAT_R16G16_SNORM, D3D12_RESOURCE_FLAG_NONE, state, true, false);
        if (terrain._streamingBuffer == nullptr || terrain._heightCache == nullptr || terrain._normalCache == nullptr || terrain._pageTable == nullptr || terrain._overviewHeightAtlas == nullptr || terrain._overviewNormalAtlas == nullptr)
        {
            DK_ASSERT_LOG(false, "Terrain streaming resource creation failed");
            return false;
        }

        DKVector<float4> pageEntries(size_t(constants._terrainPageTableWidth) * tableHeight);
        if (rm.queueTextureRegion(terrain._pageTable, 0, 0, constants._terrainPageTableWidth, tableHeight, pageEntries.data(), constants._terrainPageTableWidth * sizeof(pageEntries[0])) == false
			|| terrain._streamer.start(world, cacheSlotCount) == false)
        {
            return false;
        }

        constants._terrainStreamingEnabled = 1;
        constants._terrainHeightCacheSRV = terrain._heightCache->getSRV();
        constants._terrainNormalCacheSRV = terrain._normalCache->getSRV();
        constants._terrainPageTableSRV = terrain._pageTable->getSRV();
        constants._terrainOverviewHeightAtlasSRV = terrain._overviewHeightAtlas->getSRV();
        constants._terrainOverviewNormalAtlasSRV = terrain._overviewNormalAtlas->getSRV();
        terrain._tileCacheEntries.assign(cacheSlotCount, {});
        terrain._streamingBuffer->upload(&constants);
        return true;
    }

    void SceneManager::prepareTerrainStreaming(double cameraX, double cameraZ, float deltaTime)
    {
        ClipMapTerrain& terrain = _clipmapTerrain;
        TerrainStreamingConstants& constants = terrain._streamingConstants;
        if (terrain._streamingBuffer == nullptr)
            return;

        DK_ASSERT_LOG(std::isfinite(cameraX) && std::isfinite(cameraZ) && std::isfinite(deltaTime) && deltaTime >= 0.0f, "Terrain streaming update requires finite camera coordinates and a nonnegative delta time");
        DK_ASSERT_LOG(constants._terrainOverviewLod == terrain._world._overviewLod && constants._terrainSectionCellCountPerSide == terrain._world._sectionCellCountPerSide && constants._terrainChunkCellCountPerSide == terrain._world._chunkCellCountPerSide && constants._terrainSampleWorldSpacing == terrain._world._sampleWorldSpacing, "Terrain streaming layout must match the loaded world");
        DK_ASSERT_LOG(constants._terrainMinimumSampleX == terrain._world._minimumSampleX && constants._terrainMinimumSampleZ == terrain._world._minimumSampleZ, "Terrain streaming bounds must match the loaded world");
        DK_ASSERT_LOG(constants._terrainCacheSlotColumnCount > 0 && constants._terrainPageTableWidth == constants._terrainSectionCellCountPerSide / constants._terrainChunkCellCountPerSide && constants._terrainPageTableWidth > 0 && terrain._tileCacheEntries.size() == size_t(constants._terrainOverviewLod) * ClipMapTerrain::CACHE_SLOTS_PER_LOD && terrain._tileCacheEntries.size() == size_t(constants._terrainCacheSlotColumnCount) * constants._terrainCacheSlotRowCount, "Terrain cache slots must fill the allocated atlas grid");
        const double cameraRelativeToTerrainOriginWorldX = cameraX - constants._terrainOriginWorldX;
        const double cameraRelativeToTerrainOriginWorldZ = cameraZ - constants._terrainOriginWorldZ;
        const double cameraAnchorSampleX = std::floor(cameraRelativeToTerrainOriginWorldX / constants._terrainSampleWorldSpacing);
        const double cameraAnchorSampleZ = std::floor(cameraRelativeToTerrainOriginWorldZ / constants._terrainSampleWorldSpacing);
        DK_ASSERT_LOG(std::abs(cameraAnchorSampleX) < 2147483647.0 - 1048576.0 && std::abs(cameraAnchorSampleZ) < 2147483647.0 - 1048576.0, "Terrain camera anchor exceeds the shader integer coordinate range");
        constants._terrainCameraAnchorSampleX = static_cast<int32>(cameraAnchorSampleX);
        constants._terrainCameraAnchorSampleZ = static_cast<int32>(cameraAnchorSampleZ);
        constants._terrainCameraLocalX = static_cast<float>(cameraRelativeToTerrainOriginWorldX - cameraAnchorSampleX * constants._terrainSampleWorldSpacing);
        constants._terrainCameraLocalZ = static_cast<float>(cameraRelativeToTerrainOriginWorldZ - cameraAnchorSampleZ * constants._terrainSampleWorldSpacing);
        constants._terrainCameraWorldX = static_cast<float>(cameraX);
        constants._terrainCameraWorldZ = static_cast<float>(cameraZ);

        const TerrainWorld& world = terrain._world;
		RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();

		// 카메라 주변 필요한 selection 목록 가져오기
        const DKVector<uint32> currentSections = selectSections(world, cameraRelativeToTerrainOriginWorldX, cameraRelativeToTerrainOriginWorldZ);

		// 이전까지 랜더한 section이 카메라 주변이 아니면 invalid처리
        for (TerrainSectionConstants& section : constants._terrainSections)
        {
            const uint32 id = section._terrainSectionIndexZ * world._sectionCountX + section._terrainSectionIndexX;
            if (std::find(currentSections.begin(), currentSections.end(), id) == currentSections.end())
                section._terrainSectionResident = 0;
        }

		// 로딩 완료된 section이 카메라 주변이 아니면 날리고, 카메라 주변이면 Section Overview를 GPU에 업로드 준비
        SectionLoadResult sectionLoadResult;
        for (uint32 uploads = 0; uploads < 2 && terrain._streamer.takeReady(sectionLoadResult);)
        {
            if (sectionLoadResult._loaded == false)
            {
                DK_ASSERT_LOG(false, "Terrain section load failed: %u", sectionLoadResult._sectionId);
                return;
            }
            DK_ASSERT_LOG(sectionLoadResult._sectionId < world._sections.size(), "Loaded terrain section ID is outside the world");
            if (std::find(currentSections.begin(), currentSections.end(), sectionLoadResult._sectionId) == currentSections.end())
                continue;

            const TerrainSection& section = world._sections[sectionLoadResult._sectionId];
            const uint32 sectionSlotIndexX = section._sectionIndexX % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
            const uint32 sectionSlotIndexY = section._sectionIndexZ % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
            const TerrainSampleData& overview = sectionLoadResult._overview;
            const uint32 overviewSampleCountPerSide = constants._terrainSectionOverviewSampleCountPerSide;
            DK_ASSERT_LOG(overview._heights.size() == size_t(overviewSampleCountPerSide) * overviewSampleCountPerSide && overview._normals.size() == overview._heights.size(), "Terrain section upload must exactly fill one overview atlas slot");
            const uint32 texelX = sectionSlotIndexX * constants._terrainSectionOverviewSampleCountPerSide;
            const uint32 texelY = sectionSlotIndexY * constants._terrainSectionOverviewSampleCountPerSide;
            DK_ASSERT_LOG(texelX + overviewSampleCountPerSide <= constants._terrainOverviewAtlasTexelCountX && texelY + overviewSampleCountPerSide <= constants._terrainOverviewAtlasTexelCountY, "Terrain section upload is outside the overview atlas");
            if (rm.queueTextureRegion(terrain._overviewHeightAtlas, texelX, texelY, overviewSampleCountPerSide, overviewSampleCountPerSide, overview._heights.data(), overviewSampleCountPerSide * sizeof(overview._heights[0])) == false ||
                rm.queueTextureRegion(terrain._overviewNormalAtlas, texelX, texelY, overviewSampleCountPerSide, overviewSampleCountPerSide, overview._normals.data(), overviewSampleCountPerSide * sizeof(overview._normals[0])) == false)
                return;

            TerrainSectionConstants& metadata = constants._terrainSections[sectionSlotIndexY * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE + sectionSlotIndexX];
            metadata._terrainSectionIndexX = section._sectionIndexX;
            metadata._terrainSectionIndexZ = section._sectionIndexZ;
            metadata._terrainSectionResident = 1;

			++uploads;
        }

		// 카메라 주변 section이 만약 이전까지 랜더링 되지 않은 section이라면 로딩 요청을 위해 수집
        DKVector<uint32> missingSections;
        for (const uint32 sectionId : currentSections)
        {
            const TerrainSection& section = world._sections[sectionId];
            const uint32 sectionSlotIndexX = section._sectionIndexX % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
            const uint32 sectionSlotIndexY = section._sectionIndexZ % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
            const TerrainSectionConstants& metadata = constants._terrainSections[sectionSlotIndexY * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE + sectionSlotIndexX];
            if (metadata._terrainSectionResident == 0 || metadata._terrainSectionIndexX != section._sectionIndexX || metadata._terrainSectionIndexZ != section._sectionIndexZ)
            {
                DK_ASSERT_LOG(metadata._terrainSectionResident == 0, "Terrain section slot must be nonresident before requesting a new section");
                missingSections.push_back(sectionId);
            }
        }

		// Tile cache entry 업데이트
        for (ClipMapTerrain::TileCacheEntry& cacheEntry : terrain._tileCacheEntries)
            cacheEntry._fade = Math::min(cacheEntry._fade + deltaTime * 4.0f, 1.0f);

		// 요청이 필요한 Tile들 LOD별로 목록 가져오기
        const DKVector<TileRequest> currentTiles = selectTiles(world, currentSections, cameraRelativeToTerrainOriginWorldX, cameraRelativeToTerrainOriginWorldZ, constants._terrainLod0WorldRadius, ClipMapTerrain::CACHE_SLOTS_PER_LOD);

		// 로딩 완료된 Tile중에 현재 필요한 section에 포함되는 tile이면 slot정보를 채우고 GPU에 업로드
        TileLoadResult tileLoadResult;
        for (uint32 uploads = 0; uploads < 4 && terrain._streamer.takeReady(tileLoadResult);)
        {
            if (tileLoadResult._loaded == false)
            {
                DK_ASSERT_LOG(false, "Terrain chunk load failed: section=%u", tileLoadResult._request._sectionId);
                return;
            }
            DK_ASSERT_LOG(tileLoadResult._request._sectionId < world._sections.size() && tileLoadResult._request._lod < constants._terrainOverviewLod, "Loaded terrain tile section ID or LOD is outside the streaming layout");
            if (std::find(currentTiles.begin(), currentTiles.end(), tileLoadResult._request) == currentTiles.end())
                continue;

            const DKVector<ClipMapTerrain::TileCacheEntry>::iterator first = terrain._tileCacheEntries.begin() + tileLoadResult._request._lod * ClipMapTerrain::CACHE_SLOTS_PER_LOD;
            const DKVector<ClipMapTerrain::TileCacheEntry>::iterator last = first + ClipMapTerrain::CACHE_SLOTS_PER_LOD;
            const DKVector<ClipMapTerrain::TileCacheEntry>::iterator cacheEntry = std::find_if(first, last, [&](const ClipMapTerrain::TileCacheEntry& value)
            {
                return value._valid == false || std::find(currentTiles.begin(), currentTiles.end(), value._request) == currentTiles.end();
            });

            DK_ASSERT_LOG(cacheEntry != last, "A requested terrain tile must have an available cache slot in its LOD budget");
            const uint32 cacheSlotIndex = static_cast<uint32>(cacheEntry - terrain._tileCacheEntries.begin());
			const uint32 cacheSlotIndexX = cacheSlotIndex % constants._terrainCacheSlotColumnCount;
			const uint32 cacheSlotIndexY = cacheSlotIndex / constants._terrainCacheSlotColumnCount;
            const uint32 texelX = cacheSlotIndexX * constants._terrainCacheSlotTexelCountPerSide;
			const uint32 texelY = cacheSlotIndexY * constants._terrainCacheSlotTexelCountPerSide;
            const MipTile& tile = tileLoadResult._tile;
            DK_ASSERT_LOG(tile._tileLocalIndexX == tileLoadResult._request._tileLocalIndexX && tile._tileLocalIndexZ == tileLoadResult._request._tileLocalIndexZ && tile._lod == tileLoadResult._request._lod && tile._sampleCountPerSide == constants._terrainCacheSlotTexelCountPerSide && tile._heights.size() == size_t(tile._sampleCountPerSide) * tile._sampleCountPerSide && tile._normals.size() == tile._heights.size(), "Terrain tile upload must match its request and exactly fill one cache slot");
            DK_ASSERT_LOG(cacheSlotIndex < terrain._tileCacheEntries.size() && texelY + tile._sampleCountPerSide <= constants._terrainCacheSlotRowCount * constants._terrainCacheSlotTexelCountPerSide, "Terrain tile upload is outside the cache atlas");
            if (rm.queueTextureRegion(terrain._heightCache, texelX, texelY, tile._sampleCountPerSide, tile._sampleCountPerSide, tile._heights.data(), tile._sampleCountPerSide * sizeof(tile._heights[0])) == false ||
                rm.queueTextureRegion(terrain._normalCache, texelX, texelY, tile._sampleCountPerSide, tile._sampleCountPerSide, tile._normals.data(), tile._sampleCountPerSide * sizeof(tile._normals[0])) == false)
                return;

            cacheEntry->_request = tileLoadResult._request;
            cacheEntry->_fade = 0;
            cacheEntry->_valid = true;

			++uploads;
        }

		// CacheTile정보로 PageTable정보를 구성하고 업로드
        const uint32 tableWidth = constants._terrainPageTableWidth;
        const uint32 sectionPageTableRowCount = terrainPageTableLodRowOffset(tableWidth, constants._terrainOverviewLod);
        const uint32 tableHeight = sectionPageTableRowCount * TERRAIN_SECTION_SLOT_COUNT;
        DKVector<float4> pageEntries(size_t(tableWidth) * tableHeight);
        for (uint32 cacheSlotIndex = 0; cacheSlotIndex < terrain._tileCacheEntries.size(); ++cacheSlotIndex)
        {
            const ClipMapTerrain::TileCacheEntry& cacheEntry = terrain._tileCacheEntries[cacheSlotIndex];
			if (cacheEntry._valid == false || std::find(currentTiles.begin(), currentTiles.end(), cacheEntry._request) == currentTiles.end())
				continue;

            DK_ASSERT_LOG(cacheEntry._request._sectionId < world._sections.size() && cacheEntry._request._lod < constants._terrainOverviewLod && cacheSlotIndex / ClipMapTerrain::CACHE_SLOTS_PER_LOD == cacheEntry._request._lod && cacheEntry._fade >= 0.0f && cacheEntry._fade <= 1.0f, "Terrain tile cache entry must belong to its requested LOD and have a valid fade");
            const TerrainSection& section = world._sections[cacheEntry._request._sectionId];
			const uint32 sectionSlotIndexX = section._sectionIndexX % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
			const uint32 sectionSlotIndexY = section._sectionIndexZ % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
            const uint32 sectionSlotIndex = sectionSlotIndexY * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE + sectionSlotIndexX;
            DK_ASSERT_LOG(cacheEntry._request._tileLocalIndexX < (tableWidth >> cacheEntry._request._lod) && cacheEntry._request._tileLocalIndexZ < (tableWidth >> cacheEntry._request._lod), "Cached terrain tile indices must fit their section LOD grid");
            const uint32 pageTableIndexX = cacheEntry._request._tileLocalIndexX;
            const uint32 pageTableIndexY = cacheEntry._request._tileLocalIndexZ + sectionSlotIndex * sectionPageTableRowCount + terrainPageTableLodRowOffset(tableWidth, cacheEntry._request._lod);
            DK_ASSERT_LOG(pageEntries[size_t(pageTableIndexY) * tableWidth + pageTableIndexX].x == 0, "Terrain page table collision");
            pageEntries[size_t(pageTableIndexY) * tableWidth + pageTableIndexX] = { float(cacheSlotIndex + 1), cacheEntry._fade, float(cacheEntry._request._tileLocalIndexX), float(cacheEntry._request._tileLocalIndexZ) };
        }
        if (rm.queueTextureRegion(terrain._pageTable, 0, 0, tableWidth, tableHeight, pageEntries.data(), tableWidth * sizeof(pageEntries[0])) == false)
            return;

		// 새롭게 로딩이 필요한 Tile 로딩요청하기 위해 Page에 없는 Tile들 선별
        DKVector<TileRequest> missingTiles;
        for (const TileRequest& request : currentTiles)
        {
            const TerrainSection& section = world._sections[request._sectionId];
            const uint32 sectionSlotIndex = (section._sectionIndexZ % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE + section._sectionIndexX % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
			const uint32 pageTableSectionOffsetY = sectionSlotIndex * sectionPageTableRowCount;
            DK_ASSERT_LOG(request._lod < constants._terrainOverviewLod, "Requested terrain tile LOD must have a page table");
            const uint32 sectionLodTileCountPerSide = tableWidth >> request._lod;
            DK_ASSERT_LOG(request._tileLocalIndexX < sectionLodTileCountPerSide && request._tileLocalIndexZ < sectionLodTileCountPerSide, "Requested terrain tile indices must fit their section LOD grid");
            const uint32 pageTableIndexX = request._tileLocalIndexX;
            const uint32 pageTableIndexY = request._tileLocalIndexZ + pageTableSectionOffsetY + terrainPageTableLodRowOffset(tableWidth, request._lod);
            const float4& pageEntry = pageEntries[size_t(pageTableIndexY) * tableWidth + pageTableIndexX];
            DK_ASSERT_LOG(pageEntry.x == 0 || (pageEntry.z == float(request._tileLocalIndexX) && pageEntry.w == float(request._tileLocalIndexZ)), "Terrain page table entry must identify its section-local tile");
            if (pageEntry.x == 0)
                missingTiles.push_back(request);
        }

        terrain._streamer.request(currentSections, missingSections, missingTiles);
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
