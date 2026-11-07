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
#if defined(_DK_DEBUG_)
			DKString _boneName;
#endif
			DKVector<float4x4> _animation;
		};

		const bool loadAnimatoin(const DKString& animationPath);

		dk_inline const DKVector<BoneAnimation>& getBoneAnimation() const noexcept
		{
			return _boneAnimations;
		}
		dk_inline uint32 getFrameCount() const noexcept
		{
			return _frameCount;
		}

	private:
		DKVector<BoneAnimation> _boneAnimations;
		uint32 _frameCount = 0;
	};
}
