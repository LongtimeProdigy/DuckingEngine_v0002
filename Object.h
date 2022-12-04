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
		virtual ~Object() {}

		virtual void update(float deltaTime) = 0;

		void addComponent(Component* component) noexcept
		{
			_components.push_back(component);
		}

	public:
		DKVector<Ptr<Component>> _components;
		DK_REFLECTION_DECLARE(Transform, _worldTransform);
	};
}