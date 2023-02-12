#pragma once
#include "Component.h"

namespace DK
{
	class RenderableComponent : public Component
	{
	public:
		virtual bool loadResource() = 0;

	protected:
		DK_REFLECTION_PROPERTY(DKString, _modelPath);
		DK_REFLECTION_PROPERTY(DKString, _modelPropertyPath);
	};
}