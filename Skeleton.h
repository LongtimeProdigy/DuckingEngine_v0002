#pragma once

namespace DK
{
	struct Bone
	{
	public:
		static constexpr uint32 kInvalidBoneIndex = 0xffffffff;

	public:
		Bone(const DKString& boneName, const DKString& parentBoneName, const uint32 parentBoneIndex, const Transform& transform)
#if defined(_DK_DEBUG_)
			: _boneName(boneName)
			, _parentBoneName(parentBoneName)
#endif
			, _parentBoneIndex(parentBoneIndex)
			, _transform(transform)
		{}

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