#pragma once

#include "Transform.h"

#include "Component.h"

class Component;

class Object
{
public:
	virtual ~Object() {}

	virtual void Update() = 0;

	void SetWorldTransform(const Transform& worldTransform) noexcept;

	void AddComponent(Component* component) noexcept;

public:
	DKVector<Ptr<Component>> _components;
	Transform _worldTransform;
};