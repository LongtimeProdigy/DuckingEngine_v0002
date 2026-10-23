#pragma once

#include "RenderModule.h"	// TODO: TextureResourceViewType때문에 있는데.. 분리하자

namespace DK
{
	class Material;
	struct IBuffer;

	template<typename T>
	class MaterialParameterTemplate;
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
			Mesh(IBufferRef&& vertexBuffer, IBufferRef&& indexBuffer, const VertexBufferViewRef& vertexBufferView, const IndexBufferViewRef& indexBufferView, const uint32 indexCount)
				: _vertexBuffer(DK::move(vertexBuffer))
				, _indexBuffer(DK::move(indexBuffer))
				, _vertexBufferView(vertexBufferView)
				, _indexBufferView(indexBufferView)
				, _indexCount(indexCount)
			{}

		public:
			IBufferRef _vertexBuffer;
			IBufferRef _indexBuffer;
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
			DKVector<IBufferRef> _terrainConstantBuffer;
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
