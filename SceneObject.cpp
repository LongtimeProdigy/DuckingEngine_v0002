#include "stdafx.h"
#include "SceneObject.h"

namespace DK
{
	void SceneObject::update(float deltaTime)
	{
		const uint32 count = static_cast<uint32>(_components.size());
		for (uint32 i = 0; i < count; ++i)
			_components[i]->update(deltaTime);
	}
}
