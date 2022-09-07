#pragma once

class InputModule;
class RenderModule;
class SceneRenderer;
class TextureManager;
class ResourceManager;
class SceneObjectManager;
class GameModule;

class DuckingEngine
{
private:
	static DuckingEngine* _duckingEngine;
public:
	static DuckingEngine& getInstance()
	{
		if (_duckingEngine == nullptr)
		{
			_duckingEngine = dk_new DuckingEngine;
		}

		return *_duckingEngine;
	}

public:
	DuckingEngine()
	{
		if (_duckingEngine != nullptr)
		{
			DK_ASSERT_LOG(false, "Engine을 2개 생성을 시도하고 있습니다. 반드시 검토 바랍니다.");
		}
	}

	bool Initialize(HWND hwnd, int width, int height);

	void Update(float deltaTime) const;
	void Render() const;

	dk_inline const RenderModule& GetRenderModule() const noexcept { return *_renderModule; }
	dk_inline RenderModule& GetRenderModuleWritable() noexcept { return *_renderModule; }
	dk_inline const TextureManager& getTextureManager() const noexcept { return *_textureManager; }
	dk_inline TextureManager& getTextureManagerWritable() noexcept { return *_textureManager; }
	dk_inline const ResourceManager& GetResourceManager() const noexcept { return *_resourceManager; }
	dk_inline ResourceManager& GetResourceManagerWritable() noexcept { return *_resourceManager; }
	dk_inline const SceneObjectManager& GetSceneObjectManager() const noexcept { return *_sceneObjectManager; }
	dk_inline SceneObjectManager& GetSceneObjectManagerWritable() noexcept { return *_sceneObjectManager; }

private:
#pragma region System Modules
	static RenderModule* _renderModule;
	static SceneRenderer* _sceneRenderer;
#pragma endregion

#pragma region Editor&Game Modules
	static TextureManager* _textureManager;
	static ResourceManager* _resourceManager;
	static SceneObjectManager* _sceneObjectManager;
	static GameModule* _gameModule;
#pragma endregion
};