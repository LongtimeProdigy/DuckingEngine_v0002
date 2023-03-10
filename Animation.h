#pragma once

namespace DK
{
	class Animation
	{
	public:
		static constexpr uint32 kAnimationFrameCountPerSecond = 30;
		static constexpr float kAnimationTimePerFrame = 1.0f / kAnimationFrameCountPerSecond;

	public:
		struct BoneAnimation
		{
			DKString _boneName;
			DKVector<float4x4> _animation;
		};

		dk_inline void SetBoneAnimations(DKVector<BoneAnimation>& boneAnimations) noexcept
		{
			_boneAnimations = boneAnimations;
		}

		dk_inline const DKVector<BoneAnimation>& getBoneAnimation() const noexcept
		{
			return _boneAnimations;
		}

		dk_inline float getCurrentAnimationTime() const noexcept
		{
			return _currentAnimationTime;
		}

		dk_inline void setCurrentAnimationTime(float animationTime) noexcept
		{
			_currentAnimationTime = animationTime;
		}

		dk_inline void SetFrameCount(uint frameCount) noexcept
		{
			_frameCount = frameCount;
		}

		dk_inline uint getFrameCount() const noexcept
		{
			return _frameCount;
		}

	private:
		DKVector<BoneAnimation> _boneAnimations;
		float _currentAnimationTime = 0.0f;
		uint _frameCount = 0;
	};
}