#pragma once
#include "RenderableComponent.h"

namespace DK
{
	class StaticMeshComponent : public RenderableComponent
	{
	public:
		virtual ~StaticMeshComponent() override final
		{}

		virtual bool loadResource() override final;

		virtual void update(float deltaTime) override final
		{}

	private:
		DK_REFLECTION_PROPERTY(StaticMeshModelRef, _model);
	};
}