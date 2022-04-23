#include "stdafx.h"
#include "GameModule.h"

#include "Transform.h"

#include "DuckingEngine.h"

#include "SceneObjectManager.h"

#include "SceneObject.h"

bool GameModule::Initialize()
{
#pragma region Load MapData
	struct MapData
	{
		struct TerrainData
		{

		};
		struct SceneObjectData
		{
			SceneObjectData() = delete;
			SceneObjectData(const Transform&& worldTransform)
				: _worldTransform(worldTransform)
			{}

			Transform _worldTransform;
		};

		MapData(const TerrainData&& terrainData, const SceneObjectData&& sceneObjectData)
			: _terrainData(terrainData)
			, _sceneObjectData(sceneObjectData)
		{}

		TerrainData _terrainData;
		SceneObjectData _sceneObjectData;
	};

	MapData mapData(
		MapData::TerrainData(), MapData::SceneObjectData(Transform(float3::Zero, float3::Zero, float3::Identity))
	);

	// Static Object
	{
	}

	// Test Character
	{
		SceneObjectManager* sceneObjectManager = DuckingEngine::_duckingEngine->GetSceneObjectManagerWritable();
		SceneObject* characterObject = sceneObjectManager->CreateCharacter(
			"C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/Character/Appearance/ganfaul_m_aure.xml"
		);
		CHECK_BOOL_AND_RETURN(characterObject != nullptr);

		characterObject->SetWorldTransform(mapData._sceneObjectData._worldTransform);
	}
#pragma endregion

	return true;
}