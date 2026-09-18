#pragma once
#include "Object.h"

namespace DK
{
	struct IBuffer;
	using IBufferRef = std::shared_ptr<IBuffer>;
}

namespace DK
{
	class SceneObject : public Object
	{
	public:
		virtual void update(float deltaTime) override final {}

	public:
		IBufferRef _sceneObjectConstantBuffer;
	};
}
