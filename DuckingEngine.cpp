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
#include "ICommandList.h"
#include "IResource.h"

#pragma region System Modules
#include "InputModule.h"
#include "RenderModule.h"
#include "RenderModuleDX12.h"
#pragma endregion

#pragma region Engine Core Module
#include "ResourceManager.h"
#include "TextureManager.h"
#include "SceneObjectManager.h"

#include "GameModule.h"
#pragma endregion

#include "Camera.h"
#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "IResource.h"

struct CameraConstantBufferStruct
{
	Matrix4x4 cameraWorldMatrix;
	Matrix4x4 cameraProjectionMatrix;
};

const DuckingEngine* DuckingEngine::_duckingEngine = nullptr;
RenderModule* DuckingEngine::_renderModule = nullptr;
TextureManager* DuckingEngine::_textureManager = nullptr;
ResourceManager* DuckingEngine::_resourceManager = nullptr;
SceneObjectManager* DuckingEngine::_sceneObjectManager = nullptr;
GameModule* DuckingEngine::_gameModule = nullptr;

DuckingEngine::~DuckingEngine()
{
	dk_delete _renderModule;

	dk_delete _textureManager;
	dk_delete _resourceManager;
	dk_delete _sceneObjectManager;
	dk_delete _gameModule;
}

static SceneObject plane;
bool DuckingEngine::Initialize(HWND hwnd, int width, int height)
{
#pragma region System Modules
	if (InputModule::InitializePCController() == false)
	{
		DK_ASSERT_LOG(false, "ComputerController 생성에 실패했습니다.");
		return false;
	}
	if (InputModule::InitializeXBOXController(0) == false)
	{
		DK_ASSERT_LOG(false, "XBOXController 생성에 실패했습니다.");
		return false;
	}

	_renderModule = RenderModule::CreateRenderModule(RenderModuleType::DIRECTX12, 3);
	if (_renderModule == nullptr)
	{
		DK_ASSERT_LOG(false, "RenderModule 생성에 실패했습니다.");
		return false;
	}
	if (_renderModule->Initialize(hwnd, width, height) == false)
	{
		DK_ASSERT_LOG(false, "RenderModule 초기화에 실패했습니다.");
		return false;
	}
#pragma endregion

#pragma region MainCamera
	Camera::gMainCamera = dk_new Camera(60.0f, width, height);
	Transform cameraTransform(float3(0, 0, 0), float3(0, 0, 0), float3::Identity);
	Camera::gMainCamera->SetWorldTransform(cameraTransform);

	CameraConstantBufferStruct cameraConstanceBufferData;
	Camera::gMainCamera->GetCameraWorldMatrix(cameraConstanceBufferData.cameraWorldMatrix);
	Camera::gMainCamera->GetCameraProjectionMaterix(cameraConstanceBufferData.cameraProjectionMatrix);
	Camera::gCameraConstantBuffer = _renderModule->CreateUploadBuffer(&cameraConstanceBufferData, sizeof(cameraConstanceBufferData));
#pragma endregion

#pragma region Editor&Game Modules
	_resourceManager = dk_new ResourceManager;

	_sceneObjectManager = dk_new SceneObjectManager;

	_gameModule = dk_new GameModule;
	_gameModule->Initialize();
#pragma endregion

#pragma region Plane Test Model
	SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
	plane._worldTransform.ToMatrix4x4(sceneObjectConstantBufferData._worldMatrix);
	plane._sceneObjectConstantBuffer = _renderModule->CreateUploadBuffer(&sceneObjectConstantBufferData, sizeof(sceneObjectConstantBufferData));
	if (plane._sceneObjectConstantBuffer == nullptr)
	{
		DK_ASSERT_LOG(false, "Test Plane Initialize failed");
	}
	SkinnedMeshComponent* planeMesh = dk_new SkinnedMeshComponent;
	planeMesh->LoadResource("Plane", nullptr, nullptr, nullptr, &plane);

	plane.AddComponent(planeMesh);
#pragma endregion

	return true;
}

void DuckingEngine::Update() const
{
	InputModule::Update();

	Camera::gMainCamera->Update();
}

void DuckingEngine::Render() const
{
	// #todo- 오클루전 컬링?

	// #todo- 나중에는 Renderer를 추가하는 게 조아보임 (DX11, DX12, Vulkan...도 마찬가지자만.. staticmesh, character등등으로도)
	_renderModule->PreRender();

	// #todo- 카메라 Transform 버퍼 업데이트
	CameraConstantBufferStruct cameraConstantBufferData;
	Camera::gMainCamera->GetCameraProjectionMaterix(cameraConstantBufferData.cameraProjectionMatrix);
	Camera::gMainCamera->GetCameraWorldMatrix(cameraConstantBufferData.cameraWorldMatrix);
	cameraConstantBufferData.cameraProjectionMatrix.Transpose();
	cameraConstantBufferData.cameraWorldMatrix.Transpose();
	uint8* cameraConstantBufferAddress = Camera::gMainCamera->gCameraConstantBuffer->Map();
	memcpy(cameraConstantBufferAddress, &cameraConstantBufferData, sizeof(cameraConstantBufferData));

	// #todo- Render StaticMesh Object
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
	const std::unordered_map<uint, SceneObject>& sceneObjects = _sceneObjectManager->GetCharacterSceneObjectContainer();
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

	// render UI
	_renderModule->RenderUI();

	_renderModule->EndRender();
}