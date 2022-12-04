#include "stdafx.h"
#include "Material.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "SceneRenderer.h"
#include "TextureManager.h"

namespace DK
{
	template<typename T>
	void MaterialParameterTemplate<T>::setValue(const T& value) noexcept
	{
		_value = value;
		memcpy(_valuePtr, &_value, getSize());
	}

	ITextureRef createTexture(const DKString& texturePath)
	{
		return DuckingEngine::getInstance().getTextureManagerWritable().createTexture(texturePath);
	}

	MaterialParameter* MaterialParameter::createMaterialParameter(const MaterialParameterDefinition& parameterDefinition)
	{
		switch (parameterDefinition._type)
		{
		case MaterialParameterType::FLOAT:
		{
			return dk_new MaterialParameterFloat(parameterDefinition._name, StringUtil::atof(parameterDefinition._value.c_str()));
		}
		case MaterialParameterType::TEXTURE:
		{
			ITextureRef texture = createTexture(parameterDefinition._value);
			return dk_new MaterialParameterTexture(parameterDefinition._name, texture);
		}
		default:
			DK_ASSERT_LOG(false, "ParameterMaterial이 지정되지 않았습니다. RenderPass를 확인바랍니다.");
			break;
		}

		return nullptr;
	}

	void MaterialParameterTexture::setValue(const ITextureRef& value) noexcept
	{
		DK_ASSERT_LOG(value != nullptr, "value가 nullptr입니다. 크래시가 날 것입니다.");
		_value = value;
		memcpy(_valuePtr, &_value->getSRV(), getSize());
	}

	template <>
	uint32 MaterialParameterTexture::getSize() const noexcept
	{
		return sizeof(ITexture::TextureSRVType);
	}

	bool Material::createMaterial(const MaterialDefinition& modelProperty, Material& outMaterial)
	{
		// 기존의 Parameter들의 Value들은 유지하면서 MaterialDefinition의 Parameter들로 Material::Parameters를 구성합니다.
		SceneRenderer& sceneRenderer = DuckingEngine::getInstance().getSceneRenderWritable();
		const MaterialDefinition* materialDefinition = sceneRenderer.getMaterialDefinition(modelProperty._materialName);
		DK_ASSERT_LOG(materialDefinition != nullptr, "RenderPass에 없는 Material을 만들려합니다! 절대 발생하면 안됩니다!");

		// RenderPass로부터 Parameter를 세팅
		uint32 parameterBufferSize = 0;
		const uint32 parameterCount = static_cast<uint32>(materialDefinition->_parameters.size());
		outMaterial._materialName = modelProperty._materialName;
		outMaterial._parameters.resize(parameterCount);
		for (uint32 i = 0; i < parameterCount; ++i)
		{
			MaterialParameter* newParameter = MaterialParameter::createMaterialParameter(materialDefinition->_parameters[i]);
			outMaterial._parameters[i] = newParameter;

			parameterBufferSize += outMaterial._parameters[i]->getSize();
		}

		// CPU 및 GPU 버퍼 생성 및 Parameter ValuePtr 세팅
		uint32 offset = 0;
		outMaterial._parameterBufferForCPU.resize(parameterBufferSize);
		for (uint32 i = 0; i < parameterCount; ++i)
		{
			outMaterial._parameters[i]->setValuePtr(&outMaterial._parameterBufferForCPU[offset]);
			offset += outMaterial._parameters[i]->getSize();
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		outMaterial._parameterBufferForGPU = renderModule.createUploadBuffer(parameterBufferSize);

		if (outMaterial.setModelProperty(modelProperty) == false)
			return false;

		return true;
	}

	Material::~Material()
	{
	}

	bool Material::setModelProperty(const MaterialDefinition& modelProperty)
	{
		DK_ASSERT_LOG(_materialName == modelProperty._materialName, "MaterialName이 ModelProperty의 MaterialName과 일치하지 않습니다!");
		DK_ASSERT_LOG(_parameters.size() != 0, "Parameters가 비어있습니다. UpdateTechnique가 먼저 이루어져야합니다.");

		const uint32 parameterCount = static_cast<uint32>(_parameters.size());
		const uint32 parameterDefinitionCount = static_cast<uint32>(modelProperty._parameters.size());
		for (uint32 i = 0; i < parameterDefinitionCount; ++i)
		{
			const MaterialParameterDefinition& parameterDefinition = modelProperty._parameters[i];
			for (uint32 j = 0; j < parameterCount; ++j)
			{
				MaterialParameter* parameter = _parameters[j].get();
				if (parameterDefinition._name == parameter->getName())
				{
					switch (parameterDefinition._type)
					{
					case MaterialParameterType::FLOAT:
					{
						MaterialParameterFloat* floatParameter = static_cast<MaterialParameterFloat*>(parameter);
						floatParameter->setValue(StringUtil::atof(parameterDefinition._value.c_str()));
					}
					break;
					case MaterialParameterType::TEXTURE:
					{
						ITextureRef texture = createTexture(parameterDefinition._value);
						MaterialParameterTexture* textureParameter = static_cast<MaterialParameterTexture*>(parameter);
						textureParameter->setValue(texture);
					}
					break;
					default:
						DK_ASSERT_LOG(false, "아직 지원하지 않는 MaterialParameter Type입니다.");
						break;
					}
				}
			}
		}

		_parameterBufferForGPU->upload(_parameterBufferForCPU.data());

		return true;
	}
}