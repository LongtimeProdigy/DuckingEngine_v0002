#pragma once

#include "Matrix4x4.h"

#include "Component.h"

struct IBuffer;

class Model;
class SceneObject;

struct MaterialDefinition;
class Material;

class SkinnedMeshComponent : public Component
{
public:
	dk_inline const Model* getModel() const noexcept
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

	bool LoadResource(const DKString& modelPath, const DKString& skeletonPath, const DKString& animationSetPath, const DKString& modelPropertyPath);

	virtual void update(float deltaTime);

	dk_inline const DKVector<Matrix4x4>& GetCurrentCharacterSpaceBoneAnimation() const noexcept
	{
		return _currentCharacterSpaceBoneAnimation;
	}
	dk_inline const Ptr<IBuffer>& getSkeletonConstantBuffer() const noexcept
	{
		return _skeletonConstantBuffer;
	}
	dk_inline Ptr<IBuffer>& getSkeletonConstantBufferWritable() noexcept
	{
		return _skeletonConstantBuffer;
	}

private:
	ModelRef _model;
	SkeletonRef _skeleton;
	AnimationRef _animation;

	DKVector<Matrix4x4> _currentCharacterSpaceBoneAnimation;
	Ptr<IBuffer> _skeletonConstantBuffer;
};