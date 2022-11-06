#pragma once

struct IBuffer;

class SkinnedMeshComponent;
struct MaterialDefinition;

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
	void EndRender() const noexcept;

private:
	bool initialize_createRenderPass();
	bool initialize_createMaterialDefinition();
	bool initialize_createSceneConstantBuffer();

private:
	DKHashMap<DKString, MaterialDefinition> _materialDefinitionMap;

	Ptr<IBuffer> _sceneConstantBuffer = nullptr;
};