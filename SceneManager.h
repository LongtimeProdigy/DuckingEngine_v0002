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
		struct Mesh
		{
			Mesh()
			{}
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

		struct PostProcess
		{
			Mesh _mesh;
		};
		struct SkyDome
		{
			Mesh _mesh;
		};
		struct ClipMapTerrain
		{
			Mesh _tile;
			Mesh _filter;
			Mesh _trim;
			Mesh _cross;
			Mesh _seam;

			Ptr<Material> _material;
			DKVector<Ptr<IBuffer>> _terrainConstantBuffer;
		};
		struct GBuffer
		{
			Mesh _mesh;
		};

	public:
		void loadSkyDome();
		void loadLevel();
		void loadPostProcess();
		void loadGbuffer();

		SkyDome& getSkyDomeWritable() { return _skyDome; }
		ClipMapTerrain& getClipMapTerrainWritable() { return _clipmapTerrain; }
		PostProcess& getPostProcessWritable() { return _postProcess; }
		GBuffer& getGBufferWritable() { return _gBuffer; }

	private:
		SkyDome _skyDome;
		ClipMapTerrain _clipmapTerrain;
		PostProcess _postProcess;
		GBuffer _gBuffer;
	};
}