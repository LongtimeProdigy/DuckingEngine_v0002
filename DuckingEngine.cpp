#include "stdafx.h"
#include "DuckingEngine.h"

// #todo- �̰� �� ��������
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "d3dx12.h"

#pragma region System Modules
#include "InputModule.h"
#include "RenderModule.h"
#include "SceneRenderer.h"
#pragma endregion

#pragma region Engine Core Module
#include "ResourceManager.h"
#include "TextureManager.h"
#include "SceneObjectManager.h"

#include "GameModule.h"
#pragma endregion

#include "EditorDebugDrawManager.h"

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Material.h"

DuckingEngine* DuckingEngine::_duckingEngine;
RenderModule* DuckingEngine::_renderModule = nullptr;
SceneRenderer* DuckingEngine::_sceneRenderer = nullptr;
TextureManager* DuckingEngine::_textureManager = nullptr;
ResourceManager* DuckingEngine::_resourceManager = nullptr;
SceneObjectManager* DuckingEngine::_sceneObjectManager = nullptr;
GameModule* DuckingEngine::_gameModule = nullptr;

bool DuckingEngine::Initialize(HWND hwnd, int width, int height)
{
#pragma region Input Modules
	if (InputModule::InitializePCController() == false) return false;
	if (InputModule::InitializeXBOXController(0) == false) return false;
#pragma endregion

	// SceneRenderer�� Initialize�� RenderModule�� �ʿ��ϱ⶧���� ���� ����
	// #todo- RenderModule�� SceneRenderer������ ����� �� �� �ִٸ�.. SceneRenderer�� �����ڿ��� RenderModule�� initialize�ϴ� ������ ���ƺ���
	// >> DuckingEngine::getInstance().getRenderModule()�� ���ؼ� RenderModule�� �����ϴ� �ڵ尡 �ʹ� ����
#pragma region RenderModule
	_renderModule = dk_new RenderModule;
	if (_renderModule->initialize(hwnd, width, height) == false) return false;
#pragma endregion

#pragma region MainCamera
	Camera::gMainCamera = dk_new Camera(90.0f, width, height);
	Transform cameraTransform(float3(0, 1, -1), float3::Zero, float3::Identity);
	Camera::gMainCamera->SetWorldTransform(cameraTransform);
#pragma endregion

	// Camera ������ �ʿ��ϱ� ������ �� ������ SceneConstantBuffer�� �����մϴ�.
	// #todo- ������ �������� ������Ʈ�ϱ� ������ ���� �̷��� �� �ʿ�� ����.. PreRender���� �ٷ� ������Ʈ�ϱ� ����
#pragma region SceneRenderer
	_sceneRenderer = dk_new SceneRenderer;
	if (_sceneRenderer->initialize() == false) return false;
#pragma endregion

#pragma region Core Module
	_textureManager = dk_new TextureManager;
	if (_textureManager->initialize() == false) return false;
	_resourceManager = dk_new ResourceManager;
	_sceneObjectManager = dk_new SceneObjectManager;
#pragma endregion

#pragma region Editor Modules
#if defined(_DK_DEBUG_)
	// ���� Editor�� ������� �ʾ� �ƹ��͵� ���� �ʽ��ϴ�.
	EditorDebugDrawManager::getSingleton().initialize();
#endif
#pragma endregion

#pragma region Game Module
	_gameModule = dk_new GameModule;
	if (_gameModule->Initialize() == false) return false;
#pragma endregion

	return true;
}

void DuckingEngine::Update(float deltaTime) const
{
	InputModule::Update();

	Camera::gMainCamera->Update();

	_sceneObjectManager->update(deltaTime);
}

void DuckingEngine::Render() const
{
	_sceneRenderer->prepareShaderData();

	_sceneRenderer->preRender();
	_sceneRenderer->updateRender();
	_sceneRenderer->EndRender();
}