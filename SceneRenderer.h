#pragma once

#include "Resource/Material/ShaderStructures.h"

#include "Material.h"
#include "RenderModule.h"

namespace DK
{
	struct IBuffer;
	class SkinnedMeshComponent;
    class RenderModule;
	struct MaterialDefinition;

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
