#include "stdafx.h"
#include "DuckingEngine.h"

#include "InputModule.h"
#include "RenderModule.h"
#include "SceneRenderer.h"

#include "ResourceManager.h"
#include "TextureManager.h"
#include "SceneManager.h"
#include "SceneObjectManager.h"

#include "GameModule.h"

#if defined(_DK_DEBUG_)
#include "EditorDebugDrawManager.h"
#endif

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"

namespace DK
{
	DuckingEngine* DuckingEngine::_duckingEngine;
	RenderModule* DuckingEngine::_renderModule = nullptr;
	SceneRenderer* DuckingEngine::_sceneRenderer = nullptr;
	ResourceManager* DuckingEngine::_resourceManager = nullptr;
	SceneManager* DuckingEngine::_sceneManager = nullptr;
	SceneObjectManager* DuckingEngine::_sceneObjectManager = nullptr;
	GameModule* DuckingEngine::_gameModule = nullptr;

	bool DuckingEngine::Initialize(HWND hwnd, int width, int height)
	{
		if (InputModule::InitializePCController() == false) 
			return false;
		if (InputModule::InitializeXBOXController(0) == false) 
			return false;

		// SceneRenderer의 Initialize에 RenderModule이 필요하기때문에 먼저 생성
		// #todo- RenderModule은 SceneRenderer에서만 쓰기로 할 수 있다면.. SceneRenderer의 생성자에서 RenderModule을 initialize하는 방향이 좋아보임
		// >> DuckingEngine::getInstance().getRenderModule()을 통해서 RenderModule에 접근하는 코드가 너무 많음
		_renderModule = dk_new RenderModule;
		if (_renderModule->initialize(hwnd, width, height) == false) 
			return false;

		Camera::gMainCamera = dk_new Camera(60, width, height);
		Transform cameraTransform(float3(0, 1, -1), Quaternion(0, 0, 0), float3::Identity);
		Camera::gMainCamera->set_worldTransform(cameraTransform);

		// Camera 정보가 필요하기 때문에 이 곳에서 SceneConstantBuffer를 생성합니다.
		// #todo- 어차피 매프레임 업데이트하기 때문에 굳이 이렇게 할 필요는 없음.. PreRender직후 바로 업데이트하기 때문
		_sceneRenderer = dk_new SceneRenderer;
		if (_sceneRenderer->initialize() == false) 
			return false;

		_resourceManager = dk_new ResourceManager;

		_sceneManager = dk_new SceneManager;
		_sceneObjectManager = dk_new SceneObjectManager;

#if defined(_DK_DEBUG_)
		// 현재 Editor는 사용하지 않아 아무것도 하지 않습니다.
		EditorDebugDrawManager::getSingleton().initialize();
#endif

		_gameModule = dk_new GameModule;
		if (_gameModule->initialize() == false) return false;

		return true;
	}

	void DuckingEngine::Update(float deltaTime) const
	{
		InputModule::Update();
		Camera::gMainCamera->update(deltaTime);
		_sceneObjectManager->update(deltaTime);
	}

	void DuckingEngine::Render() const
	{
		_sceneRenderer->prepareShaderData();

		_sceneRenderer->preRender();
		_sceneRenderer->updateRender();
		_sceneRenderer->endRender();
	}
}