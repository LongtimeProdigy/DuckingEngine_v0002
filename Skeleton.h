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
		Bone(const char* boneName, const char* parentBoneName, const uint32 parentBoneIndex, const Transform& transform)
			: _boneNameHash(StringUtil::registerHash(boneName))
			, _parentBoneNameHash(StringUtil::registerHash(parentBoneName))
			, _parentBoneIndex(parentBoneIndex)
			, _transform(transform)
		{
#if defined(_DK_DEBUG_)
			_boneName = boneName;
			_parentBoneName = parentBoneName;
#endif
		}

		uint64 _boneNameHash;
		uint64 _parentBoneNameHash;
#if defined(_DK_DEBUG_)
		DKString _boneName;
		DKString _parentBoneName;
#endif
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
