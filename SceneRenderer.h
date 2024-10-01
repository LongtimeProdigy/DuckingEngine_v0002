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
		float _padding1[2];

		float4x4 _cameraWorldMatrix;
		float4x4 _cameraWorldMatrixInv;
		float4x4 _cameraProjectionMatrix;
	};
	struct AtmosphereConstantBuffer
	{
		uint32 _numInScatteringPoints = 16;
		uint32 _opticalDepthPointCount = 8;
		uint32 _densityFallOff = 1;
		uint32 _scatteringStrength = 21;

		float _sunDegree = 7.f;
		uint32 _sunIntensity = 20;
		uint32 _sunRadius = 10;
		uint32 _padding;

		int _planetRadius = 0;
		int _planetCentre[3] = { 0, -0, 0 };	// ���� ������ 6371km

		int _atmosphereRadius = 6380000 - 6360000;				// ��� ������ (1unit = 1m >> 30000  = 30km) [10km �����, 50km ������, 80km �߰���, 600km ����], ���� ����� 99%�� 30km���� ����
	};

	class SceneRenderer
	{
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

	private:
		bool initialize_createRenderPass();
		bool initialize_createMaterialDefinition();
		bool initialize_createSceneConstantBuffer();

	private:
		DKHashMap<DKString, MaterialDefinition> _materialDefinitionMap;

		SceneConstantBuffer _sceneConstantBufferData;
		Ptr<IBuffer> _sceneConstantBuffer = nullptr;

		mutable AtmosphereConstantBuffer _atmosphereConstantBufferData;	// TODO: ������ ingui������ preRender���� set�ϱ⶧���� mutable�پ��µ�.. UI�ý��� �����Ǹ� �������Ѵ�.
		Ptr<IBuffer> _atmosphereConstantBuffer = nullptr;
	};
}