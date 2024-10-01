#include "stdafx.h"
#include "GameModule.h"

#include "DuckingEngine.h"
#include "SceneManager.h"
#include "SceneObjectManager.h"
#include "SceneObject.h"

#include "Material.h"
#include "RenderModule.h"

namespace DK
{
	bool GameModule::initialize()
	{
		// Test Terrain
		DuckingEngine::getInstance().getSceneManagerWritable().loadLevel();
		DuckingEngine::getInstance().getSceneManagerWritable().loadSkyDome();
		DuckingEngine::getInstance().getSceneManagerWritable().loadPostProcess();
		DuckingEngine::getInstance().getSceneManagerWritable().loadGbuffer();


		// Test Object
		SceneObject* testObjectSceneObject = SceneObjectManager::createSceneObject(
			"Object/Model/StaticMeshStandard.dm", 
			"Object/ModelProperty/StaticMeshStandard.xml"
		);
		if (testObjectSceneObject == nullptr)
			return false;
		testObjectSceneObject->set_worldTransform(Transform(float3(0, 0, 5), Quaternion::Identity, float3::Identity));

		// Test Character
		SceneObject* testCharacterSceneObject = SceneObjectManager::createCharacter(
			"Character/Appearance/YBot.xml"
		);
		if (testCharacterSceneObject == nullptr) 
			return false;
		testCharacterSceneObject->set_worldTransform(Transform(float3(0, 0, 5), Quaternion::Identity, float3::Identity));

		return true;
	}
}