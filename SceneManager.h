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
			Archive _chunkArchive;
			Streamer _streamer;
			struct alignas(16) StreamingConstants
			{
				uint32 _terrainStreamEnabled = 0;		// 청크 스트리밍 방식의 Terrain을 사용할지 여부. 0: 비활성, 1: 활성
				uint32 _terrainHeightCache = 0;			// 상세 높이 캐시 텍스처의 SRV 인덱스
				uint32 _terrainNormalCache = 0;			// 상세 법선 캐시 텍스처의 SRV 인덱스
				uint32 _terrainPageTable = 0;			// 청크와 캐시 슬롯을 연결하는 페이지 테이블의 SRV 인덱스
				uint32 _terrainTailHeight = 0;			// 전체 지형의 상주 저해상도 높이 텍스처 SRV 인덱스
				uint32 _terrainTailNormal = 0;			// 전체 지형의 상주 저해상도 법선 텍스처 SRV 인덱스
				uint32 _terrainCacheColumns = 8;		// 물리 atlas에서 가로로 배치하는 슬롯 수
				uint32 _terrainCacheRows = 0;			// 물리 atlas에서 세로로 배치하는 슬롯 수
				uint32 _terrainCacheSide = 0;			// 캐시 슬롯 한 변의 텍셀 수
				float _terrainSampleSpacing = 1;		// LOD 0 높이 샘플 사이의 월드 거리
				float _terrainHeightOffset = -16;		// 정규화 높이가 0일 때의 월드 Y
				float _terrainHeightScale = 128;		// 정규화 높이 0~1에 대응하는 높이 범위
				uint32 _terrainCellsX = 0;				// 전체 지형 X 방향 셀 수
				uint32 _terrainCellsZ = 0;				// 전체 지형 Z 방향 셀 수
				uint32 _terrainChunkCells = 128;		// 디스크 청크 한 변의 셀 수. GPU 가상 타일도 한 변에 이 개수의 셀을 가짐
				uint32 _terrainTilesX = 0;				// LOD 0 가상 타일의 X 방향 개수
				uint32 _terrainTilesZ = 0;				// LOD 0 가상 타일의 Z 방향 개수
				uint32 _terrainTailLod = 3;				// 전체 지형에 상주시킬 저해상도 LOD. 이보다 작은 LOD만 상세 스트리밍
				uint32 _terrainTailWidth = 0;			// 상주 저해상도 텍스처의 가로 샘플 수
				uint32 _terrainTailHeightPixels = 0;	// 상주 저해상도 텍스처의 세로 샘플 수
				uint32 _terrainPageTableSide = 8;		// LOD 하나가 사용하는 페이지 테이블의 한 변
				float _terrainCameraX = 0;				// 기준점에서 본 카메라의 상대 X 위치
				float _terrainCameraZ = 0;				// 기준점에서 본 카메라의 상대 Z 위치
				int32_t _terrainAnchorX = 0;			// 카메라 근처 렌더링 기준점의 X 샘플 좌표
				int32_t _terrainAnchorZ = 0;			// 카메라 근처 렌더링 기준점의 Z 샘플 좌표
				float _terrainCameraWorldX = 0;			// 기존 float 뷰 좌표와 연결하기 위한 카메라의 절대 X 위치 복사본
				float _terrainCameraWorldZ = 0;			// 기존 float 뷰 좌표와 연결하기 위한 카메라의 절대 Z 위치 복사본
				float _terrainDetailRadius = 128;		// 거리 기반 높이 LOD의 기준 거리. 현재 128 × SampleSpacing
				int32_t _terrainGrassAnchorX = 0;		// Terrain 기준점 근처에 있는 Grass 격자의 정수 X 셀 번호
				int32_t _terrainGrassAnchorZ = 0;		// Terrain 기준점 근처에 있는 Grass 격자의 정수 Z 셀 번호
				float _terrainGrassOffsetX = 0;			// 해당 Grass 셀 시작점에서 Terrain 기준점까지의 X 거리
				float _terrainGrassOffsetZ = 0;			// 해당 Grass 셀 시작점에서 Terrain 기준점까지의 Z 거리
			};
			static_assert(sizeof(StreamingConstants) == 128);
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
			static constexpr uint32 CACHE_SLOTS_PER_LOD = 16;

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

			static constexpr uint32 GRASS_BLADE_COUNT = 6;
			static constexpr uint32 GRASS_INDEX_COUNT = GRASS_BLADE_COUNT * 15;

			IBufferRef _grassIndexBuffer;
			IndexBufferViewRef _grassIndexBufferView;
		};
		struct GBuffer
		{
			Mesh _mesh;
		};

	public:
		void loadOcean();
		void loadSkyDome();
		void loadLevel(const DKString& terrainArchive = "Terrain/Cooked/forest_valley.dkterrain");
		void prepareTerrainStreaming(double cameraX, double cameraZ, float deltaTime);
		void loadPostProcess();
		void loadGbuffer();

		SkyDome& getSkyDomeWritable() { return _skyDome; }
		Ocean& getOceanWritable() { return _ocean; }
		ClipMapTerrain& getClipMapTerrainWritable() { return _clipmapTerrain; }
		PostProcess& getPostProcessWritable() { return _postProcess; }
		GBuffer& getGBufferWritable() { return _gBuffer; }

	private:
		void initializeTerrainStreaming();

	private:
		SkyDome _skyDome;
		Ocean _ocean;
		ClipMapTerrain _clipmapTerrain;
		PostProcess _postProcess;
		GBuffer _gBuffer;
	};
}
