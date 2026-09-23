#pragma once

namespace DK
{
	class Component
	{
	public:
		virtual ~Component() = default;
		Component() = default;
		Component(const Component& rhs) = delete;

		// Framework
		virtual void update(float deltaTime) = 0;
	};
}
