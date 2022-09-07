#pragma once

#include "Transform.h"

struct Bone
{
public:
#if defined(_DK_DEBUG_)
	Bone(const DKString& boneName, const Transform& transform)
		: _boneName(boneName), _transform(transform)
	{}
#else
	Bone(const Transform& transform)
		: _transform(transform)
	{}
#endif
	//Bone(const string& boneName, const Transform& transform, const DKVector<uint>& childs)
	//	: _boneName(boneName), _transform(transform), _childs(std::move(childs))
	//{}

#if defined(_DK_DEBUG_)
	DKString _boneName;
#endif
	Transform _transform = Transform::Identity;
	DKVector<uint> _childs;
};

class Skeleton
{
public:
	dk_inline const DKVector<Bone>& GetBones() const noexcept
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