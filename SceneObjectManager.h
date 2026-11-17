#pragma once

#include "Resource/Material/ShaderStructures.h"

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

	struct StaticMeshInstanceChunk
	{
		float3 _minimum = float3(FLT_MAX, FLT_MAX, FLT_MAX);
		float3 _maximum = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		uint32 _firstInstance = 0;
		uint32 _instanceCount = 0;
	};

	struct StaticMeshInstanceBatch
	{
		StaticMeshModelRef _model;
		IBufferRef _staticMeshInstances;
		DKVector<StaticMeshInstance> _instances;
		DKVector<StaticMeshInstanceChunk> _chunks;
	};

	class SceneObject;

	class SceneObjectManager
	{
	public:
		SceneObject* loadGLTF(const char* path);
		bool loadStaticMeshPlacements(const char* path);

		SceneObject* createStaticMeshSceneObject(const DKString& modelPath, const DKString& modelPropertyPath);
		SceneObject* createCharacter(const char* appearancePath);

		void update(float deltaTime);

		dk_inline const DKVector<StaticMeshInstanceBatch>& getStaticMeshInstanceBatches() const
		{
			return _staticMeshInstanceBatches;
		}

		dk_inline const DKVector<SceneObject>& getSceneObjects() const
		{
			return _sceneObjectContainer;
		}
		dk_inline const DKVector<SceneObject>& getCharacterSceneObjects() const
		{
			return _characterSceneObjectContainer;
		}
	private:
		bool loadStaticMeshModels(const char* path, DKVector<StaticMeshModelRef>& models);
		bool createStaticMeshInstances(const DKVector<StaticMeshModelRef>& models, const DKVector<Transform>& transforms);
		SceneObject* publish(SceneObject&& object, DKVector<SceneObject>& objects);
		const AppearanceDataRef loadCharacter_LoadAppearanceFile(const char* appearancePath);

	private:
		DKHashMap<uint64, AppearanceDataRef> _appearanceRawContainers;
		DKVector<SceneObject> _sceneObjectContainer;
		DKVector<SceneObject> _characterSceneObjectContainer;
		DKVector<StaticMeshInstanceBatch> _staticMeshInstanceBatches;
	};
}
