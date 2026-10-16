#pragma once

#include "Material.h"
#include <cstddef>

namespace DK
{
	struct IBuffer;
	class SkinnedMeshComponent;
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
	struct AtmosphereConstantBuffer
	{
		uint32 _numInScatteringPoints = 8;
		uint32 _opticalDepthPointCount = 16;
		float _rayleighHeight = 7994.0f;
		float _mieHeight = 1200.0f;

		float _sunDegree = 90.f;
		float _sunIntensity = 20.0f;
		float _scatteringScale = 1.0f;
		float _cameraAltitude = 0.0f;

		float _densityReferenceRadius = 6360000.0f;
		float _planetCentre[3] = { 0, -6360000.0f, 0 };

		float _atmosphereRadius = 6390000.0f;
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
	};
	static_assert(sizeof(AtmosphereConstantBuffer) == 96, "Match the HLSL cbuffer layout");
	static_assert(offsetof(AtmosphereConstantBuffer, _densityReferenceRadius) == 32, "Match HLSL density reference offset");
	static_assert(offsetof(AtmosphereConstantBuffer, _densityTransitionRadius) == 76, "Match HLSL transition offset");

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
		void preRender() noexcept;
		void updateRender() noexcept;
		void endRender() const noexcept;
		void setOrbitView() const noexcept;
		void updateAtmosphereCamera(Camera& camera) const noexcept;

	private:
		bool initialize_createRenderPass();
		bool initialize_createMaterialDefinition();
		bool initialize_createSceneConstantBuffer();
		void updateAtmosphereTransmittanceLut();

	private:
		DKHashMap<DKString, MaterialDefinition> _materialDefinitionMap;

		SceneConstantBuffer _sceneConstantBufferData;
		IBufferRef _sceneConstantBuffer = nullptr;

		AtmosphereConstantBuffer _atmosphereConstantBufferData;	// TODO: 지금은 ingui때문에 preRender에서 set하기때문에 mutable붙었는데.. UI시스템 구성되면 지워야한다.
		bool _renderLocalScene = false;
		IBufferRef _atmosphereConstantBuffer = nullptr;
		ITextureRef _atmosphereTransmittanceLut = nullptr;
		AtmosphereConstantBuffer _cachedAtmosphereLutParameters;
		bool _atmosphereLutValid = false;
		static constexpr uint32 kAtmosphereLutWidth = 512;
		static constexpr uint32 kAtmosphereLutHeight = 128;
	};
}
