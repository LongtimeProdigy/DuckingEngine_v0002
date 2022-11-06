#pragma once
#include "Object.h"

struct IBuffer;

class SceneObject : public Object
{
public:
	virtual void Update() override final {}

public:
	Ptr<IBuffer> _sceneObjectConstantBuffer = nullptr;
};