#include "stdafx.h"
#include "Material.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "SceneRenderer.h"

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
			ITextureRef texture = DuckingEngine::getInstance().GetRenderModuleWritable().allocateTexture(parameterDefinition._value);
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

	MaterialParameter::Type MaterialParameterFloat::getType() const noexcept
	{
		return Type::FLOAT;
	}
	MaterialParameter::Type MaterialParameterTexture::getType() const noexcept
	{
		return Type::TEXTURE;
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
		outMaterial->_parameterArr.resize(parameterCount);
		for (uint32 i = 0; i < parameterCount; ++i)
		{
			MaterialParameter* newParameter = MaterialParameter::createMaterialParameter(materialDefinition->_parameters[i]);
			outMaterial->_parameterArr[i] = newParameter;

			parameterBufferSize += outMaterial->_parameterArr[i]->getParameterSize();
		}

		// CPU �� GPU ���� ���� �� Parameter ValuePtr ����
		uint32 offset = 0;
		outMaterial->_parameterBufferForCPU.resize(parameterBufferSize);
		for (uint32 i = 0; i < parameterCount; ++i)
		{
			outMaterial->_parameterArr[i]->setParameterValuePtr(&outMaterial->_parameterBufferForCPU[offset]);
			offset += outMaterial->_parameterArr[i]->getParameterSize();
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		outMaterial->_parameterBufferForGPU = renderModule.createUploadBuffer(parameterBufferSize, L"Material_CBuffer");

		if (outMaterial->setModelProperty(modelProperty) == false)
			return nullptr;

		return outMaterial;
	}

	bool Material::setModelProperty(const MaterialDefinition& modelProperty)
	{
		DK_ASSERT_LOG(_materialName == modelProperty._materialName, "MaterialName�� ModelProperty�� MaterialName�� ��ġ���� �ʽ��ϴ�!");
		DK_ASSERT_LOG(_parameterArr.size() != 0, "Parameters�� ����ֽ��ϴ�. UpdateTechnique�� ���� �̷�������մϴ�.");

		const uint32 parameterCount = _parameterArr.size();
		const uint32 parameterDefinitionCount = modelProperty._parameters.size();
		for (uint32 i = 0; i < parameterDefinitionCount; ++i)
		{
			bool validation = true;
			const MaterialParameterDefinition& parameterDefinition = modelProperty._parameters[i];
			for (uint32 j = 0; j < parameterCount; ++j)
			{
				MaterialParameter* parameter = _parameterArr[j].get();
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
						ITextureRef texture = DuckingEngine::getInstance().GetRenderModuleWritable().allocateTexture(parameterDefinition._value);
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

	void Material::setParameterValue(const DKString& name, void* value)
	{
		for (Ptr<MaterialParameter>& parameter : _parameterArr)
		{
			if (parameter->getParameterName() != name)
				continue;

			switch (parameter->getType())
			{
			case MaterialParameter::Type::FLOAT:
				parameter->setParameterValuePtr(value);	// �⺻Ÿ���� ĳ���� ���� �׳� �����ص� ��������
				break;
			case MaterialParameter::Type::TEXTURE:
			{
				MaterialParameterTexture* textureParameter = static_cast<MaterialParameterTexture*>(parameter.get());
				textureParameter->setParameterValue(*static_cast<ITextureRef*>(value));
				break;
			}
			}
;
			_parameterBufferForGPU->upload(_parameterBufferForCPU.data());
			return;
		}
	}
}