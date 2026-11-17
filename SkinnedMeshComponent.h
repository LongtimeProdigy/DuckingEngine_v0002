#pragma once
#include "Resource/Material/ShaderStructures.h"
#include "RenderableComponent.h"

namespace DK
{
	struct IBuffer;
}

namespace DK
{
	class SkinnedMeshComponent : public RenderableComponent
	{
	public:
		virtual bool loadResource() override final;

		virtual void update(float deltaTime) override final;

	private:
		DK_REFLECTION_PROPERTY(DKString, _skeletonPath);

		SkinnedMeshModelRef _model;

		SkeletonRef _skeleton;

		DKString _animationPath;
		AnimationRef _animation;
		double _animationTime = 0.0;
		DKVector<uint32> _trackIndices;
		DKVector<float4x4> _inverseBindPose;
		DKVector<float4x4> _animatedPose;

		SkeletonConstantBuffer _skeletonConstantBufferData;
		IBufferRef _currentAnimationBufferCS;
	};
}
