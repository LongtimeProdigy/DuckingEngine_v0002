#pragma once
#include "Object.h"

namespace DK
{
	struct IBuffer;
}

namespace DK
{
	class SceneObject : public Object
	{
	public:
		virtual void update(float deltaTime) override final {}

	public:
		Ptr<IBuffer> _sceneObjectConstantBuffer = nullptr;
	};
}