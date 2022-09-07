#pragma once
#include "Object.h"

class SceneObject : public Object
{
public:
	virtual ~SceneObject() override final;

	virtual void Update() override final {}

public:
	IResource* _sceneObjectConstantBuffer = nullptr;
};