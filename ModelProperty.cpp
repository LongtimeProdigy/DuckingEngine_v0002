#include "stdafx.h"
#include "ModelProperty.h"

#include "RenderModule.h"
#include "Material.h"

namespace DK
{
	ModelProperty::ModelProperty(const DKVector<MaterialDefinition>& materialDefinitionArr)
		: _materialDefinitionArr(materialDefinitionArr)
	{}
}