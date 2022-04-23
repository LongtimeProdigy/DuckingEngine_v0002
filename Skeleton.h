#pragma once

#include "Transform.h"

struct Bone
{
public:
	Bone(const std::string& boneName, const Transform& transform, const std::vector<uint>& childs)
		: _boneName(boneName), _transform(transform), _childs(std::move(childs))
	{}

#if defined(DK_DEBUG)
	std::string _boneName;
#endif
	Transform _transform = Transform::Identity;
	std::vector<uint> _childs;
};

class Skeleton
{
public:
	dk_inline const std::vector<Bone>& GetBones() const noexcept
	{
		return _bones;
	}
	dk_inline void  SetBones(std::vector<Bone>& bones) noexcept
	{
		_bones.swap(bones);
	}

private:
	std::vector<Bone> _bones;
};