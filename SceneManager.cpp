#include "stdafx.h"
#include "SceneManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

namespace DK
{
	void SceneManager::loadLevel()
	{
		/*
		* 128*128Unit == 1Sector
		* Vertex Distance == 1M
		*/
		const uint32 horizontalVertexCountPerSector = 64;
		const uint32 verticalVertexCountPerSector = 64;
		const float distancePerVertex = 1.0f; // 1m

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
		const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexArr.data(), sizeof(TerrainVertex), static_cast<uint32>(vertexArr.size()), vertexBufferView);
		if (vertexBufferSuccess == false)
			return;

		IndexBufferViewRef indexBufferView;
		const bool indexBufferSuccess = renderModule.createIndexBuffer(indexArr.data(), static_cast<uint32>(indexArr.size()), indexBufferView);
		if (indexBufferSuccess == false)
			return;

		Terrain newLevel(vertexBufferView, indexBufferView, static_cast<uint32>(indexArr.size()));
		_terrainContainer.push_back(DK::move(newLevel));
	}
}
