#pragma once

#include "RenderModule.h"	// TODO: TextureResourceViewType때문에 있는데.. 분리하자
#include "TerrainData.h"
#include "TerrainStreaming.h"

namespace DK
{
	class Material;
	struct IBuffer;

	template<typename T> class MaterialParameterTemplate;
	using MaterialParameterTexture = MaterialParameterTemplate<ITextureRef>;

	struct alignas(16) TerrainMeshConstantBuffer
	{
		// xy: 패치 원점의 월드 XZ
		// z : 현재 레벨의 정점 간격
		// w : 패치 회전 번호, 0~3
		float4 _baseXY_scale_rotate;

		// xy: 실제 카메라의 월드 XZ
		// z : 모핑 시작 거리
		// w : 모핑 완료 거리
		float4 _morphCenter_start_end;

		// xy: 부모 레벨의 스냅 원점
		// z : 부모 레벨의 정점 간격
		// w : 예약
		float4 _parentOrigin_spacing;

		uint32 _type;
		uint32 _level;
		uint32 _hasParent;
		uint32 _padding;
	};

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

			struct OceanParams
			{
				OceanParams(
					const float time, const float g, const float2& windDir, const float heightScale, const uint32 length, const float A, const float L, const uint32 N,
					const TextureResourceViewType h0SRV, const TextureResourceViewType h0UAV,
					const TextureResourceViewType htUAV,
					const TextureResourceViewType heightSRV, const TextureResourceViewType heightUAV,
					const TextureResourceViewType normalSRV, const TextureResourceViewType normalUAV
				)
					: _time(time)
					, _g(g)
					, _windDir(windDir)
					, _heightScale(heightScale)
					, _length(length)
					, _A(A)
					, _L(L)
					, _N(N)
					, _h0SRV(h0SRV)
					, _h0UAV(h0UAV)
					, _htUAV(htUAV)
					, _heightSRV(heightSRV)
					, _heightUAV(heightUAV)
					, _normalSRV(normalSRV)
					, _normalUAV(normalUAV)
				{}

				const float _time;
				const float _g;
				const float2 _windDir;

				const float _heightScale;
				const uint32 _length;
				const float _A;
				const float _L;

				const uint32 _N;
				const TextureResourceViewType _h0SRV;
				const TextureResourceViewType _h0UAV;
				const TextureResourceViewType _htUAV;

				const TextureResourceViewType _heightSRV;
				const TextureResourceViewType _heightUAV;
				const TextureResourceViewType _normalSRV;
				const TextureResourceViewType _normalUAV;
			};

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
            struct alignas(16) SectionConstants
            {
                uint32 _terrainSectionX = 0;
                uint32 _terrainSectionZ = 0;
                uint32 _terrainSectionCellCountX = 0;
                uint32 _terrainSectionCellCountZ = 0;
                uint32 _terrainSectionOverviewSampleCountX = 0;
                uint32 _terrainSectionOverviewSampleCountZ = 0;
                uint32 _terrainSectionResident = 0;
                uint32 _terrainSectionPadding = 0;
            };
            static_assert(sizeof(SectionConstants) == 32);
            struct alignas(16) StreamingConstants
            {
                uint32 _terrainStreamingEnabled = 0;
                uint32 _terrainHeightCacheSRV = 0;
                uint32 _terrainNormalCacheSRV = 0;
                uint32 _terrainPageTableSRV = 0;
                uint32 _terrainOverviewHeightSRV = 0;
                uint32 _terrainOverviewNormalSRV = 0;
                uint32 _terrainCacheSlotColumnCount = 8;
                uint32 _terrainCacheSlotRowCount = 0;
                uint32 _terrainCacheSlotTexelCountPerSide = 0;
                float _terrainSampleWorldSpacing = 1;
                float _terrainBaseWorldHeight = -16;
                float _terrainWorldHeightRange = 128;
                uint32 _terrainCellCountX = 0;
                uint32 _terrainCellCountZ = 0;
                uint32 _terrainChunkCellCountPerSide = 128;
                uint32 _terrainSectionCellCountPerSide = 0;
                uint32 _terrainOverviewLod = 3;
                uint32 _terrainOverviewSampleCountX = 0;
                uint32 _terrainOverviewSampleCountZ = 0;
                uint32 _terrainPageTableEntryCountPerSide = 16;
                float _terrainCameraAnchorRelativeWorldX = 0;
                float _terrainCameraAnchorRelativeWorldZ = 0;
                int32 _terrainAnchorSampleX = 0;
                int32 _terrainAnchorSampleZ = 0;
                float _terrainCameraWorldX = 0;
                float _terrainCameraWorldZ = 0;
                float _terrainLod0WorldRadius = 128.0f;
                float _terrainOriginWorldX = 0;
                float _terrainOriginWorldZ = 0;
                uint32 _terrainSectionCountX = 0;
                uint32 _terrainSectionCountZ = 0;
                uint32 _terrainOverviewSlotSampleCountPerSide = 0;
                DKArray<SectionConstants, 9> _terrainSections;
            };
            static_assert(sizeof(StreamingConstants) == 416);
            static_assert(offsetof(StreamingConstants, _terrainSections) == 128);
			struct CacheSlot
			{
				TileRequest _request;
				bool _valid = false;
				float _fade = 0;
			};
			StreamingConstants _streamingConstants;
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
			static constexpr uint32 GRASS_BLADE_COUNT = 6;
			static constexpr uint32 GRASS_INDEX_COUNT = GRASS_BLADE_COUNT * 15;

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
