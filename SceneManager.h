#pragma once

namespace DK
{
	class SceneManager
	{
	public:
		struct Terrain
		{
			Terrain(const VertexBufferViewRef& vertexBufferView, const IndexBufferViewRef& indexBufferView, const uint32 indexCount)
				: _vertexBufferView(vertexBufferView)
				, _indexBufferView(indexBufferView)
				, _indexCount(indexCount)
			{}

			VertexBufferViewRef _vertexBufferView;
			IndexBufferViewRef _indexBufferView;
			uint32 _indexCount;
		};

	public:
		void loadLevel();

		const DKVector<Terrain>& getTerrainContainer() const { return _terrainContainer; }

	private:
		DKVector<Terrain> _terrainContainer;
	};
}