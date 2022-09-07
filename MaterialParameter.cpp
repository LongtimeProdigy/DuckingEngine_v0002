#include "stdafx.h"
#include "MaterialParameter.h"

#include "DuckingEngine.h"
#include "TextureManager.h"

MaterialParameter* MaterialParameter::createMaterialParameter(const MaterialParameterDefinition& parameterDefinition)
{
	switch (parameterDefinition._type)
	{
	case MaterialParameterType::FLOAT:
	{
		return dk_new MaterialParameterFloat(parameterDefinition._name, parameterDefinition._value);
	}
	case MaterialParameterType::TEXTURE:
	{
		uint32 textureIndex = 0;
		ITexture texture(static_cast<const char*>(parameterDefinition._value));
		if (DuckingEngine::getInstance().getTextureManagerWritable().createTexture(texture.getPath(), texture, textureIndex) == false)
			return nullptr;

		MaterialParameterTexture parameter(parameterDefinition._name, texture, parameterDefinition._register);
		parameter.setValuePtr(&textureIndex);

		return parameter;
	}
	default:
		DK_ASSERT_LOG(false, "ParameterMaterial�� �������� �ʾҽ��ϴ�. RenderPass�� Ȯ�ιٶ��ϴ�.");
		break;
	}
}

template <>
void MaterialParameterTexture::setValue(void* value) noexcept
{
	DK_ASSERT_LOG(value != nullptr, "value�� nullptr�Դϴ�. ũ���ð� �� ���Դϴ�.");

	using TextureIndexType = uint32;

	static const uint32 gDefaultTextureIndex = 0;
	TextureIndexType textureIndex = -1;
	const DKString texturePath = static_cast<const char*>(value);
	if (DuckingEngine::getInstance().getTextureManagerWritable().createTexture(texturePath, *_value.get(), textureIndex) == false)
	{
		DK_ASSERT_LOG(false, "Texture Loading�� �����߽��ϴ�. �⺻ �ؽ��İ� ���� �� �ֽ��ϴ�.");
		memcpy(_valuePtr, &gDefaultTextureIndex, sizeof(TextureIndexType));
		return;
	}

	memcpy(_valuePtr, &textureIndex, sizeof(TextureIndexType));
}

template <>
uint32 MaterialParameterTexture::getSize() const noexcept
{

}