#include "stdafx.h"
#include "GameModule.h"

#include "SceneObjectManager.h"
#include "SceneObject.h"

namespace DK
{
	bool GameModule::initialize()
	{
		// Test Object
		//SceneObject* testObjectSceneObject = SceneObjectManager::createSceneObject(
		//	"C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/Object/Model/StaticMeshStandard.dm", 
		//	"C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/Object/ModelProperty/StaticMeshStandard.xml"
		//);
		//if (testObjectSceneObject == nullptr)
		//	return false;
		//testObjectSceneObject->set_worldTransform(Transform(float3::Zero, Quaternion::Identity, float3::Identity));

		// Test Character
		SceneObject* testCharacterSceneObject = SceneObjectManager::createCharacter(
			"C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/Character/Appearance/YBot.xml"
		);
		if (testCharacterSceneObject == nullptr) 
			return false;
		testCharacterSceneObject->set_worldTransform(Transform(float3::Zero, Quaternion::Identity, float3::Identity));

		return true;
	}
}