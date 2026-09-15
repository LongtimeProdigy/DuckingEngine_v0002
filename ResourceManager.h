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
		const bool loadAnimation(const DKString& animationPath, SkeletonRef& skeleton, AnimationRef& outAnimation);

		// TODO : 임시로 GLTF 로딩때문에 public으로 두었는데 고민후에 private로 옮기자
		DKHashMap<DKString, StaticMeshModelRef> _staticMeshModelContainer;

	private:
		DKHashMap<DKString, ModelPropertyRef> _modelPropertyContainer;
		DKHashMap<DKString, SkinnedMeshModelRef> _skinnedMeshModelContainer;
		DKHashMap<DKString, SkeletonRef> _skeletonContainer;
		DKHashMap<DKString, AnimationRef> _animationContainer;
	};
}
