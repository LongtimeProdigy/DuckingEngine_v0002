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
		SceneObject() = default;
		SceneObject(SceneObject&& rhs) = default;
		SceneObject& operator=(SceneObject&&) noexcept = default;

		void update(float deltaTime) override final;

	public:
		IBufferRef _sceneObjectConstantBuffer;
	};
}
