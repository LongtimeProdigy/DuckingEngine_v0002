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

		AppearanceData(const DKString& skeletonPath, const DKString& animationSetPath, DKVector<ModelData>&& modelDataArr)
			: _skeletonPath(skeletonPath), _animationSetPath(animationSetPath), _modelDataArr(DK::move(modelDataArr))
		{}
	};

	struct SceneObjectConstantBufferStruct
	{
		float4x4 _worldMatrix;
	};

	class SceneObject;

	class SceneObjectManager
	{
	public:
		SceneObject* loadGLTF(const char* path);

		SceneObject* createStaticMeshSceneObject(const DKString& modelPath, const DKString& modelPropertyPath);
		SceneObject* createCharacter(const char* appearancePath);

		void update(float deltaTime);

		dk_inline const DKVector<SceneObject>& getSceneObjects() const
		{
			return _sceneObjectContainer;
		}
		dk_inline const DKVector<SceneObject>& getCharacterSceneObjects() const
		{
			return _characterSceneObjectContainer;
		}
	private:
		SceneObject* publish(SceneObject&& object, DKVector<SceneObject>& objects);
		const AppearanceDataRef loadCharacter_LoadAppearanceFile(const char* appearancePath);

	private:
		DKHashMap<DKString, AppearanceDataRef> _appearanceRawContainers;
		DKVector<SceneObject> _sceneObjectContainer;
		DKVector<SceneObject> _characterSceneObjectContainer;
	};
}
