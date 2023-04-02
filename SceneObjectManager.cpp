#include "stdafx.h"
#include "SceneObjectManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

#include "SceneObject.h"
#include "StaticMeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "Skeleton.h"

namespace DK
{
	bool createSceneObjectConstantBuffer(SceneObject& sceneObject)
	{
		// for GPU resource
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
		sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
		sceneObject._sceneObjectConstantBuffer = renderModule.createUploadBuffer(sizeof(sceneObjectConstantBufferData), L"SceneObject_Cbuffer");
		if (sceneObject._sceneObjectConstantBuffer.get() == nullptr)
		{
			DK_ASSERT_LOG(false, "SceneObjectConstantBuffer 생성에 실패");
			return false;
		}

		return true;
	}
	const AppearanceDataRef SceneObjectManager::loadCharacter_LoadAppearanceFile(const char* appearancePath)
	{
		using FindResult = DKHashMap<DKString, AppearanceDataRef>::iterator;
		FindResult findResult = _appearanceRawContainers.find(appearancePath);
		if (findResult != _appearanceRawContainers.end())
			return findResult->second;

		ScopeString<DK_MAX_PATH> appearanceFullPath = GlobalPath::makeResourceFullPath(appearancePath);

		TiXmlDocument appearanceDocument;
		if (appearanceDocument.LoadFile(appearanceFullPath.c_str()) == false)
		{
			DK_ASSERT_LOG(false, "Appearance File LoadError: %s", appearanceDocument.ErrorDesc());
			return nullptr;
		}

		TiXmlNode* rootNode = appearanceDocument.RootElement();
		TiXmlNode* skeletonNode = rootNode->FirstChild("Skeleton");
		const DKString skeletonPath = skeletonNode->ToElement()->GetText();
		TiXmlNode* animationSetNode = rootNode->FirstChild("AnimationSet");
		const DKString animationSetPath = animationSetNode->ToElement()->GetText();

		DKVector<AppearanceData::ModelData> modelDataArr;
		TiXmlNode* modelDataArrNode = rootNode->FirstChild("ModelDataArr");
		const int modelDataCount = DK::atoi(modelDataArrNode->ToElement()->Attribute("Count"));
		modelDataArr.reserve(modelDataCount);
		for (TiXmlNode* modelDataNode = modelDataArrNode->FirstChild(); modelDataNode != nullptr; modelDataNode = modelDataNode->NextSiblingElement())
		{
			const TiXmlElement* modelNode = modelDataNode->FirstChildElement("Model");
			const char* modelPath = modelNode->GetText();
			const TiXmlElement* modelPropertyNode = modelDataNode->FirstChildElement("ModelProperty");
			const char* modelPropertyPath = modelPropertyNode->GetText();

			modelDataArr.push_back(AppearanceData::ModelData(modelPath, modelPropertyPath));
		}
		
		auto insertResult = _appearanceRawContainers.insert(
			DKPair<const char*, AppearanceDataRef>(appearancePath, dk_new AppearanceData(skeletonPath, animationSetPath, DK::move(modelDataArr)))
		);

		return insertResult.first->second;
	}
	SceneObject* SceneObjectManager::createSceneObject(const DKString& modelPath, const DKString& modelPropertyPath)
	{
		SceneObjectManager& thisXXX = DuckingEngine::getInstance().GetSceneObjectManagerWritable();

		SceneObject newSceneObject;
		StaticMeshComponent* staticMeshComponent = dk_new StaticMeshComponent;
		newSceneObject.addComponent(staticMeshComponent);
		staticMeshComponent->set_modelPath(modelPath);
		staticMeshComponent->set_modelPropertyPath(modelPropertyPath);
		if (staticMeshComponent->loadResource() == false)
			return nullptr;	

		if (createSceneObjectConstantBuffer(newSceneObject) == false)
			return nullptr;

		const uint32 key = static_cast<uint32>(thisXXX._sceneObjectContainer.size());
		auto success = thisXXX._sceneObjectContainer.insert(DKPair<uint32, SceneObject>(key, DK::move(newSceneObject)));
		if (success.second == false)
			return nullptr;

		return &success.first->second;
	}
	SceneObject* SceneObjectManager::createCharacter(const char* appearancePath)
	{
		SceneObjectManager& thisXXX = DuckingEngine::getInstance().GetSceneObjectManagerWritable();

		const AppearanceDataRef& appearanceData = thisXXX.loadCharacter_LoadAppearanceFile(appearancePath);
		if (appearanceData == nullptr)
			return nullptr;

		SceneObject newSceneObject;
		const uint32 skinnedMeshCount = static_cast<const uint32>(appearanceData->_modelDataArr.size());
		for (uint32 i = 0; i < skinnedMeshCount; ++i)
		{
			SkinnedMeshComponent* skinnedMeshComponent = dk_new SkinnedMeshComponent;
			newSceneObject.addComponent(skinnedMeshComponent);
			skinnedMeshComponent->set_modelPath(appearanceData->_modelDataArr[i]._modelPath);
			skinnedMeshComponent->set_modelPropertyPath(appearanceData->_modelDataArr[i]._modelPropertyPath);
			if (i == 0)	// MainSkinnedMesh
			{
				skinnedMeshComponent->set_skeletonPath(appearanceData->_skeletonPath);
				skinnedMeshComponent->set_animationPath(appearanceData->_animationSetPath);	// #todo- 나중에 AnimationSet으로 변경(AnimationController 만든 후에)
			}
			if (skinnedMeshComponent->loadResource() == false)
				return nullptr;
		}

		if (createSceneObjectConstantBuffer(newSceneObject) == false)
			return nullptr;

		const uint32 key = static_cast<uint32>(thisXXX._characterSceneObjectContainer.size());
		auto success = thisXXX._characterSceneObjectContainer.insert(DKPair<uint32, SceneObject>(key, DK::move(newSceneObject)));
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
				sceneObject._components[i]->update(deltaTime);
		}
	}
}