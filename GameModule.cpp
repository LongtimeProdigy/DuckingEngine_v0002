#include "stdafx.h"
#include "GameModule.h"

#include "SceneObjectManager.h"
#include "SceneObject.h"

namespace DK
{
	// TestPlane
	static SceneObject plane;

	bool GameModule::initialize()
	{
		// Test Character
		SceneObject* characterObject = SceneObjectManager::createCharacter(
			"C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/Character/Appearance/YBot.xml"
		);
		if (characterObject == nullptr) return false;
		characterObject->set_worldTransform(Transform(float3::Zero, Quaternion::Identity, float3::Identity));

		return true;
	}
}