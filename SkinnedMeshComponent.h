#pragma once

#include "Matrix4x4.h"

#include "Component.h"

class Model;
class SceneObject;

struct MaterialDefinition;
class Material;

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

		dk_delete _skeletonConstantBuffer;
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

	bool LoadResource(const char* modelPath, const char* skeletonPath, const char* animationSetPath, const char* modelPropertyPath);

	void Update(float deltaTime);

	dk_inline const DKVector<Matrix4x4>& GetCurrentCharacterSpaceBoneAnimation() const noexcept
	{
		return _currentCharacterSpaceBoneAnimation;
	}
	dk_inline const IResource* getSkeletonConstantBuffer() const noexcept
	{
		return _skeletonConstantBuffer;
	}

private:
	ModelRef _model;
	SkeletonRef _skeleton;
	AnimationRef _animation;

	DKVector<Matrix4x4> _currentCharacterSpaceBoneAnimation;
	// for GPU Resource
	IResource* _skeletonConstantBuffer;
};