#pragma once
#include "Object.h"

class SceneObject : public Object
{
public:
	SceneObject();
	virtual ~SceneObject() override final;

	virtual void Update() override final {}

public:
	IResource* _skeletonConstantBuffer;
};