#pragma once

#include "Transform.h"

class Component;
class IResource;

class Object
{
public:
	Object();
	virtual ~Object();

	virtual void Update() = 0;

	void SetWorldTransform(const Transform& worldTransform) noexcept;

	void AddComponent(Component* component) noexcept;

public:
	std::vector<Component*> _components;
	Transform _worldTransform;
	IResource* _sceneObjectConstantBuffer = nullptr;
};