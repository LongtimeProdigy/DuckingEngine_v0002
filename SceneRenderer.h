#pragma once

#include "Material.h"

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
		uint32 _numInScatteringPoints = 32;
		uint32 _opticalDepthPointCount = 16;
		float _rayleighHeight = 7994.0f;
		float _mieHeight = 1200.0f;

		float _sunDegree = 90.f;
		float _sunIntensity = 20.0f;
		float _scatteringScale = 1.0f;
		float _cameraAltitude = 0.0f;

		float _planetRadius = 6360000.0f;
		float _planetCentre[3] = { 0, -6360000.0f, 0 };

		float _atmosphereRadius = 6390000.0f;
		float _sunAngularRadius = 0.27f;
		float _exposureEV = 0.0f;
		float _bloomStrength = 0.02f;

		float _bloomRadius = 16.0f;
		float _bloomThreshold = 2.0f;
		float _sunRadianceScale = 1.0f;
		float _padding = 0.0f;
	};
	static_assert(sizeof(AtmosphereConstantBuffer) == 80, "Match the HLSL cbuffer layout");

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
		void preRender() const noexcept;
		void updateRender() noexcept;
		void endRender() const noexcept;
		void setOrbitView() const noexcept;
		void updateAtmosphereCamera(Camera& camera) const noexcept;

	private:
		bool initialize_createRenderPass();
		bool initialize_createMaterialDefinition();
		bool initialize_createSceneConstantBuffer();

	private:
		DKHashMap<DKString, MaterialDefinition> _materialDefinitionMap;

		SceneConstantBuffer _sceneConstantBufferData;
		IBufferRef _sceneConstantBuffer = nullptr;

		mutable AtmosphereConstantBuffer _atmosphereConstantBufferData;	// TODO: 지금은 ingui때문에 preRender에서 set하기때문에 mutable붙었는데.. UI시스템 구성되면 지워야한다.
		bool _renderLocalScene = false;
		IBufferRef _atmosphereConstantBuffer = nullptr;
	};
}
