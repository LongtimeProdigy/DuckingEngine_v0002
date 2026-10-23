#pragma once

#include "Material.h"
#include "RenderModule.h"
#include <cstddef>

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
		float _farDistance;
		uint32 _rngState;
		float _padding;

		float4x4 _cameraWorldMatrix;
		float4x4 _cameraWorldMatrixInv;
		float4x4 _cameraProjectionMatrix;
	};
    struct AtmosphereAbsorptionLayer
    {
        float extinction[3] = { 0, 0, 0 };
        float strength = 0;
        float peakRadius = 0;
        float lowerWidth = 10000;
        float upperWidth = 10000;
        float reserved = 0;
    };
    static_assert(sizeof(AtmosphereAbsorptionLayer) == 32, "Match HLSL absorption layer");

    struct SurfaceShadowCascade
    {
        float right[4] = {}, up[4] = {}, forward[4] = {}, origin[4] = {};
        uint32 albedoIndex = 0, depthIndex = 0, mapSize = 512, padding = 0;
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
        AtmosphereAbsorptionLayer _absorptionLayers[2];
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
        SurfaceShadowCascade _shadowCascades[4];
        uint32 _shadowCascadeCount = 4;
        float _shadowCascadeBlend = 0.1f;
        float _shadowCascadePadding[2] = {};
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
		const MaterialDefinition* getMaterialDefinition(const DKString& materialName) const;

		// Render
		void prepareShaderData(const float deltaTime) noexcept;
		void renderUI() noexcept;
		void updateRender() noexcept;
		void endRender() const noexcept;
		void updateAtmosphereCamera(Camera& camera) const noexcept;
        void setAbsorptionLayer(uint32 index, const AtmosphereAbsorptionLayer& layer) { if (index < 2u) _atmosphereConstantBufferData._absorptionLayers[index] = layer; }

	private:
		bool initialize_createRenderPass();
		bool initialize_createMaterialDefinition();
		bool initialize_createSceneConstantBuffer();
		void updateAtmosphereTransmittanceLut();
        void updateAtmosphereSkyLighting();
        void updateAtmosphereMultipleScattering();
        void prepareSurfaceCapture(const float3& cameraPosition);
        void updateSurfaceTransport();
        void drawSceneGeometry(RenderModule& renderModule, const IBufferRef& sceneConstants, bool includeDebug);

	private:
        static constexpr uint32 kAtmosphereLutWidth = 512;
        static constexpr uint32 kAtmosphereLutHeight = 128;

		DKHashMap<DKString, MaterialDefinition> _materialDefinitionMap;

        // Debug
        bool _localGeometryEnabled = true;
        float3 _localGeometryCentre = float3(0, 0, 0);
        float _localGeometryRadius = 50000.0f;
        bool _compareExtendedAtmosphereTop = false;

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
        ReflectiveSurfaceMap _surfaceMap;
        ReflectiveSurfaceMap _nearShadowMaps[3];
        IBufferRef _surfaceSceneConstantBuffer = nullptr;
        IBufferRef _nearShadowSceneConstants[3];
        ITextureRef _surfaceBounce = nullptr;
	};
}
