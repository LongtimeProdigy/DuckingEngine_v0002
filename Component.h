#pragma once

namespace DK
{
	class Component
	{
	public:
		virtual ~Component() {}

		// Framework
		virtual void update(float deltaTime) = 0;
	};
}