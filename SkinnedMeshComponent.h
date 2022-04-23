#pragma once

#include "Component.h"

class Model;
class SceneObject;

class SkinnedMeshComponent : public Component
{
public:
	virtual ~SkinnedMeshComponent() override
	{
		// #todo- Model은 ModelContainer에서 관리하기 때문에 SkinnedMeshComponent에서 삭제하지 않습니다. Container Element의 RefCount를 줄여야함

		// 현재 LoadResource를 실패하면 Model을 삭제해야하는데..
		// container에서 들고 있기 때문에 Model의 refcount가 1인 상태여서 삭제가 안됩니다.
		// 문제는 이 상태에서 model의 vertexbuffer, indexbuffer, IResource 등이 삭제가 되지 않는 문제가 있으므로..
		// 꼭 삭제조치할 수 있는 방법을 모색해야합니다.
	}

	dk_inline const Model* GetModel() const noexcept
	{
		return _model.get();
	}
	dk_inline Model* GetModelWritable() const noexcept
	{
		return _model.get();
	}
	dk_inline const Skeleton* GetSkeleton() const noexcept
	{
		return _skeleton.get();
	}
	dk_inline Skeleton* GetSkeletonWritable() const noexcept
	{
		return _skeleton.get();
	}

	bool LoadResource(const char* modelPath, const char* skeletonPath, const char* animationSetPath, const char* modelPropertyPath, SceneObject* sceneObject);

private:
	ModelRef _model;
	SkeletonRef _skeleton;
	AnimationRef _animation;
};