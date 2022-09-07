#pragma once

struct ID3D12Resource;

class SceneRenderer
{
public:
	void createSceneConstantBuffer() noexcept;
	void uploadSceneConstantBuffer() const noexcept;

	void PreRender() const noexcept;
	void RenderSkinnedMesh(const SkinnedMeshComponent* skinnedMeshComponent) const noexcept;
	void RenderUI() const noexcept;
	void EndRender() const noexcept;

private:
	ID3D12Resource* _sceneConstantBuffer = nullptr;
};