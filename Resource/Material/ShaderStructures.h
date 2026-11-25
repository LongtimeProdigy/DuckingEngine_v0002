#ifndef DK_SHADER_STRUCTURES_CPP
#ifdef __cplusplus
#define DK_SHADER_STRUCTURES_CPP
#endif

// HLSL은 필요한 데이터 그룹을 선택해 include하므로 구조체별로 중복 선언을 막는다.
#ifndef DK_SHADER_STRUCTURE_TYPES
#define DK_SHADER_STRUCTURE_TYPES
#include "SkinningLimits.h"
#include "TerrainShared.h"
#include "Atmosphere/AtmosphereLimits.h"
#ifdef __cplusplus
#include "../../MathTypes.h"
#include "../../Ref.h"
#define DK_SHADER_UINT uint32
#define DK_SHADER_INT int32
#define DK_SHADER_CONST const
#define DK_SHADER_DEFAULT(value) = value
#define DK_SHADER_DEFAULT2(x, y) = { x, y }
#define DK_SHADER_DEFAULT3(x, y, z) = { x, y, z }
#define DK_SHADER_DEFAULT4(x, y, z, w) = { x, y, z, w }
#define DK_SHADER_SEMANTIC(name)
#define DK_SHADER_ALIGN16 alignas(16)
#define DK_SHADER_CBUFFER(name, slot) struct name
#define DK_SHADER_ARRAY(type, name, count) DKArray<type, count> name
#define DK_SHADER_UINT2(name) uint32 name[2]
#define DK_SHADER_UINT3(name) uint32 name[3]
#define DK_SHADER_UINT4(name) uint32 name[4]
#define DK_SHADER_FLOAT2(name) float name[2]
#define DK_SHADER_FLOAT3(name) float name[3]
#define DK_SHADER_FLOAT4(name) float name[4]
#else
#define DK_SHADER_UINT uint
#define DK_SHADER_INT int
#define DK_SHADER_CONST
#define DK_SHADER_DEFAULT(value)
#define DK_SHADER_DEFAULT2(x, y)
#define DK_SHADER_DEFAULT3(x, y, z)
#define DK_SHADER_DEFAULT4(x, y, z, w)
#define DK_SHADER_SEMANTIC(name) : name
#define DK_SHADER_ALIGN16
#define DK_SHADER_CBUFFER(name, slot) cbuffer name : register(slot)
#define DK_SHADER_ARRAY(type, name, count) type name[count]
#define DK_SHADER_UINT2(name) uint2 name
#define DK_SHADER_UINT3(name) uint3 name
#define DK_SHADER_UINT4(name) uint4 name
#define DK_SHADER_FLOAT2(name) float2 name
#define DK_SHADER_FLOAT3(name) float3 name
#define DK_SHADER_FLOAT4(name) float4 name
#endif
#endif

