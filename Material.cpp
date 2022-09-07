#include "stdafx.h"
#include "Material.h"

// #todo- 지울 수 있으면 지울 것
// D3D12_RESOURCE_STATE_GENERIC_READ 래핑하면 가능함
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "d3dx12.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "MaterialParameter.h"

Material* Material::createMaterial(const MaterialDefinition& data)
{
	Material* newMaterial = dk_new Material(data._materialName);

	// 기존의 Parameter들의 Value들은 유지하면서 MaterialDefinition의 Parameter들로 Material::Parameters를 구성합니다.
	RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
	const MaterialDefinition* materialDefinition = rm.findRenderPassByMaterialName(newMaterial->_materialName);
	DK_ASSERT_LOG(materialDefinition != nullptr, "RenderPass에 없는 Material을 만들려합니다! 절대 발생하면 안됩니다!");

	// RenderPass로부터 Parameter를 세팅
	uint32 parameterBufferSize = 0;
	const uint32 parameterCount = materialDefinition->_parameters.size();
	newMaterial->_parameters.reserve(parameterCount);
	for (uint32 i = 0; i < parameterCount; ++i)
	{
		MaterialParameter* newParameter = MaterialParameter::createMaterialParameter(materialDefinition->_parameters[i]);
		newMaterial->_parameters.push_back(std::move(newParameter));

		parameterBufferSize += newParameter.getSize();
	}
	
	// CPU 및 GPU 버퍼 생성 및 Parameter ValuePtr 세팅
	uint32 offset = 0;
	newMaterial->_parameterBufferForCPU.resize(parameterBufferSize);
	for (uint32 i = 0; i < parameterCount; ++i)
	{
		newMaterial->_parameters[i].setValuePtr(&newMaterial->_parameterBufferForCPU[offset]);
		offset += newMaterial->_parameters[i].getSize();
	}
	newMaterial->_parameterBufferForGPU = rm.createUploadBuffer(newMaterial->_parameterBufferForCPU.data(), parameterBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);

	if (newMaterial->setModelProperty(data) == false)
		return nullptr;

	return newMaterial;
}

bool Material::setModelProperty(const MaterialDefinition& materialDefinition)
{
	DK_ASSERT_LOG(_materialName == materialDefinition._materialName, "MaterialName이 ModelProperty의 MaterialName과 일치하지 않습니다!");
	DK_ASSERT_LOG(_parameters.size() != 0, "Parameters가 비어있습니다. UpdateTechnique가 먼저 이루어져야합니다.");

	const uint32 parameterCount = _parameters.size();
	const uint32 parameterDefinitionCount = materialDefinition._parameters.size();
	for (uint32 i = 0; i < parameterDefinitionCount; ++i)
	{
		const MaterialParameterDefinition& parameterDefinition = materialDefinition._parameters[i];
		for (uint32 j = 0; j < parameterCount; ++j)
		{
			MaterialParameter& parameter = _parameters[j];
			if (parameterDefinition._name == parameter.getName())
			{
				switch (parameterDefinition._type)
				{
					case MaterialParameterType::FLOAT:
						parameter.setValue(parameterDefinition._value);
						break;
					case MaterialParameterType::TEXTURE:
						break;
					default:
						break;
				}
			}
		}
	}

	return true;
}