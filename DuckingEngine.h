#pragma once

namespace DK
{
	class InputModule;
	class RenderModule;
	class RaytracingRenderer;
	class SceneRenderer;
	class ResourceManager;
	class SceneObjectManager;
	class GameModule;
	class SceneManager;

	class DuckingEngine
	{
	private:
		static Ptr<DuckingEngine> _duckingEngine;
	public:
		static DuckingEngine& getInstance();
		void destroy();

	public:
		DuckingEngine();
		~DuckingEngine();

		bool Initialize(HWND hwnd, int width, int height);

		void Update(const float deltaTime);
		void Render(const float deltaTime);

		dk_inline const RenderModule& GetRenderModule() const noexcept { return *_renderModule.get(); }
		dk_inline RenderModule& GetRenderModuleWritable() noexcept { return *_renderModule.get(); }
		dk_inline const RaytracingRenderer& GetRaytracingRenderer() const noexcept { return *_raytracingRenderer.get(); }
		dk_inline RaytracingRenderer& GetRaytracingRendererWritable() noexcept { return *_raytracingRenderer.get(); }
		dk_inline const SceneRenderer& getSceneRender() const noexcept { return *_sceneRenderer.get(); }
		dk_inline SceneRenderer& getSceneRenderWritable() noexcept { return *_sceneRenderer.get(); }
		dk_inline const ResourceManager& GetResourceManager() const noexcept { return *_resourceManager.get(); }
		dk_inline ResourceManager& GetResourceManagerWritable() noexcept { return *_resourceManager.get(); }
		dk_inline const SceneManager& getSceneManager() const noexcept { return *_sceneManager.get(); }
		dk_inline SceneManager& getSceneManagerWritable() noexcept { return *_sceneManager.get(); }
		dk_inline const SceneObjectManager& GetSceneObjectManager() const noexcept { return *_sceneObjectManager.get(); }
		dk_inline SceneObjectManager& GetSceneObjectManagerWritable() noexcept { return *_sceneObjectManager.get(); }

	private:
#pragma region System Modules
		Ptr<RenderModule> _renderModule;
		Ptr<RaytracingRenderer> _raytracingRenderer;
		Ptr<SceneRenderer> _sceneRenderer;
#pragma endregion

#pragma region Editor&Game Modules
		Ptr<ResourceManager> _resourceManager;
		Ptr<SceneManager> _sceneManager;
		Ptr<SceneObjectManager> _sceneObjectManager;
		Ptr<GameModule> _gameModule;
#pragma endregion
	};
}
