#pragma once

namespace DK
{
	class Material;
	struct IBuffer;

	template<typename T>
	class MaterialParameterTemplate;
	using MaterialParameterTexture = MaterialParameterTemplate<ITextureRef>;

	struct TerrainMeshConstantBuffer
	{
		float4 _baseXY_scale_rotate;
		uint32 _type;
	};

	class SceneManager
	{
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
			VertexBufferViewRef _vertexBufferView = nullptr;
			IndexBufferViewRef _indexBufferView = nullptr;
			uint32 _indexCount = 0;
		};

		struct PostProcess
		{
			Mesh _mesh;
		};
		struct Ocean
		{
			static constexpr const uint32 TILE_RESOLUTION = 32;
			static_assert((TILE_RESOLUTION % 8) == 0, "Ocean TIleResolution must be mul by 8");

			static constexpr const float OCEAN_LENGTH = 256; // or 516
			static constexpr const uint32 OCEAN_N = 256;	// or 512	

			Mesh _mesh;

			struct OceanParams
			{
				OceanParams(const float2& windDir, const float A, const float L, const uint32 N)
					: _windDir(windDir)
					, _A(A)
					, _L(L)
					, _N(N)
				{}

				float2 _windDir;
				float _A;
				float _L;
				uint32 _N;
			};
			Ptr<IBuffer> _initialSpectrumConstantBuffer;
			ITextureRef _h0;
		};
		struct SkyDome
		{
			Mesh _mesh;
		};
		struct ClipMapTerrain
		{
			static const float2 TILE_SCALE;
			static constexpr const uint32 TILE_RESOLUTION = 32;
			static constexpr const uint32 PATCH_VERT_RESOLUTION = TILE_RESOLUTION + 1;
			static constexpr const uint32 CLIPMAP_VERT_RESOLUTION = TILE_RESOLUTION * 4 + 1 + 1;
			static constexpr const uint32 NUM_CLIPMAP_LEVELS = 8;

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
		void loadOcean();
		void loadSkyDome();
		void loadLevel();
		void loadPostProcess();
		void loadGbuffer();

		SkyDome& getSkyDomeWritable() { return _skyDome; }
		Ocean& getOceanWritable() { return _ocean; }
		ClipMapTerrain& getClipMapTerrainWritable() { return _clipmapTerrain; }
		PostProcess& getPostProcessWritable() { return _postProcess; }
		GBuffer& getGBufferWritable() { return _gBuffer; }

	private:
		SkyDome _skyDome;
		Ocean _ocean;
		ClipMapTerrain _clipmapTerrain;
		PostProcess _postProcess;
		GBuffer _gBuffer;
	};
}
