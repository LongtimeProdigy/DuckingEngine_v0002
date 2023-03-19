#pragma once

namespace DK
{
	class Material;
	struct IBuffer;

	const uint32 TILE_RESOLUTION = 4;
	const uint32 PATCH_VERT_RESOLUTION = TILE_RESOLUTION + 1;
	const uint32 CLIPMAP_RESOLUTION = TILE_RESOLUTION * 4 + 1;
	const uint32 CLIPMAP_VERT_RESOLUTION = CLIPMAP_RESOLUTION + 1;
	const uint32 NUM_CLIPMAP_LEVELS = 3;

	struct TerrainMeshConstantBuffer
	{
		float4 _base_scale;
		float4 _color;
		float4x4 _rotate;
	};

	class SceneManager
	{
	public:
		struct Terrain
		{
			Terrain(const VertexBufferViewRef& vertexBufferView, const IndexBufferViewRef& indexBufferView, const uint32 indexCount, Material* material)
				: _vertexBufferView(vertexBufferView)
				, _indexBufferView(indexBufferView)
				, _indexCount(indexCount)
				, _material(DK::move(material))
			{}

			VertexBufferViewRef _vertexBufferView;
			IndexBufferViewRef _indexBufferView;
			uint32 _indexCount;

			Ptr<Material> _material;
		};

		struct ClipMapTerrain
		{
			struct Mesh
			{
				Mesh(const VertexBufferViewRef& vertexBufferView, const IndexBufferViewRef& indexBufferView, const uint32 indexCount)
					: _vertexBufferView(vertexBufferView)
					, _indexBufferView(indexBufferView)
					, _indexCount(indexCount)
				{}

			public:
				VertexBufferViewRef _vertexBufferView;
				IndexBufferViewRef _indexBufferView;
				uint32 _indexCount;
			};

			ClipMapTerrain(const Mesh&& tile, const Mesh&& filter, const Mesh&& trim, const Mesh&& cross, const Mesh&& seam, Material* material)
				: _tile(DK::move(tile))
				, _filter(DK::move(filter))
				, _trim(DK::move(trim))
				, _cross(DK::move(cross))
				, _seam(DK::move(seam))
				, _material(material)
			{}

		public:
			Mesh _tile;
			Mesh _filter;
			Mesh _trim;
			Mesh _cross;
			Mesh _seam;

			Ptr<Material> _material;
			DKVector<Ptr<IBuffer>> _terrainConstantBuffer;
		};

	public:
		void loadLevel();

		const DKVector<Terrain>& getTerrainContainer() const { return _terrainContainer; }
		const DKVector<ClipMapTerrain>& getClipMapTerrainContainer() const { return _clipmapTerrainContainer; }
		DKVector<ClipMapTerrain>& getClipMapTerrainContainerWritable() { return _clipmapTerrainContainer; }

	private:
		DKVector<Terrain> _terrainContainer;
		DKVector<ClipMapTerrain> _clipmapTerrainContainer;
	};
}