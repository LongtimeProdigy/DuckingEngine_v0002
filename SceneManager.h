#pragma once

#include "Resource/Material/ShaderStructures.h"

#include "RenderModule.h"	// TODO: TextureResourceViewType때문에 있는데.. 분리하자
#include "TerrainData.h"
#include "TerrainStreaming.h"
#include "Resource/Material/GrassTiles.h"

namespace DK
{
	class Material;
	struct IBuffer;

	template<typename T> class MaterialParameterTemplate;
	using MaterialParameterTexture = MaterialParameterTemplate<ITextureRef>;

	class SceneManager
	{
	public:
		struct Mesh
		{
			Mesh()
			{}
			Mesh(IBufferRef&& vertexBuffer, IBufferRef&& indexBuffer, const VertexBufferViewRef& vertexBufferView, const IndexBufferViewRef& indexBufferView, const uint32 indexCount, const DKVector<float2>& vertices = {})
				: _vertexBuffer(DK::move(vertexBuffer))
				, _indexBuffer(DK::move(indexBuffer))
				, _vertexBufferView(vertexBufferView)
				, _indexBufferView(indexBufferView)
				, _indexCount(indexCount)
			{
				if (vertices.empty() == false)
				{
					_minimum = vertices.front();
					_maximum = vertices.front();
					for (const float2& vertex : vertices)
					{
						_minimum.x = Math::min(_minimum.x, vertex.x);
						_minimum.y = Math::min(_minimum.y, vertex.y);
						_maximum.x = Math::max(_maximum.x, vertex.x);
						_maximum.y = Math::max(_maximum.y, vertex.y);
					}
				}
			}

		public:
			IBufferRef _vertexBuffer;
			IBufferRef _indexBuffer;
			VertexBufferViewRef _vertexBufferView = nullptr;
			IndexBufferViewRef _indexBufferView = nullptr;
			uint32 _indexCount = 0;
			float2 _minimum = float2::Zero;
			float2 _maximum = float2::Zero;
		};

		struct PostProcess
		{
			Mesh _mesh;
		};
		struct Ocean
		{
			static constexpr const float OCEAN_LENGTH = 512; // or 512
			static constexpr const uint32 OCEAN_N = 512;	// or 512

			Mesh _mesh;

			float _heightScale = 750.f;

			IBufferRef _initialSpectrumConstantBuffer;
			ITextureRef _h0[kFrameCount];
			ITextureRef _ht[kFrameCount * 2]; // *2 for Ping-pong
			ITextureRef _height[kFrameCount];
			ITextureRef _normal[kFrameCount];

			uint32 _currentReadTextureIndex = 0;
		};
		struct SkyDome
		{
			Mesh _tile;
			Mesh _filter;
			Mesh _trim;
			Mesh _cross;
			Mesh _seam;
		};
		struct ClipMapTerrain
		{
            TerrainWorld _world;
            Streamer _streamer;

			struct CacheSlot
			{
				TileRequest _request;
				bool _valid = false;
				float _fade = 0;
			};
			TerrainStreamingConstants _streamingConstants;
			IBufferRef _streamingBuffer;
			ITextureRef _heightCache, _normalCache, _pageTable, _tailHeight, _tailNormal;
			DKVector<CacheSlot> _cacheSlots;
			static constexpr uint32 CACHE_SLOTS_PER_LOD = 72;

			static constexpr const uint32 TILE_RESOLUTION = 64;
			static constexpr const uint32 PATCH_VERT_RESOLUTION = TILE_RESOLUTION + 1;
			static constexpr const uint32 CLIPMAP_VERT_RESOLUTION = TILE_RESOLUTION * 4 + 1 + 1;
			static constexpr const uint32 NUM_CLIPMAP_LEVELS = 10;
			static constexpr const float BASE_VERTEX_SPACING = 0.5f;

			Mesh _tile;
			Mesh _filter;
			Mesh _trim;
			Mesh _cross;
			Mesh _seam;
			Ptr<Material> _material;
			DKVector<IBufferRef> _terrainConstantBuffer;
		};
		struct Grass
		{
			static constexpr uint32 GRASS_INDEX_COUNT = GRASS_BLADE_COUNT * GRASS_BLADE_INDEX_COUNT;

			uint32 _tileGridSide = 0;
			float _grassMapOriginX = 0;
			float _grassMapOriginZ = 0;
			float _grassMapWidth = 0;
			float _grassMapDepth = 0;
			ITextureRef _placementTexture;
			IBufferRef _indexBuffer;
			IndexBufferViewRef _indexBufferView;
		};
		struct GBuffer
		{
			Mesh _mesh;
		};

	public:
		void loadOcean();
		void loadSkyDome();
		bool loadLevel(const DKString& levelPath);
		void prepareTerrainStreaming(double cameraX, double cameraZ, float deltaTime);
		void loadPostProcess();
		void loadGbuffer();

		SkyDome& getSkyDomeWritable() { return _skyDome; }
		Ocean& getOceanWritable() { return _ocean; }
		ClipMapTerrain& getClipMapTerrainWritable() { return _clipmapTerrain; }
		const Grass& getGrass() const { return _grass; }
		PostProcess& getPostProcessWritable() { return _postProcess; }
		GBuffer& getGBufferWritable() { return _gBuffer; }

	private:
		bool loadTerrain(const DKString& terrainWorld, const DKString& modelPropertyPath, const float2& terrainWorldOrigin);
		bool loadGrass(const DKString& placementTexturePath, uint32 tileGridSide);
		bool initializeTerrainStreaming(const float2& terrainWorldOrigin);

	private:
		SkyDome _skyDome;
		Ocean _ocean;
		ClipMapTerrain _clipmapTerrain;
		Grass _grass;
		PostProcess _postProcess;
		GBuffer _gBuffer;
	};
}
