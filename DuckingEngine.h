#pragma once

class InputModule;
class RenderModule;
class TextureManager;
class ResourceManager;
class SceneObjectManager;
class GameModule;

class DuckingEngine
{
public:
	static const DuckingEngine* _duckingEngine;

public:
	DuckingEngine()
	{
		if (_duckingEngine != nullptr)
		{
			DK_ASSERT_LOG(false, "Engine을 두 개 생성해서는 안됩니다.");
			return;
		}

		_duckingEngine = this;
	}
	~DuckingEngine();

	bool Initialize(HWND hwnd, int width, int height);

	void Update() const;
	void Render() const;

	dk_inline static const RenderModule* GetRenderModule() noexcept { return _renderModule; }
	dk_inline static RenderModule* GetRenderModuleWritable() noexcept { return _renderModule; }
	dk_inline static const ResourceManager* GetResourceManager() noexcept { return _resourceManager; }
	dk_inline static ResourceManager* GetResourceManagerWritable() noexcept { return _resourceManager; }
	dk_inline static const SceneObjectManager* GetSceneObjectManager() noexcept { return _sceneObjectManager; }
	dk_inline static SceneObjectManager* GetSceneObjectManagerWritable() noexcept { return _sceneObjectManager; }

private:
#pragma region System Modules
	static RenderModule* _renderModule;
#pragma endregion

#pragma region Editor&Game Modules
	static TextureManager* _textureManager;
	static ResourceManager* _resourceManager;
	static SceneObjectManager* _sceneObjectManager;
	static GameModule* _gameModule;
#pragma endregion
};