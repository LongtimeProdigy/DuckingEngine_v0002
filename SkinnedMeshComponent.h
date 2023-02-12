#pragma once
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
		DK_REFLECTION_PROPERTY(DKString, _animationPath);

		DK_REFLECTION_PROPERTY(SkinnedMeshModelRef, _model);
		SkeletonRef _skeleton;
		AnimationRef _animation;

		DK_REFLECTION_VECTOR_PROPERTY(float4x4, _currentCharacterSpaceBoneAnimation);
		DK_REFLECTION_PTR_PROPERTY(IBuffer, _skeletonConstantBuffer);
	};
}