#ifdef __cplusplus
namespace DK
{
#endif

#if defined(__cplusplus) || defined(DK_SHADER_SCENE)
#ifndef DK_SHARED_SCENE_CONSTANT_BUFFER
#define DK_SHARED_SCENE_CONSTANT_BUFFER
DK_SHADER_CBUFFER(SceneConstantBuffer, b0)
{
    DK_SHADER_UINT _frameIndex;
    DK_SHADER_UINT2(_resolution);
    float _time;
    float _nearDistance;
    DK_SHADER_UINT _rngState;
    DK_SHADER_UINT2(_paddingSCB) DK_SHADER_DEFAULT({});
    float4x4 _cameraWorldMatrix;
    float4x4 _cameraWorldMatrixInv;
    float4x4 _cameraProjectionMatrix;
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_ATMOSPHERE)
#ifndef DK_SHARED_ATMOSPHERE_ABSORPTION_LAYER
#define DK_SHARED_ATMOSPHERE_ABSORPTION_LAYER
struct AtmosphereAbsorptionLayer
{
    DK_SHADER_FLOAT3(_extinction) DK_SHADER_DEFAULT3(0, 0, 0); // RGB coefficient at unit layer density, in inverse metres.
    float _strength DK_SHADER_DEFAULT(0);
    float _peakRadius DK_SHADER_DEFAULT(0); // Absolute distance from the planet centre; independent of terrain.
    float _lowerWidth DK_SHADER_DEFAULT(10000);
    float _upperWidth DK_SHADER_DEFAULT(10000);
    float _reserved DK_SHADER_DEFAULT(0);

#if defined(__cplusplus) && defined(_DK_DEBUG_)
    void validate([[maybe_unused]] uint32 layerIndex) const;
#endif
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_ATMOSPHERE)
#ifndef DK_SHARED_SURFACE_SHADOW_CASCADE
#define DK_SHARED_SURFACE_SHADOW_CASCADE
struct SurfaceShadowCascade
{
    DK_SHADER_FLOAT4(_right) DK_SHADER_DEFAULT({});
    DK_SHADER_FLOAT4(_up) DK_SHADER_DEFAULT({});
    DK_SHADER_FLOAT4(_forward) DK_SHADER_DEFAULT({});
    DK_SHADER_FLOAT4(_origin) DK_SHADER_DEFAULT({});
    DK_SHADER_UINT _albedoIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _depthIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _mapSize DK_SHADER_DEFAULT(512);
    DK_SHADER_UINT _padding DK_SHADER_DEFAULT(0);
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_ATMOSPHERE)
#ifndef DK_SHARED_ATMOSPHERE_CONSTANT_BUFFER
#define DK_SHARED_ATMOSPHERE_CONSTANT_BUFFER
#ifdef __cplusplus
    // 지구 대기 프리셋의 초기 반경이며 런타임 환경 설정으로 변경할 수 있다.
    static constexpr float kEarthAtmosphereRadius = 6390000.0f;
#endif
DK_SHADER_CBUFFER(AtmosphereConstantBuffer, b1)
{
    DK_SHADER_UINT _numInScatteringPoints DK_SHADER_DEFAULT(8);
    DK_SHADER_UINT _opticalDepthPointCount DK_SHADER_DEFAULT(16);
    float _rayleighHeight DK_SHADER_DEFAULT(7994.0f);
    float _mieHeight DK_SHADER_DEFAULT(1200.0f);
    float _sunDegree DK_SHADER_DEFAULT(90.f);
    float _sunIrradiance DK_SHADER_DEFAULT(20.0f);
    float _scatteringScale DK_SHADER_DEFAULT(1.0f);
    float _cameraAltitude DK_SHADER_DEFAULT(0.0f);
    float _densityReferenceRadius DK_SHADER_DEFAULT(6360000.0f);
    DK_SHADER_FLOAT3(_planetCentre) DK_SHADER_DEFAULT3(0, -6360000.0f, 0);
    float _atmosphereRadius DK_SHADER_DEFAULT(kEarthAtmosphereRadius);
    float _sunAngularRadius DK_SHADER_DEFAULT(0.27f);
    float _exposureEV DK_SHADER_DEFAULT(0.0f);
    float _bloomStrength DK_SHADER_DEFAULT(0.02f);
    float _bloomRadius DK_SHADER_DEFAULT(16.0f);
    float _bloomThreshold DK_SHADER_DEFAULT(2.0f);
    float _sunRadianceScale DK_SHADER_DEFAULT(1.0f);
    float _densityTransitionRadius DK_SHADER_DEFAULT(6358000.0f); // Inner approximation boundary, in metres from centre.
    float _integrationRelativeTolerance DK_SHADER_DEFAULT(0.002f);
    DK_SHADER_UINT _integrationMaxDepth DK_SHADER_DEFAULT(8);
    DK_SHADER_UINT _atmosphereSamplingMode DK_SHADER_DEFAULT(ATMOSPHERE_PRODUCTION);
    DK_SHADER_UINT _transmittanceLutIndex DK_SHADER_DEFAULT(0);
    float _rayleighScale DK_SHADER_DEFAULT(1.0f);
    float _mieScale DK_SHADER_DEFAULT(1.0f);
    float _mieAbsorptionScale DK_SHADER_DEFAULT(0.0f);
    float _mieAnisotropy DK_SHADER_DEFAULT(0.76f);
    DK_SHADER_FLOAT3(_sunColor) DK_SHADER_DEFAULT3(1.0f, 1.0f, 1.0f); // Linear RGB chromaticity; normalized to unit luminance in the helper.
    float _skyLightingStrength DK_SHADER_DEFAULT(1.0f);
    DK_SHADER_UINT _skyRadianceIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _skyFilteredIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _viewNodeBudget DK_SHADER_DEFAULT(256);
    DK_SHADER_UINT _opticalNodeBudget DK_SHADER_DEFAULT(128);
    AtmosphereAbsorptionLayer _absorptionLayers[DK_ATMOSPHERE_ABSORPTION_LAYER_COUNT];
    DK_SHADER_FLOAT4(_surfaceRight) DK_SHADER_DEFAULT4(1, 0, 0, 2000); // xyz: world basis, w: half-width in metres.
    DK_SHADER_FLOAT4(_surfaceUp) DK_SHADER_DEFAULT4(0, 0, 1, 2000);
    DK_SHADER_FLOAT4(_surfaceForward) DK_SHADER_DEFAULT4(0, -1, 0, 8000); // w: total capture depth.
    DK_SHADER_FLOAT4(_surfaceOrigin) DK_SHADER_DEFAULT4(0, 4000, 0, 0); // Light-view origin relative to the main camera.
    DK_SHADER_UINT _surfaceAlbedoIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _surfaceNormalIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _surfaceMaterialIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _surfaceDepthIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _surfaceBounceIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _surfaceMapEnabled DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _surfaceMapSize DK_SHADER_DEFAULT(512);
    float _surfaceShadowStrength DK_SHADER_DEFAULT(1.0f);
    float _surfaceBounceStrength DK_SHADER_DEFAULT(1.0f);
    float _surfaceBias DK_SHADER_DEFAULT(1.0f);
    DK_SHADER_UINT _surfaceBounceSamples DK_SHADER_DEFAULT(32);
    float _surfacePadding DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _multipleScatteringLutIndex DK_SHADER_DEFAULT(0);
    float _multipleScatteringStrength DK_SHADER_DEFAULT(1.0f);
    DK_SHADER_UINT _multipleScatteringMaxOrder DK_SHADER_DEFAULT(4);
    float _multipleScatteringPadding DK_SHADER_DEFAULT(0);
    SurfaceShadowCascade _shadowCascades[DK_ATMOSPHERE_SHADOW_CASCADE_COUNT];
    DK_SHADER_UINT _shadowCascadeCount DK_SHADER_DEFAULT(DK_ATMOSPHERE_SHADOW_CASCADE_COUNT);
    float _shadowCascadeBlend DK_SHADER_DEFAULT(0.1f);
    DK_SHADER_FLOAT2(_shadowCascadePadding) DK_SHADER_DEFAULT({});
    // Eight intervals give one preview evaluation per production seed, without the refinement evaluations.
    DK_SHADER_UINT _previewViewIntervalCount DK_SHADER_DEFAULT(8u);
    DK_SHADER_UINT _previewAtmosphereShadows DK_SHADER_DEFAULT(0u);
    DK_SHADER_UINT _previewSurfaceBounce DK_SHADER_DEFAULT(0u);
    DK_SHADER_UINT _previewPadding DK_SHADER_DEFAULT(0u);

#if defined(__cplusplus) && defined(_DK_DEBUG_)
    void validate() const;
#endif
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_CLOUD)
#ifndef DK_SHARED_CLOUD_CONSTANT_BUFFER
#define DK_SHARED_CLOUD_CONSTANT_BUFFER
DK_SHADER_CBUFFER(CloudConstantBuffer, b3)
{
    // Terrestrial low-cloud preset. Heights are metres above the atmosphere density reference radius.
    float _cloudBaseHeight DK_SHADER_DEFAULT(1200.0f);
    float _cloudTopHeight DK_SHADER_DEFAULT(3000.0f);
    float _cloudCoverage DK_SHADER_DEFAULT(0.55f);
    float _cloudExtinction DK_SHADER_DEFAULT(0.028f); // Per metre at unit cloud density.
    // Regional weather, kilometre-scale puffs and smaller edge erosion repeat at separate scales.
    float _cloudWeatherPeriod DK_SHADER_DEFAULT(64000.0f);
    float _cloudShapePeriod DK_SHADER_DEFAULT(4200.0f);
    float _cloudDetailPeriod DK_SHADER_DEFAULT(560.0f);
    DK_SHADER_UINT _cloudWeatherIndex DK_SHADER_DEFAULT(0);
    DK_SHADER_FLOAT2(_cloudWindSpeedXZ) DK_SHADER_DEFAULT2(8.0f, 3.0f); // Metres per second.
    DK_SHADER_UINT _cloudNoiseIndex DK_SHADER_DEFAULT(0);
    float _cloudScatteringAlbedo DK_SHADER_DEFAULT(0.999f); // Visible-light scattering dominates droplet absorption.

#if defined(__cplusplus) && defined(_DK_DEBUG_)
    void validate() const;
#endif
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_TERRAIN_MESH)
#ifndef DK_SHARED_TERRAIN_MESH_CONSTANT_BUFFER
#define DK_SHARED_TERRAIN_MESH_CONSTANT_BUFFER
#ifdef __cplusplus
struct alignas(16) TerrainMeshConstantBuffer
#else
cbuffer TerrainMeshConstantBuffer : register(b2)
#endif
{
    float4 _baseXY_scale_rotate; // xy: Anchor 상대 패치 원점 XZ, z: 현재 정점 간격, w: 회전 번호 0~3
    float4 _morphCenter_start_end; // xy: Anchor 상대 카메라 XZ, z/w: 모핑 시작/종료 거리
    float2 _parentOriginRelativeWorldXZ; // Anchor 상대 부모 레벨 스냅 원점 XZ
    DK_SHADER_UINT _type;
    DK_SHADER_UINT _level;
    DK_SHADER_UINT _hasParent;
    DK_SHADER_UINT3(_padding);
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_OCEAN)
#ifndef DK_SHARED_OCEAN_PARAMS
#define DK_SHARED_OCEAN_PARAMS
DK_SHADER_CBUFFER(OceanParams, b1)
{
    DK_SHADER_CONST float _timeForOcean;
    DK_SHADER_CONST float _g;
    DK_SHADER_CONST float2 _windDir;
    DK_SHADER_CONST float _heightScale;
    DK_SHADER_CONST DK_SHADER_UINT _length;
    DK_SHADER_CONST float _A;
    DK_SHADER_CONST float _L;
    DK_SHADER_CONST DK_SHADER_UINT _N;
    DK_SHADER_CONST DK_SHADER_UINT _h0SRV;
    DK_SHADER_CONST DK_SHADER_UINT _h0UAV;
    DK_SHADER_CONST DK_SHADER_UINT _htUAV;
    DK_SHADER_CONST DK_SHADER_UINT _heightSRV;
    DK_SHADER_CONST DK_SHADER_UINT _heightUAV;
    DK_SHADER_CONST DK_SHADER_UINT _normalSRV;
    DK_SHADER_CONST DK_SHADER_UINT _normalUAV;

#ifdef __cplusplus
    OceanParams(const float time, const float g, const float2& windDir, const float heightScale, const uint32 length, const float A, const float L, const uint32 N, const uint32 h0SRV, const uint32 h0UAV, const uint32 htUAV, const uint32 heightSRV, const uint32 heightUAV, const uint32 normalSRV, const uint32 normalUAV)
        : _timeForOcean(time)
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
#endif
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_TERRAIN_STREAMING)
#ifndef DK_SHARED_TERRAIN_SECTION_CONSTANTS
#define DK_SHARED_TERRAIN_SECTION_CONSTANTS
struct DK_SHADER_ALIGN16 TerrainSectionConstants
{
    DK_SHADER_UINT _terrainSectionIndexX DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainSectionIndexZ DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainSectionResident DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainSectionPadding DK_SHADER_DEFAULT(0);
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_TERRAIN_STREAMING)
#ifndef DK_SHARED_TERRAIN_STREAMING_CONSTANTS
#define DK_SHARED_TERRAIN_STREAMING_CONSTANTS
#ifdef __cplusplus
struct alignas(16) TerrainStreamingConstants
#else
cbuffer TerrainStreamingConstants : register(b3)
#endif
{
    DK_SHADER_UINT _terrainStreamingEnabled DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainHeightCacheSRV DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainNormalCacheSRV DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainPageTableSRV DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainOverviewHeightAtlasSRV DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainOverviewNormalAtlasSRV DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainCacheSlotColumnCount DK_SHADER_DEFAULT(8);
    DK_SHADER_UINT _terrainCacheSlotRowCount DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainCacheSlotTexelCountPerSide DK_SHADER_DEFAULT(0);
    float _terrainSampleWorldSpacing DK_SHADER_DEFAULT(0.5f);
    float _terrainBaseWorldHeight DK_SHADER_DEFAULT(-16);
    float _terrainWorldHeightRange DK_SHADER_DEFAULT(128);
    DK_SHADER_UINT _terrainCellCountX DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainCellCountZ DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainChunkCellCountPerSide DK_SHADER_DEFAULT(128);
    DK_SHADER_UINT _terrainSectionCellCountPerSide DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainOverviewLod DK_SHADER_DEFAULT(3);
    DK_SHADER_UINT _terrainOverviewAtlasTexelCountX DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainOverviewAtlasTexelCountY DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainPageTableWidth DK_SHADER_DEFAULT(0);
    float _terrainCameraLocalX DK_SHADER_DEFAULT(0);
    float _terrainCameraLocalZ DK_SHADER_DEFAULT(0);
    DK_SHADER_INT _terrainCameraAnchorSampleX DK_SHADER_DEFAULT(0);
    DK_SHADER_INT _terrainCameraAnchorSampleZ DK_SHADER_DEFAULT(0);
    float _terrainCameraWorldX DK_SHADER_DEFAULT(0);
    float _terrainCameraWorldZ DK_SHADER_DEFAULT(0);
    float _terrainLod0WorldRadius DK_SHADER_DEFAULT(128.0f);
    float _terrainOriginWorldX DK_SHADER_DEFAULT(0);
    float _terrainOriginWorldZ DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainSectionCountX DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainSectionCountZ DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _terrainSectionOverviewSampleCountPerSide DK_SHADER_DEFAULT(0);
    DK_SHADER_INT _terrainMinimumSampleX DK_SHADER_DEFAULT(0);
    DK_SHADER_INT _terrainMinimumSampleZ DK_SHADER_DEFAULT(0);
    DK_SHADER_ARRAY(TerrainSectionConstants, _terrainSections, TERRAIN_SECTION_SLOT_COUNT);
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_OBJECT)
#ifndef DK_SHARED_SCENE_OBJECT_CONSTANT_BUFFER
#define DK_SHARED_SCENE_OBJECT_CONSTANT_BUFFER
DK_SHADER_CBUFFER(SceneObjectConstantBuffer, b1)
{
    float4x4 _worldMatrix;
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_INSTANCE)
#ifndef DK_SHARED_STATIC_MESH_INSTANCE
#define DK_SHARED_STATIC_MESH_INSTANCE
struct StaticMeshInstance
{
    float4x4 _worldMatrix;
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_STATIC_VERTEX)
#ifndef DK_SHARED_STATIC_MESH_VERTEX
#define DK_SHARED_STATIC_MESH_VERTEX
struct StaticMeshVertex
{
    DK_SHADER_CONST float3 _position DK_SHADER_SEMANTIC(POSITION) DK_SHADER_DEFAULT(float3::Zero);
    DK_SHADER_CONST float3 _normal DK_SHADER_SEMANTIC(NORMAL) DK_SHADER_DEFAULT(float3::Zero);
    DK_SHADER_CONST float2 _uv DK_SHADER_SEMANTIC(TEXCOORD0) DK_SHADER_DEFAULT(float2::Zero);
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_SKINNED_VERTEX)
#ifndef DK_SHARED_SKINNED_MESH_VERTEX
#define DK_SHARED_SKINNED_MESH_VERTEX
struct SkinnedMeshVertex
{
    DK_SHADER_CONST float3 _position DK_SHADER_SEMANTIC(POSITION) DK_SHADER_DEFAULT(float3::Zero);
    DK_SHADER_CONST float3 _normal DK_SHADER_SEMANTIC(NORMAL) DK_SHADER_DEFAULT(float3::Zero);
    DK_SHADER_CONST float2 _uv DK_SHADER_SEMANTIC(TEXCOORD0) DK_SHADER_DEFAULT(float2::Zero);
    DK_SHADER_CONST DK_SHADER_UINT4(_boneIndices) DK_SHADER_SEMANTIC(BONEINDEXES) DK_SHADER_DEFAULT4(static_cast<uint32>(-1), static_cast<uint32>(-1), static_cast<uint32>(-1), static_cast<uint32>(-1));
    DK_SHADER_CONST DK_SHADER_FLOAT4(_boneWeights) DK_SHADER_SEMANTIC(BONEWEIGHTS) DK_SHADER_DEFAULT4(0.0f, 0.0f, 0.0f, 0.0f);
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_DEBUG_DRAW)
#ifndef DK_SHARED_SPHERE_PRIMITIVE_INFO
#define DK_SHARED_SPHERE_PRIMITIVE_INFO
struct SpherePrimitiveInfo
{
    float3 _worldPosition;
    float _radius DK_SHADER_DEFAULT(1.0f);
    float3 _color;
    float _padding DK_SHADER_DEFAULT(1.0f);

#ifdef __cplusplus
    static IBufferRef kVertexBuffer;
    static IBufferRef kIndexBuffer;
    static VertexBufferViewRef kVertexBufferView;
    static IndexBufferViewRef kIndexBufferView;
    static uint32 indexCount;

