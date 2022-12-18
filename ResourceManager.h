#pragma once

namespace DK
{
	class ResourceManager
	{
	public:
		const bool loadSkinnedMesh(const DKString& modelPath, SkinnedMeshModelRef& outModel);
		const bool loadSkeleton(const DKString& skeletonPath, const SkinnedMeshModelRef& model, SkeletonRef& outSkeleton);
		const bool loadAnimation(const DKString& animationPath, SkeletonRef& skeleton, AnimationRef& outAnimation);

	private:
		DKHashMap<DKString, SkinnedMeshModelRef> _modelContainer;
		DKHashMap<DKString, SkeletonRef> _skeletonContainer;
		DKHashMap<DKString, AnimationRef> _animationContainer;
	};
}