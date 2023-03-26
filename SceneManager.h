#pragma once

namespace DK
{
	class Material;
	struct IBuffer;

	struct TerrainMeshConstantBuffer
	{
		float4 _baseXY_scale_rotate;
		uint32 _type;
	};

	class SceneManager
	{
	public:
		static constexpr uint32 TILE_RESOLUTION = 32;
		static constexpr uint32 PATCH_VERT_RESOLUTION = TILE_RESOLUTION + 1;
		static constexpr uint32 CLIPMAP_RESOLUTION = TILE_RESOLUTION * 4 + 1;
		static constexpr uint32 CLIPMAP_VERT_RESOLUTION = CLIPMAP_RESOLUTION + 1;
		static constexpr uint32 NUM_CLIPMAP_LEVELS = 7;

	public:
		struct SkyDome
		{
			VertexBufferViewRef _vertexBufferView;
			IndexBufferViewRef _indexBufferView;
			uint32 _indexCount;
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
		void loadSkyDome();
		void loadLevel();

		SkyDome& getSkyDomeWritable() { return _skyDome; }
		DKVector<ClipMapTerrain>& getClipMapTerrainContainerWritable() { return _clipmapTerrainContainer; }

	private:
		SkyDome _skyDome;
		DKVector<ClipMapTerrain> _clipmapTerrainContainer;
	};
}