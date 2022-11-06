#pragma once

class Component
{
public:
	virtual ~Component() {}

	virtual void update(float deltaTime) = 0;
};