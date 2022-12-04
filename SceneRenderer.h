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
	class SceneRenderer
	{
	public:
		// Initialize
		bool initialize();

		// Helper
		const MaterialDefinition* getMaterialDefinition(const DKString& materialName) const;

		// Render
		void prepareShaderData() noexcept;
		void preRender() const noexcept;
		void updateRender() noexcept;
#ifdef _DK_DEBUG_
		void updateRender_Editor() noexcept;
#endif
		void endRender() const noexcept;

	private:
		bool initialize_createRenderPass();
		bool initialize_createMaterialDefinition();
		bool initialize_createSceneConstantBuffer();

	private:
		DKHashMap<DKString, MaterialDefinition> _materialDefinitionMap;

		Ptr<IBuffer> _sceneConstantBuffer = nullptr;
	};
}