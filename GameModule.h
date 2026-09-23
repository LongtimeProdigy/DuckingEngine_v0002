#pragma once

namespace DK
{
	class SceneObject;

	class GameModule
	{
	public:
		bool initialize();

	private:
		SceneObject* _sponza = nullptr;
		SceneObject* _testObjectSceneObject = nullptr;
		SceneObject* _testCharacterSceneObject = nullptr;
	};
}
