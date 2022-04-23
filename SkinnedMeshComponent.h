#pragma once

#include "Component.h"

class Model;
class SceneObject;

class SkinnedMeshComponent : public Component
{
public:
	virtual ~SkinnedMeshComponent() override
	{
		// #todo- Model�� ModelContainer���� �����ϱ� ������ SkinnedMeshComponent���� �������� �ʽ��ϴ�. Container Element�� RefCount�� �ٿ�����

		// ���� LoadResource�� �����ϸ� Model�� �����ؾ��ϴµ�..
		// container���� ��� �ֱ� ������ Model�� refcount�� 1�� ���¿��� ������ �ȵ˴ϴ�.
		// ������ �� ���¿��� model�� vertexbuffer, indexbuffer, IResource ���� ������ ���� �ʴ� ������ �����Ƿ�..
		// �� ������ġ�� �� �ִ� ����� ����ؾ��մϴ�.
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