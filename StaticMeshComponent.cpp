#include "stdafx.h"
#include "StaticMeshComponent.h"

#include "DuckingEngine.h"
#include "ResourceManager.h"
#include "RenderModule.h"

#include "Model.h"
#include "Material.h"

namespace DK
{
	bool StaticMeshComponent::loadResource()
	{
		ResourceManager& resourceManager = DuckingEngine::getInstance().GetResourceManagerWritable();

		const ModelPropertyRef modelProperty = resourceManager.loadModelProperty(_modelPropertyPath);
		if (modelProperty == nullptr)
			return false;

		_model = resourceManager.loadStaticMesh(_modelPath, modelProperty);
		if (_model == nullptr)
			return false;

		return true;
	}
}