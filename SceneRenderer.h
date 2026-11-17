#pragma once

#include "Material.h"
#include "RenderModule.h"
#include "Resource/Material/Atmosphere/AtmosphereLimits.h"
#include <cstddef>
#include <cmath>

namespace DK
{
	struct IBuffer;
	class SkinnedMeshComponent;
    class RenderModule;
	struct MaterialDefinition;
}

namespace DK
{
	struct SceneConstantBuffer
	{
		uint32 _frameIndex;

		uint32 _resolution[2];
		float _time;

		float _nearDistance;
		uint32 _rngState;
		uint32 _paddingSCB[2] = {};

		float4x4 _cameraWorldMatrix;
		float4x4 _cameraWorldMatrixInv;
		float4x4 _cameraProjectionMatrix;
	};
	static_assert(sizeof(SceneConstantBuffer) == 224);
	static_assert(offsetof(SceneConstantBuffer, _cameraWorldMatrix) == 32);

    struct AtmosphereAbsorptionLayer
    {
        float _extinction[3] = { 0, 0, 0 };
        float _strength = 0;
        float _peakRadius = 0;
        float _lowerWidth = 10000;
        float _upperWidth = 10000;
        float _reserved = 0;

        void validate([[maybe_unused]] uint32 layerIndex) const
        {
            for (uint32 channel = 0; channel < DK_COUNT_OF(_extinction); ++channel)
                DK_ASSERT_LOG(std::isfinite(_extinction[channel]) && _extinction[channel] >= 0.0f, "Absorption layer %u: invalid _extinction[%u]=%g", layerIndex, channel, _extinction[channel]);
            DK_ASSERT_LOG(std::isfinite(_strength) && _strength >= 0.0f, "Absorption layer %u: invalid _strength=%g", layerIndex, _strength);
            DK_ASSERT_LOG(std::isfinite(_peakRadius) && _peakRadius >= 0.0f, "Absorption layer %u: invalid _peakRadius=%g", layerIndex, _peakRadius);
            DK_ASSERT_LOG(std::isfinite(_lowerWidth) && _lowerWidth > 0.0f, "Absorption layer %u: invalid _lowerWidth=%g", layerIndex, _lowerWidth);
            DK_ASSERT_LOG(std::isfinite(_upperWidth) && _upperWidth > 0.0f, "Absorption layer %u: invalid _upperWidth=%g", layerIndex, _upperWidth);
            if (_strength > 0.0f && (_extinction[0] > 0.0f || _extinction[1] > 0.0f || _extinction[2] > 0.0f))
                DK_ASSERT_LOG(_peakRadius >= _lowerWidth, "Absorption layer %u: inner radius is negative, _peakRadius=%g, _lowerWidth=%g", layerIndex, _peakRadius, _lowerWidth);
        }
    };
    static_assert(sizeof(AtmosphereAbsorptionLayer) == 32, "Match HLSL absorption layer");

    struct SurfaceShadowCascade
    {
        float _right[4] = {}, _up[4] = {}, _forward[4] = {}, _origin[4] = {};
        uint32 _albedoIndex = 0, _depthIndex = 0, _mapSize = 512, _padding = 0;
    };
    static_assert(sizeof(SurfaceShadowCascade) == 80, "Match HLSL cascade layout");

    static constexpr const float kEarthAtmosphereRadius = 6390000.0f;

	struct AtmosphereConstantBuffer
	{
		uint32 _numInScatteringPoints = 8;
		uint32 _opticalDepthPointCount = 16;
		float _rayleighHeight = 7994.0f;
		float _mieHeight = 1200.0f;

		float _sunDegree = 90.f;
		float _sunIrradiance = 20.0f;
		float _scatteringScale = 1.0f;
		float _cameraAltitude = 0.0f;

		float _densityReferenceRadius = 6360000.0f;
		float _planetCentre[3] = { 0, -6360000.0f, 0 };

		float _atmosphereRadius = kEarthAtmosphereRadius;
		float _sunAngularRadius = 0.27f;
		float _exposureEV = 0.0f;
		float _bloomStrength = 0.02f;

		float _bloomRadius = 16.0f;
		float _bloomThreshold = 2.0f;
		float _sunRadianceScale = 1.0f;
		float _densityTransitionRadius = 6358000.0f;

