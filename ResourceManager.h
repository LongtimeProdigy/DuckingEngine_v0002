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
		DKHashMap<uint64, StaticMeshModelRef> _staticMeshModelContainer;

		DKHashMap<uint64, ModelPropertyRef> _modelPropertyContainer;
		DKHashMap<uint64, SkinnedMeshModelRef> _skinnedMeshModelContainer;
		DKHashMap<uint64, SkeletonRef> _skeletonContainer;
		DKHashMap<uint64, AnimationRef> _animationContainer;
	};
}
