#include "stdafx.h"
#include "Object.h"

#include "Component.h"

Object::~Object()
{
	if (_components.size() != 0)
	{
		dk_delete_array& _components[0];
		_components.clear();
	}

	if (_sceneObjectConstantBuffer != nullptr)
	{
		dk_delete _sceneObjectConstantBuffer;
	}
}

void Object::SetWorldTransform(const Transform& worldTransform) noexcept
{
	_worldTransform = worldTransform;
}

void Object::AddComponent(Component&& component) noexcept
{
	_components.push_back(component);
}