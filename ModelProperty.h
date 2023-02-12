#pragma once

namespace DK
{
	struct MaterialDefinition;

	class ModelProperty
	{
	public:
		ModelProperty(const DKVector<MaterialDefinition>& materialDefinitionArr);

	private:
		DK_REFLECTION_VECTOR_PROPERTY(MaterialDefinition, _materialDefinitionArr);
	};
}