		float _integrationRelativeTolerance = 0.002f;
		uint32 _integrationMaxDepth = 8;
		uint32 _atmosphereSamplingMode = 0;
		uint32 _transmittanceLutIndex = 0;

        float _rayleighScale = 1.0f;
        float _mieScale = 1.0f;
        float _mieAbsorptionScale = 0.0f;
        float _mieAnisotropy = 0.76f;
        float _sunColor[3] = { 1.0f, 1.0f, 1.0f };
        float _skyLightingStrength = 1.0f;
        uint32 _skyRadianceIndex = 0;
        uint32 _skyFilteredIndex = 0;
        uint32 _viewNodeBudget = 256;
        uint32 _opticalNodeBudget = 128;
        AtmosphereAbsorptionLayer _absorptionLayers[DK_ATMOSPHERE_ABSORPTION_LAYER_COUNT];
        float _surfaceRight[4] = { 1, 0, 0, 2000 };
        float _surfaceUp[4] = { 0, 0, 1, 2000 };
        float _surfaceForward[4] = { 0, -1, 0, 8000 };
        float _surfaceOrigin[4] = { 0, 4000, 0, 0 };
        uint32 _surfaceAlbedoIndex = 0, _surfaceNormalIndex = 0, _surfaceMaterialIndex = 0, _surfaceDepthIndex = 0;
        uint32 _surfaceBounceIndex = 0, _surfaceMapEnabled = 0, _surfaceMapSize = 512;
        float _surfaceShadowStrength = 1.0f;
        float _surfaceBounceStrength = 1.0f, _surfaceBias = 1.0f;
        uint32 _surfaceBounceSamples = 32;
        float _surfacePadding = 0;
        uint32 _multipleScatteringLutIndex = 0;
        float _multipleScatteringStrength = 1.0f;
        uint32 _multipleScatteringMaxOrder = 4;
        float _multipleScatteringPadding = 0;
        SurfaceShadowCascade _shadowCascades[DK_ATMOSPHERE_SHADOW_CASCADE_COUNT];
        uint32 _shadowCascadeCount = DK_ATMOSPHERE_SHADOW_CASCADE_COUNT;
        float _shadowCascadeBlend = 0.1f;
        float _shadowCascadePadding[2] = {};

