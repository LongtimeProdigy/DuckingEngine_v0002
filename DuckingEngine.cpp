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

		// SceneRenderer�� Initialize�� RenderModule�� �ʿ��ϱ⶧���� ���� ����
		// #todo- RenderModule�� SceneRenderer������ ����� �� �� �ִٸ�.. SceneRenderer�� �����ڿ��� RenderModule�� initialize�ϴ� ������ ���ƺ���
		// >> DuckingEngine::getInstance().getRenderModule()�� ���ؼ� RenderModule�� �����ϴ� �ڵ尡 �ʹ� ����
		_renderModule = dk_new RenderModule;
		if (_renderModule->initialize(hwnd, width, height) == false) 
			return false;

		Camera::gMainCamera = dk_new Camera(60, width, height);
		Transform cameraTransform(float3(0, 1, -1), Quaternion(0, 0, 0), float3::Identity);
		Camera::gMainCamera->set_worldTransform(cameraTransform);

		// Camera ������ �ʿ��ϱ� ������ �� ������ SceneConstantBuffer�� �����մϴ�.
		// #todo- ������ �������� ������Ʈ�ϱ� ������ ���� �̷��� �� �ʿ�� ����.. PreRender���� �ٷ� ������Ʈ�ϱ� ����
		_sceneRenderer = dk_new SceneRenderer;
		if (_sceneRenderer->initialize() == false) 
			return false;

		_resourceManager = dk_new ResourceManager;

		_sceneManager = dk_new SceneManager;
		_sceneObjectManager = dk_new SceneObjectManager;

#if defined(_DK_DEBUG_)
		// ���� Editor�� ������� �ʾ� �ƹ��͵� ���� �ʽ��ϴ�.
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