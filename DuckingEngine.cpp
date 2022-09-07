#include "stdafx.h"
#include "DuckingEngine.h"

// #todo- 이거 꼭 지워야함
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

	// SceneRenderer의 Initialize에 RenderModule이 필요하기때문에 먼저 생성
	// #todo- RenderModule은 SceneRenderer에서만 쓰기로 할 수 있다면.. SceneRenderer의 생성자에서 RenderModule을 initialize하는 방향이 좋아보임
	// >> DuckingEngine::getInstance().getRenderModule()을 통해서 RenderModule에 접근하는 코드가 너무 많음
#pragma region RenderModule
	_renderModule = dk_new RenderModule;
	if (_renderModule->initialize(hwnd, width, height) == false) return false;
#pragma endregion

	// Camera 정보가 필요하기 때문에 이 곳에서 SceneConstantBuffer를 생성합니다.
	// #todo- 어차피 매프레임 업데이트하기 때문에 굳이 이렇게 할 필요는 없음.. PreRender직후 바로 업데이트하기 때문
#pragma region SceneRenderer
	_sceneRenderer = dk_new SceneRenderer;
	_sceneRenderer->createSceneConstantBuffer();	// renderModule과 Camera가 필요하기 때문에 후에 업데이트
#pragma endregion

#pragma region Core Module
	_textureManager = dk_new TextureManager;
	_resourceManager = dk_new ResourceManager;
	_sceneObjectManager = dk_new SceneObjectManager;
#pragma endregion

#pragma region Editor Modules
#if defined(_DK_DEBUG_)
	// 현재 Editor는 사용하지 않아 아무것도 하지 않습니다.
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
	// #todo- 오클루전 컬링?

	// #todo- _renderModule과 _sceneRenderer의 확실한 기능을 구분해야함
	// _renderModule은 Graphics API를 사용하는 용도
	// _sceneRenderer는 renderModule을 이용하여 Rendering하는 용도
	// 따라서 이곳은 sceneRenderer->PreRender가 되어야할듯
	_renderModule->PreRender();

	// SceneConstantBufer Upload
	_sceneRenderer->uploadSceneConstantBuffer();

	// #todo- Render StaticMesh Object	// 현재는 Plane이 임시로 SkinnedMesh로 구현되어있기 때문에 이렇게 해두었습니다.
	{
		SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
		plane._worldTransform.ToMatrix4x4(sceneObjectConstantBufferData._worldMatrix);
		sceneObjectConstantBufferData._worldMatrix.Transpose();
		uint8* sceneObjectConstantBuffer = plane._sceneObjectConstantBuffer->Map();
		memcpy(sceneObjectConstantBuffer, &sceneObjectConstantBufferData, sizeof(sceneObjectConstantBufferData));

		uint32 componentCount = static_cast<uint32>(plane._components.size());
		for (uint i = 0; i < componentCount; ++i)
		{
			// #todo- component 완전 개편 필요해보임.
			// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
			// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
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
			// #todo- component 완전 개편 필요해보임.
			// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
			// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
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