#pragma once

#include "RenderModule.h"	// TODO: TextureResourceViewType때문에 있는데.. 분리하자

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
			static constexpr const float OCEAN_LENGTH = 258; // or 516
			static constexpr const uint32 OCEAN_N = 258;	// or 512	

			Mesh _mesh;

			struct OceanParams
			{
				OceanParams(
					const float time, const float g, const uint32 stages, const float heightScale, const float2& windDir, const uint32 length, const float A, const float L, const uint32 N,
					const TextureResourceViewType h0SRV, const TextureResourceViewType h0UAV,
					const TextureResourceViewType htSRV, const TextureResourceViewType htUAV,
					const TextureResourceViewType heightSRV, const TextureResourceViewType heightUAV)
					: _time(time)
					, _g(g)
					, _stages(stages)
					, _heightScale(heightScale)
					, _windDir(windDir)
					, _length(length)
					, _A(A)
					, _L(L)
					, _N(N)
					, _h0SRV(h0SRV)
					, _h0UAV(h0UAV)
					, _htSRV(htSRV)
					, _htUAV(htUAV)
					, _heightSRV(heightSRV)
					, _heightUAV(heightUAV)
				{}

				const float _time;
				const float _g;
				const uint32 _stages;
				const float _heightScale;

				const float2 _windDir;
				const uint32 _length;
				const float _A;

				const float _L;
				const uint32 _N;
				const TextureResourceViewType _h0SRV;
				const TextureResourceViewType _htSRV;

				const TextureResourceViewType _heightSRV;
				const TextureResourceViewType _h0UAV;
				const TextureResourceViewType _htUAV;
				const TextureResourceViewType _heightUAV;
			};
			Ptr<IBuffer> _initialSpectrumConstantBuffer;
			ITextureRef _h0[RenderModule::kFrameCount];
			ITextureRef _ht[RenderModule::kFrameCount * 2]; // *2 for Ping-pong
			ITextureRef _height[RenderModule::kFrameCount];

			uint32 _currentReadTextureIndex = 0;
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
