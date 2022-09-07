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

#pragma region MainCamera
	Camera::gMainCamera = dk_new Camera(60.0f, width, height);
	Transform cameraTransform(float3(0, 1, -3), float3::Identity, float3::Identity);
	Camera::gMainCamera->SetWorldTransform(cameraTransform);
#pragma endregion

	// SceneRenderer�� Initialize�� RenderModule�� �ʿ��ϱ⶧���� ���� ����
	// #todo- RenderModule�� SceneRenderer������ ����� �� �� �ִٸ�.. SceneRenderer�� �����ڿ��� RenderModule�� initialize�ϴ� ������ ���ƺ���
	// >> DuckingEngine::getInstance().getRenderModule()�� ���ؼ� RenderModule�� �����ϴ� �ڵ尡 �ʹ� ����
#pragma region RenderModule
	_renderModule = dk_new RenderModule;
	if (_renderModule->initialize(hwnd, width, height) == false) return false;
#pragma endregion

	// Camera ������ �ʿ��ϱ� ������ �� ������ SceneConstantBuffer�� �����մϴ�.
	// #todo- ������ �������� ������Ʈ�ϱ� ������ ���� �̷��� �� �ʿ�� ����.. PreRender���� �ٷ� ������Ʈ�ϱ� ����
#pragma region SceneRenderer
	_sceneRenderer = dk_new SceneRenderer;
	_sceneRenderer->createSceneConstantBuffer();	// renderModule�� Camera�� �ʿ��ϱ� ������ �Ŀ� ������Ʈ
#pragma endregion

#pragma region Core Module
	_textureManager = dk_new TextureManager;
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

	_sceneObjectManager->Update(deltaTime);
}

void DuckingEngine::Render() const
{
	// #todo- ��Ŭ���� �ø�?

	// #todo- _renderModule�� _sceneRenderer�� Ȯ���� ����� �����ؾ���
	// _renderModule�� Graphics API�� ����ϴ� �뵵
	// _sceneRenderer�� renderModule�� �̿��Ͽ� Rendering�ϴ� �뵵
	// ���� �̰��� sceneRenderer->PreRender�� �Ǿ���ҵ�
	_renderModule->PreRender();

	// SceneConstantBufer Upload
	_sceneRenderer->uploadSceneConstantBuffer();

	// #todo- Render StaticMesh Object	// ����� Plane�� �ӽ÷� SkinnedMesh�� �����Ǿ��ֱ� ������ �̷��� �صξ����ϴ�.
	{
		SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
		plane._worldTransform.ToMatrix4x4(sceneObjectConstantBufferData._worldMatrix);
		sceneObjectConstantBufferData._worldMatrix.Transpose();
		uint8* sceneObjectConstantBuffer = plane._sceneObjectConstantBuffer->Map();
		memcpy(sceneObjectConstantBuffer, &sceneObjectConstantBufferData, sizeof(sceneObjectConstantBufferData));

		uint32 componentCount = static_cast<uint32>(plane._components.size());
		for (uint i = 0; i < componentCount; ++i)
		{
			// #todo- component ���� ���� �ʿ��غ���.
			// for���� �ƴ϶� unity, unreal������ GetComponent<T>�� ��� �۵��ϴ��� ���� ������ ��
			// ����ũ: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
			SkinnedMeshComponent* skinnedMeshComponent = static_cast<SkinnedMeshComponent*>(plane._components[i]);
			if (skinnedMeshComponent == nullptr)
			{
				continue;
			}

			_renderModule->RenderSkinnedMeshComponent(skinnedMeshComponent);
		}
	}

	// Render Character
	const DKHashMap<uint, SceneObject>& sceneObjects = _sceneObjectManager->GetCharacterSceneObjectContainer();
	for (auto iter = sceneObjects.begin(); iter != sceneObjects.end(); iter++)
	{
		const SceneObject& characterSceneObject = iter->second;

		SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
		characterSceneObject._worldTransform.ToMatrix4x4(sceneObjectConstantBufferData._worldMatrix);
		sceneObjectConstantBufferData._worldMatrix.Transpose();
		uint8* sceneObjectConstantBuffer = characterSceneObject._sceneObjectConstantBuffer->Map();
		memcpy(sceneObjectConstantBuffer, &sceneObjectConstantBufferData, sizeof(sceneObjectConstantBufferData));
		characterSceneObject._sceneObjectConstantBuffer->UnMap();

		uint32 componentCount = static_cast<uint32>(characterSceneObject._components.size());
		for (uint i = 0; i < componentCount; ++i)
		{
			// #todo- component ���� ���� �ʿ��غ���.
			// for���� �ƴ϶� unity, unreal������ GetComponent<T>�� ��� �۵��ϴ��� ���� ������ ��
			// ����ũ: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
			SkinnedMeshComponent* skinnedMeshComponent = static_cast<SkinnedMeshComponent*>(characterSceneObject._components[i]);
			if (skinnedMeshComponent == nullptr)
			{
				continue;
			}

			_renderModule->RenderSkinnedMeshComponent(skinnedMeshComponent);
		}
	}

	_renderModule->RenderUI();

	_renderModule->EndRender();
}