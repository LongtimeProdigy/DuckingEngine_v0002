#pragma once

namespace DK
{
	struct AppearanceData
	{
		struct ModelData
		{
			const DKString _modelPath;
			const DKString _modelPropertyPath;

			ModelData(const DKString& modelPath, const DKString& modelPropertyPath)
				: _modelPath(modelPath), _modelPropertyPath(modelPropertyPath)
			{}
		};

		const DKString _skeletonPath;
		const DKString _animationSetPath;
		DKVector<ModelData> _modelDataArr;

		AppearanceData(const DKString& skeletonPath, const DKString& animationSetPath, const DKVector<ModelData>&& modelDataArr)
			: _skeletonPath(skeletonPath), _animationSetPath(animationSetPath), _modelDataArr(DK::move(modelDataArr))
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
		static SceneObject* createSceneObject(const DKString& modelPath, const DKString& modelPropertyPath);
		static SceneObject* createCharacter(const char* appearancePath);

		void update(float deltaTime);

		dk_inline const DKHashMap<uint32, SceneObject>& getSceneObjects() const
		{
			return _sceneObjectContainer;
		}
		dk_inline DKHashMap<uint32, SceneObject>& getSceneObjectsWritable()
		{
			return _sceneObjectContainer;
		}

		dk_inline const DKHashMap<uint32, SceneObject>& getCharacterSceneObjects() const
		{
			return _characterSceneObjectContainer;
		}
		dk_inline DKHashMap<uint32, SceneObject>& getCharacterSceneObjectsWritable()
		{
			return _characterSceneObjectContainer;
		}

	private:
		const AppearanceDataRef loadCharacter_LoadAppearanceFile(const char* appearancePath);

	private:
		DKHashMap<DKString, AppearanceDataRef> _appearanceRawContainers;
		DKHashMap<uint32, SceneObject> _sceneObjectContainer;
		DKHashMap<uint32, SceneObject> _characterSceneObjectContainer;
	};
}