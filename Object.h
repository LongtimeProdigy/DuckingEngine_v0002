#pragma once

#include "Transform.h"

class Component;

class Object
{
public:
	virtual ~Object();

	virtual void Update() = 0;

	void SetWorldTransform(const Transform& worldTransform) noexcept;

	void AddComponent(Component&& component) noexcept;

public:
	DKVector<Component> _components;
	Transform _worldTransform;
};