#pragma once

namespace DK
{
	class ResourceManager
	{
	public:
		const ModelPropertyRef loadModelProperty(const DKString& modelPropertyPath);

		const StaticMeshModelRef loadStaticMesh(const DKString& modelPath, const ModelPropertyRef& modelProperty);

		const SkinnedMeshModelRef loadSkinnedMesh(const DKString& modelPath, const ModelPropertyRef& modelProperty);
		const bool loadSkeleton(const DKString& skeletonPath, const SkinnedMeshModelRef& model, SkeletonRef& outSkeleton);
		const bool loadAnimation(const DKString& animationPath, AnimationRef& outAnimation);

	private:
		DKHashMap<DKString, StaticMeshModelRef> _staticMeshModelContainer;

		DKHashMap<DKString, ModelPropertyRef> _modelPropertyContainer;
		DKHashMap<DKString, SkinnedMeshModelRef> _skinnedMeshModelContainer;
		DKHashMap<DKString, SkeletonRef> _skeletonContainer;
		DKHashMap<DKString, AnimationRef> _animationContainer;
	};
}
