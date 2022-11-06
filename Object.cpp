#include "stdafx.h"
#include "Object.h"

void Object::SetWorldTransform(const Transform& worldTransform) noexcept
{
	_worldTransform = worldTransform;
}

void Object::AddComponent(Component* component) noexcept
{
	_components.push_back(component);
}