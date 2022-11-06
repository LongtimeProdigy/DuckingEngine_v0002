#include "stdafx.h"
#include "SceneObjectManager.h"

#include "Matrix4x4.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Skeleton.h"

const AppearanceRawRef SceneObjectManager::loadCharacter_LoadAppearanceFile(const char* appearancePath)
{
	using FindResult = DKHashMap<DKString, AppearanceRawRef>::iterator;
	FindResult findResult = _appearanceRawContainers.find(appearancePath);
	if (findResult != _appearanceRawContainers.end())
	{
		return findResult->second;
	}

#if defined(USE_TINYXML)
	TiXmlDocument ReadDoc;
	ReadDoc.LoadFile(appearancePath);

	TiXmlElement* ReadRoot = ReadDoc.FirstChildElement("Appearance");
	TiXmlElement* modelNode = ReadRoot->FirstChildElement("Model");
	const char* modelPath = modelNode->GetText();
	TiXmlElement* skeletonNode = ReadRoot->FirstChildElement("Skeleton");
	const char* skeletonPath = skeletonNode->GetText();
	TiXmlElement* animationSetNode = ReadRoot->FirstChildElement("AnimationSet");
	const char* animationPath = animationSetNode->GetText();
	TiXmlElement* modelPropertyNode = ReadRoot->FirstChildElement("ModelProperty");
	const char* modelPropertyPath = modelPropertyNode->GetText();
#else
	static_assert("아직 지원되는 XML Loader가 없습니다.");
#endif

	auto insertResult = _appearanceRawContainers.insert(
		DKPair<const char*, AppearanceRawRef>(appearancePath, dk_new AppearanceRaw(modelPath, skeletonPath, animationPath, modelPropertyPath))
	);

	return insertResult.first->second;
}
SceneObject* SceneObjectManager::createCharacter(const char* appearancePath)
{
	SceneObject newSceneObject;

	SceneObjectManager& thisXXX = DuckingEngine::getInstance().GetSceneObjectManagerWritable();
	const AppearanceRawRef appearanceRaw = thisXXX.loadCharacter_LoadAppearanceFile(appearancePath);
	SkinnedMeshComponent* mainSkinnedMeshComponent = dk_new SkinnedMeshComponent;
	if (mainSkinnedMeshComponent->LoadResource(
		appearanceRaw->_modelPath.c_str(), appearanceRaw->_skeletonPath.c_str(), 
		appearanceRaw->_animationSetPath.c_str(), appearanceRaw->_modelPropertyPath.c_str()
	) == false)
	{
		DK_ASSERT_LOG(false, "SkinnedMeshComponent::LoadResource에 실패");
		return nullptr;
	}
	newSceneObject.AddComponent(mainSkinnedMeshComponent);

	// for GPU resource
	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
	newSceneObject._worldTransform.ToMatrix4x4(sceneObjectConstantBufferData._worldMatrix);
	newSceneObject._sceneObjectConstantBuffer = renderModule.createUploadBuffer(&sceneObjectConstantBufferData, sizeof(sceneObjectConstantBufferData));
	if (newSceneObject._sceneObjectConstantBuffer.get() == nullptr)
	{
		DK_ASSERT_LOG(false, "SceneObjectConstantBuffer 생성에 실패");
		return nullptr;
	}

	const uint32 key = static_cast<uint32>(thisXXX._characterSceneObjectContainer.size());
	auto success = thisXXX._characterSceneObjectContainer.insert(DKPair<uint32, SceneObject>(key, std::move(newSceneObject)));
	if (success.second == false)
		return nullptr;

	return &success.first->second;
}

void SceneObjectManager::update(float deltaTime)
{
	for (auto iter = _characterSceneObjectContainer.begin(); iter != _characterSceneObjectContainer.end(); ++iter)
	{
		SceneObject& sceneObject = iter->second;
		uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
		for (uint32 i = 0; i < componentCount; ++i)
		{
			sceneObject._components[i]->update(deltaTime);
		}
	}
}