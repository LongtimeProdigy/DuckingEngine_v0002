#pragma once

namespace DK
{
	struct AppearanceRaw
	{
		const std::string _modelPath;
		const std::string _skeletonPath;
		const std::string _animationSetPath;
		const std::string _modelPropertyPath;

		AppearanceRaw(
			const char* modelPath, const char* skeletonPath, const char* animationPath, const char* modelPropertyPath
		) : _modelPath(modelPath), _skeletonPath(skeletonPath), _animationSetPath(animationPath), _modelPropertyPath(modelPropertyPath)
		{}
	};

	struct SceneObjectConstantBufferStruct
	{
		float4x4 _worldMatrix;
	};

#define MAX_BONE_COUNT 55
#define INVALID_TEXTURE_INDEX 0xffffffff
	struct SkeletonConstantBufferStruct
	{
		float4x4 _skeletonMatrixBuffer[MAX_BONE_COUNT];
	};

	class SceneObject;

	class SceneObjectManager
	{
	public:
		static SceneObject* createCharacter(const char* appearancePath);

		void update(float deltaTime);

		dk_inline const DKHashMap<uint32, SceneObject>& getCharacterSceneObjects() const
		{
			return _characterSceneObjectContainer;
		}
		dk_inline DKHashMap<uint32, SceneObject>& getCharacterSceneObjectsWritable()
		{
			return _characterSceneObjectContainer;
		}

	private:
		const AppearanceRawRef loadCharacter_LoadAppearanceFile(const char* appearancePath);

	private:
		DKHashMap<DKString, AppearanceRawRef> _appearanceRawContainers;
		DKHashMap<uint32, SceneObject> _characterSceneObjectContainer;
	};
}