    SpherePrimitiveInfo(const float3& worldPosition, const float3& color, const float& radius)
        : _worldPosition(worldPosition)
        , _color(color)
        , _radius(radius)
        {}
#endif
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_DEBUG_DRAW)
#ifndef DK_SHARED_LINE_PRIMITIVE_INFO
#define DK_SHARED_LINE_PRIMITIVE_INFO
struct LinePrimitiveInfo
{
    float3 _startWorldPosition;
    float3 _endWorldPosition;
    float3 _color;

#ifdef __cplusplus
    static IBufferRef kVertexBuffer;
    static IBufferRef kIndexBuffer;
    static VertexBufferViewRef kVertexBufferView;
    static IndexBufferViewRef kIndexBufferView;
    static uint32 indexCount;

    LinePrimitiveInfo(const float3& startWorldPosition, const float3& endWorldPosition, const float3& color)
        : _startWorldPosition(startWorldPosition)
        , _endWorldPosition(endWorldPosition)
        , _color(color)
        {}
#endif
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_SKELETON)
#ifndef DK_SHARED_SKELETON_CONSTANT_BUFFER
#define DK_SHARED_SKELETON_CONSTANT_BUFFER
DK_SHADER_CBUFFER(SkeletonConstantBuffer, b3)
{
    DK_SHADER_ARRAY(float4x4, _currentCharacterSpaceBoneAnimation, DK_MAX_BONE_COUNT);
};
#endif
#endif

#if defined(__cplusplus) || defined(DK_SHADER_GPU_ASSERT)
#ifndef DK_SHARED_GPU_ASSERT_RECORD
#define DK_SHARED_GPU_ASSERT_RECORD
// 상태 uint와 진단 헤더 uint3 뒤에 uint4 값을 저장하는 ByteAddressBuffer 형식이다.
#define DK_GPU_ASSERT_HEADER_OFFSET 4u
#define DK_GPU_ASSERT_DATA_OFFSET 16u
#define DK_GPU_ASSERT_UINT4 0u
#define DK_GPU_ASSERT_FLOAT4 1u
#ifdef __cplusplus
enum class GpuAssertDataType : uint32 { Uint4 = DK_GPU_ASSERT_UINT4, Float4 = DK_GPU_ASSERT_FLOAT4 };
#endif
struct GpuAssertRecord
{
    DK_SHADER_UINT _failed DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _fileId DK_SHADER_DEFAULT(0);
    DK_SHADER_UINT _sourceLine DK_SHADER_DEFAULT(0);
#ifdef __cplusplus
    GpuAssertDataType _dataType = GpuAssertDataType::Uint4;
#else
    uint _dataType;
#endif
    DK_SHADER_UINT4(_data) DK_SHADER_DEFAULT({});
};
#endif
#endif

#ifdef __cplusplus
    static_assert(sizeof(GpuAssertRecord) == 32);
    static_assert(offsetof(GpuAssertRecord, _fileId) == DK_GPU_ASSERT_HEADER_OFFSET);
    static_assert(offsetof(GpuAssertRecord, _data) == DK_GPU_ASSERT_DATA_OFFSET);
    static_assert(sizeof(SkeletonConstantBuffer) == sizeof(float4x4) * DK_MAX_BONE_COUNT);
    static_assert(sizeof(StaticMeshInstance) == 64);
    static_assert(sizeof(SceneObjectConstantBuffer) == 64);
    static_assert(sizeof(TerrainMeshConstantBuffer) == 64);
    static_assert(offsetof(TerrainMeshConstantBuffer, _parentOriginRelativeWorldXZ) == 32);
    static_assert(offsetof(TerrainMeshConstantBuffer, _type) == 40 && offsetof(TerrainMeshConstantBuffer, _level) == 44);
    static_assert(offsetof(TerrainMeshConstantBuffer, _hasParent) == 48 && offsetof(TerrainMeshConstantBuffer, _padding) == 52);
    static_assert(sizeof(OceanParams) == 64);
    static_assert(offsetof(OceanParams, _N) == 32);
    static_assert(sizeof(TerrainSectionConstants) == 16);
    static_assert(offsetof(TerrainSectionConstants, _terrainSectionResident) == 8);
    static_assert(sizeof(TerrainStreamingConstants) == 288);
    static_assert(offsetof(TerrainStreamingConstants, _terrainPageTableWidth) == 76);
    static_assert(offsetof(TerrainStreamingConstants, _terrainMinimumSampleX) == 128);
    static_assert(offsetof(TerrainStreamingConstants, _terrainMinimumSampleZ) == 132);
    static_assert(offsetof(TerrainStreamingConstants, _terrainSections) == 144);
    static_assert(sizeof(StaticMeshVertex) == 32);
    static_assert(offsetof(StaticMeshVertex, _uv) == 24);
    static_assert(sizeof(SkinnedMeshVertex) == 64);
    static_assert(sizeof(SkinnedMeshVertex::_boneIndices) / sizeof(uint32) == DK_MAX_SKINNING_COUNT);
    static_assert(offsetof(SkinnedMeshVertex, _boneIndices) == 32);
    static_assert(offsetof(SkinnedMeshVertex, _boneWeights) == 48);
    static_assert(sizeof(SceneConstantBuffer) == 224);
    static_assert(sizeof(CloudConstantBuffer) == 48);
    static_assert(offsetof(CloudConstantBuffer, _cloudWeatherIndex) == 28);
    static_assert(offsetof(CloudConstantBuffer, _cloudWindSpeedXZ) == 32);
    static_assert(offsetof(CloudConstantBuffer, _cloudNoiseIndex) == 40);
    static_assert(offsetof(SceneConstantBuffer, _cameraWorldMatrix) == 32);
    static_assert(sizeof(AtmosphereAbsorptionLayer) == 32, "Match HLSL absorption layer");
    static_assert(sizeof(SurfaceShadowCascade) == 80, "Match HLSL cascade layout");
    static_assert(sizeof(AtmosphereConstantBuffer) == 688, "Match the HLSL cbuffer layout");
    static_assert(offsetof(AtmosphereConstantBuffer, _previewViewIntervalCount) == 672);
    static_assert(offsetof(AtmosphereConstantBuffer, _previewAtmosphereShadows) == 676);
    static_assert(offsetof(AtmosphereConstantBuffer, _previewSurfaceBounce) == 680);
    static_assert(offsetof(AtmosphereConstantBuffer, _shadowCascades) == 336, "Match HLSL cascades");
    static_assert(offsetof(AtmosphereConstantBuffer, _multipleScatteringLutIndex) == 320, "Match HLSL multiple-scattering controls");
    static_assert(offsetof(AtmosphereConstantBuffer, _absorptionLayers) == 144, "Match HLSL absorption layers");
    static_assert(offsetof(AtmosphereConstantBuffer, _surfaceRight) == 208, "Match HLSL surface capture");
    static_assert(offsetof(AtmosphereConstantBuffer, _densityReferenceRadius) == 32, "Match HLSL density reference offset");
    static_assert(offsetof(AtmosphereConstantBuffer, _densityTransitionRadius) == 76, "Match HLSL transition offset");
    static_assert(offsetof(AtmosphereConstantBuffer, _rayleighScale) == 96, "Match scattering controls");
    static_assert(offsetof(AtmosphereConstantBuffer, _skyRadianceIndex) == 128, "Match sky controls");
    static_assert(sizeof(SpherePrimitiveInfo) == 32);
    static_assert(offsetof(SpherePrimitiveInfo, _color) == 16);
    static_assert(sizeof(LinePrimitiveInfo) == 36);
    static_assert(offsetof(LinePrimitiveInfo, _endWorldPosition) == 12);
    static_assert(offsetof(LinePrimitiveInfo, _color) == 24);
}
#endif

#endif
