#pragma once
#include "Resource/Material/SkinningLimits.h"

namespace DK
{
	struct Bone
	{
	public:
		static constexpr uint32 kInvalidBoneIndex = 0xffffffff;
		static constexpr uint32 kMaxBoneCount = DK_MAX_BONE_COUNT;

	public:
		Bone(const DKString& boneName, const DKString& parentBoneName, const uint32 parentBoneIndex, const Transform& transform)
			: _boneName(boneName)
			, _parentBoneName(parentBoneName)
			, _parentBoneIndex(parentBoneIndex)
			, _transform(transform)
		{}

		DKString _boneName;
		DKString _parentBoneName;
		uint32 _parentBoneIndex;
		Transform _transform = Transform::Identity;
	};

	class Skeleton
	{
	public:
		dk_inline const DKVector<Bone>& getBoneArr() const noexcept
		{
			return _bones;
		}
		dk_inline void  SetBones(DKVector<Bone>& bones) noexcept
		{
			_bones.swap(bones);
		}

	private:
		DKVector<Bone> _bones;
	};
}