        void validate() const
        {
            DK_ASSERT_LOG(std::isfinite(_densityReferenceRadius) && _densityReferenceRadius > 0.0f, "Invalid _densityReferenceRadius=%g", _densityReferenceRadius);
            DK_ASSERT_LOG(std::isfinite(_densityTransitionRadius) && _densityTransitionRadius > 0.0f && _densityTransitionRadius < _densityReferenceRadius, "Invalid _densityTransitionRadius=%g, _densityReferenceRadius=%g", _densityTransitionRadius, _densityReferenceRadius);
            DK_ASSERT_LOG(std::isfinite(_atmosphereRadius) && _atmosphereRadius > _densityTransitionRadius, "Invalid _atmosphereRadius=%g, _densityTransitionRadius=%g", _atmosphereRadius, _densityTransitionRadius);
            DK_ASSERT_LOG(_atmosphereRadius - _densityTransitionRadius <= ATMOSPHERE_LUT_MAX_RADIAL_DEPTH, "Transmittance LUT diagnostic range exceeded: _atmosphereRadius=%g, _densityTransitionRadius=%g, maximum span=%g", _atmosphereRadius, _densityTransitionRadius, ATMOSPHERE_LUT_MAX_RADIAL_DEPTH);
            DK_ASSERT_LOG(std::isfinite(_rayleighHeight) && _rayleighHeight > 0.0f, "Invalid _rayleighHeight=%g", _rayleighHeight);
            DK_ASSERT_LOG(std::isfinite(_mieHeight) && _mieHeight > 0.0f, "Invalid _mieHeight=%g", _mieHeight);
            DK_ASSERT_LOG(std::isfinite(_scatteringScale) && _scatteringScale >= 0.0f, "Invalid _scatteringScale=%g", _scatteringScale);
            DK_ASSERT_LOG(std::isfinite(_rayleighScale) && _rayleighScale >= 0.0f, "Invalid _rayleighScale=%g", _rayleighScale);
            DK_ASSERT_LOG(std::isfinite(_mieScale) && _mieScale >= 0.0f, "Invalid _mieScale=%g", _mieScale);
            DK_ASSERT_LOG(std::isfinite(_mieAbsorptionScale) && _mieAbsorptionScale >= 0.0f, "Invalid _mieAbsorptionScale=%g", _mieAbsorptionScale);
            DK_ASSERT_LOG(std::isfinite(_mieAnisotropy), "Invalid _mieAnisotropy=%g", _mieAnisotropy);
            DK_ASSERT_LOG(std::isfinite(_sunDegree), "Invalid _sunDegree=%g", _sunDegree);
            DK_ASSERT_LOG(std::isfinite(_sunIrradiance) && _sunIrradiance >= 0.0f, "Invalid _sunIrradiance=%g", _sunIrradiance);
            for (uint32 channel = 0; channel < DK_COUNT_OF(_sunColor); ++channel)
            {
                DK_ASSERT_LOG(std::isfinite(_sunColor[channel]) && _sunColor[channel] >= 0.0f, "Invalid _sunColor[%u]=%g", channel, _sunColor[channel]);
                DK_ASSERT_LOG(std::isfinite(_planetCentre[channel]), "Invalid _planetCentre[%u]=%g", channel, _planetCentre[channel]);
            }
            DK_ASSERT_LOG(_surfaceBounceSamples <= SURFACE_BOUNCE_MAX_SAMPLES, "Invalid _surfaceBounceSamples=%u, maximum=%u", _surfaceBounceSamples, SURFACE_BOUNCE_MAX_SAMPLES);
            DK_ASSERT_LOG(_shadowCascadeCount <= DK_ATMOSPHERE_SHADOW_CASCADE_COUNT, "Invalid _shadowCascadeCount=%u", _shadowCascadeCount);
            for (uint32 index = 0; index < DK_COUNT_OF(_absorptionLayers); ++index)
                _absorptionLayers[index].validate(index);
        }
	};
	static_assert(sizeof(AtmosphereConstantBuffer) == 672, "Match the HLSL cbuffer layout");
    static_assert(offsetof(AtmosphereConstantBuffer, _shadowCascades) == 336, "Match HLSL cascades");
    static_assert(offsetof(AtmosphereConstantBuffer, _multipleScatteringLutIndex) == 320, "Match HLSL multiple-scattering controls");
    static_assert(offsetof(AtmosphereConstantBuffer, _absorptionLayers) == 144, "Match HLSL absorption layers");
    static_assert(offsetof(AtmosphereConstantBuffer, _surfaceRight) == 208, "Match HLSL surface capture");
	static_assert(offsetof(AtmosphereConstantBuffer, _densityReferenceRadius) == 32, "Match HLSL density reference offset");
	static_assert(offsetof(AtmosphereConstantBuffer, _densityTransitionRadius) == 76, "Match HLSL transition offset");

	static_assert(offsetof(AtmosphereConstantBuffer, _rayleighScale) == 96, "Match scattering controls");
	static_assert(offsetof(AtmosphereConstantBuffer, _skyRadianceIndex) == 128, "Match sky controls");

	class RaytracingRenderer;
	class Camera;

	class SceneRenderer
	{
		friend class RaytracingRenderer;

	public:
		// Initialize
		bool initialize();

		// Helper
		const MaterialDefinition* getMaterialDefinition(uint64 materialNameHash) const;
		const MaterialDefinition* getMaterialDefinition(const char* materialName) const { return getMaterialDefinition(StringUtil::hash(materialName)); }

		// Render
		void prepareShaderData(const float deltaTime) noexcept;
#if defined(USE_IMGUI)
		void renderUI() noexcept;
#endif
		void updateRender() noexcept;
		void endRender() const noexcept;
		void updateAtmosphereCamera(Camera& camera) const noexcept;
        void setAbsorptionLayer(uint32 index, const AtmosphereAbsorptionLayer& layer)
        {
            DK_ASSERT_LOG(index < DK_COUNT_OF(_atmosphereConstantBufferData._absorptionLayers), "Invalid atmosphere absorption layer index: %u", index);
            layer.validate(index);
            _atmosphereConstantBufferData._absorptionLayers[index] = layer;
        }

