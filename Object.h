#pragma once

#include "Component.h"

namespace DK
{
	class Component;
}

namespace DK
{
	class Object
	{
	public:
		Object() = default;
		Object(Object&&) = default;
		Object(const Object& rhs) = delete;

		virtual void update(float deltaTime) = 0;

		void addComponent(Component* component) noexcept
		{
			_components.push_back(Ptr<Component>(component));
		}

	public:
		DKVector<Ptr<Component>> _components;
		DK_REFLECTION_PROPERTY(Transform, _worldTransform);
	};
}
