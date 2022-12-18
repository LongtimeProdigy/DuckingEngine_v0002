#pragma once
#include "Component.h"

namespace DK
{
	class Model;
}

namespace DK
{
	class MeshComponent : public Component
	{
	private:
		DK_REFLECTION_DECLARE(StaticMeshModelRef, _model);
	};
}