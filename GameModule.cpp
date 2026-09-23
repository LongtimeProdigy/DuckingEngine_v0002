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
		DuckingEngine::getInstance().getSceneManagerWritable().loadOcean();
		DuckingEngine::getInstance().getSceneManagerWritable().loadLevel();
		DuckingEngine::getInstance().getSceneManagerWritable().loadSkyDome();
		DuckingEngine::getInstance().getSceneManagerWritable().loadPostProcess();
		DuckingEngine::getInstance().getSceneManagerWritable().loadGbuffer();

		_sponza = DuckingEngine::getInstance().GetSceneObjectManagerWritable().loadGLTF("Resource/Object/Sponza/glTF/Sponza.gltf");
		if (_sponza == nullptr)
			return false;
		_sponza->set_worldTransform(Transform(float3(0, 0, 0), Quaternion::Identity, float3(0.05f, 0.05f, 0.05f)));

		// Test Object
		_testObjectSceneObject = DuckingEngine::getInstance().GetSceneObjectManagerWritable().createSceneObject(
			"Object/Model/StaticMeshStandard.dm", 
			"Object/ModelProperty/StaticMeshStandard.xml"
		);
		if (_testObjectSceneObject == nullptr)
			return false;
		_testObjectSceneObject->set_worldTransform(Transform(float3(0, 0, 5), Quaternion::Identity, float3::Identity));

		// Test Character
		_testCharacterSceneObject = DuckingEngine::getInstance().GetSceneObjectManagerWritable().createCharacter(
			"Character/Appearance/YBot.xml"
		);
		if (_testCharacterSceneObject == nullptr) 
			return false;
		_testCharacterSceneObject->set_worldTransform(Transform(float3(0, 0, 5), Quaternion::Identity, float3::Identity));

		return true;
	}
}
