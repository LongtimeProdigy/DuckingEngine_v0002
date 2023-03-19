#include "stdafx.h"
#include "Material.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "SceneRenderer.h"
#include "TextureManager.h"

namespace DK
{
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
			ITextureRef texture = DuckingEngine::getInstance().getTextureManagerWritable().createTexture(parameterDefinition._value);
			return dk_new MaterialParameterTexture(parameterDefinition._name, texture);
		}
		default:
			DK_ASSERT_LOG(false, "ParameterMaterial�� �������� �ʾҽ��ϴ�. RenderPass�� Ȯ�ιٶ��ϴ�.");
			break;
		}

		return nullptr;
	}

	uint32 MaterialParameterTexture::getParameterSize() const noexcept
	{
		return sizeof(ITexture::TextureSRVType);
	}

	template<typename T>
	void MaterialParameterTemplate<T>::setParameterValue(const T& value) noexcept
	{
		_value = value;
		DK::memcpy(_valuePtr, &_value, getParameterSize());
	}
	void MaterialParameterTexture::setParameterValue(const ITextureRef& value) noexcept
	{
		DK_ASSERT_LOG(value != nullptr, "value�� nullptr�Դϴ�. ũ���ð� �� ���Դϴ�.");
		_value = value;
		DK::memcpy(_valuePtr, &_value->getSRV(), getParameterSize());
	}

	Material* Material::createMaterial(const MaterialDefinition& modelProperty)
	{
		// ������ Parameter���� Value���� �����ϸ鼭 MaterialDefinition�� Parameter��� Material::Parameters�� �����մϴ�.
		SceneRenderer& sceneRenderer = DuckingEngine::getInstance().getSceneRenderWritable();
		const MaterialDefinition* materialDefinition = sceneRenderer.getMaterialDefinition(modelProperty._materialName);
		if (materialDefinition == nullptr)
		{
			DK_ASSERT_LOG(false, "RenderPass�� ���� Material(%s)�� ������մϴ�! ���� �߻��ϸ� �ȵ˴ϴ�!", modelProperty._materialName.c_str());
			return nullptr;
		}

		// RenderPass�κ��� Parameter�� ����
		uint32 parameterBufferSize = 0;
		const uint32 parameterCount = static_cast<uint32>(materialDefinition->_parameters.size());
		Material* outMaterial = dk_new Material;
		outMaterial->_materialName = modelProperty._materialName;
		outMaterial->_parameters.resize(parameterCount);
		for (uint32 i = 0; i < parameterCount; ++i)
		{
			MaterialParameter* newParameter = MaterialParameter::createMaterialParameter(materialDefinition->_parameters[i]);
			outMaterial->_parameters[i] = newParameter;

			parameterBufferSize += outMaterial->_parameters[i]->getParameterSize();
		}

		// CPU �� GPU ���� ���� �� Parameter ValuePtr ����
		uint32 offset = 0;
		outMaterial->_parameterBufferForCPU.resize(parameterBufferSize);
		for (uint32 i = 0; i < parameterCount; ++i)
		{
			outMaterial->_parameters[i]->setParameterValuePtr(&outMaterial->_parameterBufferForCPU[offset]);
			offset += outMaterial->_parameters[i]->getParameterSize();
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		outMaterial->_parameterBufferForGPU = renderModule.createUploadBuffer(parameterBufferSize);

		if (outMaterial->setModelProperty(modelProperty) == false)
			return nullptr;

		return outMaterial;
	}

	bool Material::setModelProperty(const MaterialDefinition& modelProperty)
	{
		DK_ASSERT_LOG(_materialName == modelProperty._materialName, "MaterialName�� ModelProperty�� MaterialName�� ��ġ���� �ʽ��ϴ�!");
		DK_ASSERT_LOG(_parameters.size() != 0, "Parameters�� ����ֽ��ϴ�. UpdateTechnique�� ���� �̷�������մϴ�.");

		const uint32 parameterCount = static_cast<uint32>(_parameters.size());
		const uint32 parameterDefinitionCount = static_cast<uint32>(modelProperty._parameters.size());
		for (uint32 i = 0; i < parameterDefinitionCount; ++i)
		{
			bool validation = true;
			const MaterialParameterDefinition& parameterDefinition = modelProperty._parameters[i];
			for (uint32 j = 0; j < parameterCount; ++j)
			{
				MaterialParameter* parameter = _parameters[j].get();
				if (parameterDefinition._name == parameter->getParameterName())
				{
					switch (parameterDefinition._type)
					{
					case MaterialParameterType::FLOAT:
					{
						MaterialParameterFloat* floatParameter = static_cast<MaterialParameterFloat*>(parameter);
						floatParameter->setParameterValue(StringUtil::atof(parameterDefinition._value.c_str()));
						break;
					}
					case MaterialParameterType::TEXTURE:
					{
						ITextureRef texture = DuckingEngine::getInstance().getTextureManagerWritable().createTexture(parameterDefinition._value);
						MaterialParameterTexture* textureParameter = static_cast<MaterialParameterTexture*>(parameter);
						textureParameter->setParameterValue(texture);
						break;
					}
					default:
					{
						DK_ASSERT_LOG(false, "���� �������� �ʴ� MaterialParameter Type�Դϴ�.");
						break;
					}
					}

					validation &= true;
				}
			}

			DK_ASSERT_LOG(validation, "Material�� ModelProperty���̿� Name�� �ٸ� Parameter�� �ֽ��ϴ�.");
		}

		_parameterBufferForGPU->upload(_parameterBufferForCPU.data());

		return true;
	}
}