	private:
		bool initialize_createRenderPass();
		bool initialize_renderPassHandles();
		bool initialize_createMaterialDefinition();
		bool initialize_createSceneConstantBuffer();
		void updateAtmosphereTransmittanceLut();
        void updateAtmosphereSkyLighting();
        void updateAtmosphereMultipleScattering();
        void prepareSurfaceCapture(const float3& cameraPosition);
		void validateSurfaceCaptureParameters() const;
        void updateSurfaceTransport();
        void drawSceneGeometry(RenderModule& renderModule, const IBufferRef& sceneConstants, const SceneConstantBuffer& sceneConstantsData, bool includeDebug);
    void drawGrass(RenderModule& renderModule);

	private:
        static constexpr uint32 kAtmosphereLutWidth = ATMOSPHERE_LUT_WIDTH;
        static constexpr uint32 kAtmosphereLutHeight = ATMOSPHERE_LUT_HEIGHT;

		DKHashMap<uint64, MaterialDefinition> _materialDefinitionMap;

		RenderPass* _transmittancePass = nullptr;
		RenderPass* _multipleScatteringPass = nullptr;
		RenderPass* _surfaceBouncePass = nullptr;
		RenderPass* _skyLightingPass = nullptr;
		RenderPass* _oceanPass = nullptr;
		RenderPass* _gbufferResolvePass = nullptr;
		RenderPass* _atmospherePass = nullptr;
		RenderPass* _bloomHorizontalPass = nullptr;
		RenderPass* _bloomVerticalPass = nullptr;
		RenderPass* _finalOutputPass = nullptr;
		RenderPass* _mainGeometryPass = nullptr;
		const RenderPassInstance* _mainGeometryClearPass = nullptr;
		const RenderPassInstance* _surfaceCapturePass = nullptr;
		const RenderPassInstance* _nearShadowPasses[DK_ATMOSPHERE_SHADOW_CASCADE_COUNT - 1] = {};

        // Debug
#if defined(USE_IMGUI)
        bool _localGeometryEnabled = true;
        bool _compareExtendedAtmosphereTop = false;
#endif
        float3 _localGeometryCentre = float3(0, 0, 0);
        float _localGeometryRadius = 50000.0f;

        // CPU Data
		bool _renderLocalScene = false;

        // SceneConstant
		SceneConstantBuffer _sceneConstantBufferData;
		IBufferRef _sceneConstantBuffer = nullptr;

        // Atmosphere
        AtmosphereConstantBuffer _atmosphereConstantBufferData;
        IBufferRef _atmosphereConstantBuffer = nullptr;
        AtmosphereConstantBuffer _cachedSkyParameters;              // refresh flag를 설정하기 위한 prev저장용일뿐임
        float _cachedSkyPosition[3] = { 0, 0, 0 };                  // refresh flag를 설정하기 위한 prev저장용일뿐임
        //// LUT
        AtmosphereConstantBuffer _cachedAtmosphereLutParameters;    // refresh flag를 설정하기 위한 prev저장용일뿐임
        ITextureRef _atmosphereTransmittanceLut = nullptr;
        bool _atmosphereLutValid = false;
        //// Sky
        bool _skyLightingValid = false;
        ITextureRef _skyRadiance = nullptr;
        ITextureRef _skyFiltered = nullptr;
        //// mutli-scattering
        bool _multipleScatteringValid = false;
        float _cachedMultipleAnisotropy = 0;
        uint32 _cachedMultipleMaxOrder = 0;
        ITextureRef _multipleScatteringLut[2] = { nullptr, nullptr };

        // Shadow & Capture
        //// Surface
        bool _surfaceTransportValid = false;
        uint64 _lastSurfaceTransportUpdate = 0;
        AtmosphereConstantBuffer _cachedSurfaceParameters;          // refresh flag를 설정하기 위한 prev저장용일뿐임
        float _surfaceCaptureExtent = 2000.0f;
        float _surfaceCaptureDepth = 8000.0f;
        IBufferRef _surfaceSceneConstantBuffer = nullptr;
        IBufferRef _nearShadowSceneConstants[DK_ATMOSPHERE_SHADOW_CASCADE_COUNT - 1];
        SceneConstantBuffer _surfaceSceneConstantsData[DK_ATMOSPHERE_SHADOW_CASCADE_COUNT];
        uint32 _staticMeshDrawCount = 0;
        uint32 _staticMeshInstanceCount = 0;
        uint32 _staticMeshVisibleChunkCount = 0;
        uint32 _staticMeshTotalChunkCount = 0;
        uint64 _staticMeshTriangleCount = 0;
        ITextureRef _surfaceBounce = nullptr;
	};
}
