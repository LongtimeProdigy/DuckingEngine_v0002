#pragma once
class Component
{
public:
	Component() {}
	virtual ~Component() {}

private:
	uint _guid = -1;
};