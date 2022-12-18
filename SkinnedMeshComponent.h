#pragma once
#include "Component.h"

namespace DK
{
	struct IBuffer;
}

namespace DK
{
	class SkinnedMeshComponent : public Component
	{
	public:
		bool LoadResource(const DKString& modelPath, const DKString& skeletonPath, const DKString& animationSetPath, const DKString& modelPropertyPath);

		virtual void update(float deltaTime) override final;

	private:
		DK_REFLECTION_DECLARE(SkinnedMeshModelRef, _model);
		SkeletonRef _skeleton;
		AnimationRef _animation;

		DK_REFLECTION_VECTOR_DECLARE(float4x4, _currentCharacterSpaceBoneAnimation);
		DK_REFLECTION_PTR_DECLARE(IBuffer, _skeletonConstantBuffer);
	};
}