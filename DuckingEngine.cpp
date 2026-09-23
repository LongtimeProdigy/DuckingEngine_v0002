#include "stdafx.h"
#include "DuckingEngine.h"

#include "InputModule.h"
#include "RenderModule.h"
#include "RaytracingRenderer.h"
#include "SceneRenderer.h"

#include "ResourceManager.h"
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
	Ptr<DuckingEngine> DuckingEngine::_duckingEngine = nullptr;

	DuckingEngine& DuckingEngine::getInstance()
	{
		if (_duckingEngine.get() == nullptr)
			_duckingEngine = dk_new DuckingEngine;

		return *_duckingEngine.get();
	}

	void DuckingEngine::destroy()
	{
		_renderModule->waitAllGPU();

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

#if defined(_DK_DEBUG_)
		EditorDebugDrawManager::getSingleton().get_primitiveInfoSphereArrWritable().clear();
		EditorDebugDrawManager::getSingleton().get_primitiveInfoSphereBufferWritable().reset();
		EditorDebugDrawManager::getSingleton().get_primitiveInfoLineArrWritable().clear();
		EditorDebugDrawManager::getSingleton().get_primitiveInfoLineBufferWritable().reset();
		EditorDebugDrawManager::SpherePrimitiveInfo::kVertexBuffer.reset();
		EditorDebugDrawManager::SpherePrimitiveInfo::kIndexBuffer.reset();
		EditorDebugDrawManager::LinePrimitiveInfo::kVertexBuffer.reset();
		EditorDebugDrawManager::LinePrimitiveInfo::kIndexBuffer.reset();
#endif

		_gameModule.release();
		_sceneObjectManager.release();
		_sceneManager.release();
		_resourceManager.release();
		_sceneRenderer.release();
		_raytracingRenderer.release();
		_renderModule->destroy();
		_renderModule.release();

		_duckingEngine.release();
	}

	DuckingEngine::DuckingEngine()
	{
		if (_duckingEngine.get() != nullptr)
		{
			DK_ASSERT_LOG(false, "Engine을 2개 생성을 시도하고 있습니다. 반드시 검토 바랍니다.");
		}
	}

	DuckingEngine::~DuckingEngine()
	{
	}

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
		Transform cameraTransform(float3(0, 2, -10), Quaternion::Identity, float3::Identity);
		Camera::gMainCamera->set_worldTransform(cameraTransform);

		// Camera 정보가 필요하기 때문에 이 곳에서 SceneConstantBuffer를 생성합니다.
		// #todo- 어차피 매프레임 업데이트하기 때문에 굳이 이렇게 할 필요는 없음.. PreRender직후 바로 업데이트하기 때문
		_sceneRenderer = dk_new SceneRenderer;
		if (_sceneRenderer->initialize() == false) 
			return false;

		_raytracingRenderer = dk_new RaytracingRenderer;
		if (_raytracingRenderer->initialize(_renderModule.get(), width, height) == false)
			return false;

		_resourceManager = dk_new ResourceManager;

		_sceneManager = dk_new SceneManager;
		_sceneObjectManager = dk_new SceneObjectManager;

#if defined(_DK_DEBUG_)
		EditorDebugDrawManager::getSingleton().initialize();
#endif

		_gameModule = dk_new GameModule;
		if (_gameModule->initialize() == false)
			return false;

		return true;
	}

	void DuckingEngine::Update(const float deltaTime)
	{
		InputModule::Update();
		Camera::gMainCamera->update(deltaTime);
		_sceneObjectManager->update(deltaTime);
	}

	void DuckingEngine::Render(const float deltaTime)
	{
		_sceneRenderer->prepareShaderData(deltaTime);

		_sceneRenderer->preRender();
		_sceneRenderer->updateRender();
		_sceneRenderer->endRender();
	}
}
