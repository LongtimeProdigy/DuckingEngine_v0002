#pragma once

#include "Transform.h"

#include "Matrix4x4.h"

class Animation
{
public:
	struct BoneAnimation
	{
		DKString _boneName;
		DKVector<Matrix4x4> _animation;
	};

	dk_inline void SetBoneAnimations(DKVector<BoneAnimation>& boneAnimations) noexcept
	{
		_boneAnimations = boneAnimations;
	}

	dk_inline const DKVector<BoneAnimation>& GetBoneAnimation() const noexcept
	{
		return _boneAnimations;
	}

	dk_inline float GetAnimationTime() const noexcept
	{
		return _animationTime;
	}

	dk_inline void SetAnimationTime(float animationTime) noexcept
	{
		_animationTime = animationTime;
	}

	dk_inline void SetFrameCount(uint frameCount) noexcept
	{
		_frameCount = frameCount;
	}

	dk_inline uint GetFrameCount() const noexcept
	{
		return _frameCount;
	}

private:
	DKVector<BoneAnimation> _boneAnimations;
	float _animationTime = 0.0f;
	uint _frameCount = 0;
};