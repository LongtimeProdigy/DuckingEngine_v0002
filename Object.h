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
		Object& operator=(Object&&) = default;
		Object(const Object& rhs) = delete;
		virtual ~Object() = default;

		virtual void update(float deltaTime) = 0;

		void addComponent(Component* component)
		{
		    if (component)
		        _components.push_back(Ptr<Component>(component));
		}
		const DKVector<Ptr<Component>>& getComponents() const noexcept { return _components; }

		DK_REFLECTION_PROPERTY(Transform, _worldTransform);

	protected:
		DKVector<Ptr<Component>> _components;
	};
}
