#include "stdafx.h"
#include "GameModule.h"

#include "Transform.h"

#include "SceneObjectManager.h"
#include "SceneObject.h"

// TestPlane
static SceneObject plane;

bool GameModule::Initialize()
{
#pragma region Load MapData
	// Test Character
	SceneObject* characterObject = SceneObjectManager::CreateCharacter(
		"C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/Character/Appearance/ganfaul_m_aure.xml"
	);
	if (characterObject == nullptr) return false;
	characterObject->SetWorldTransform(Transform(float3::Zero, float3(0, 0, 0), float3::Identity));
#pragma endregion

	return true;
}