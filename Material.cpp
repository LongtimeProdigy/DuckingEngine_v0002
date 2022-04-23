#include "stdafx.h"
#include "Material.h"

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#include <DirectXMath.h>

// #todo- Camera������ ��
#include "Camera.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "RenderModuleDX12.h"

#include "ITexture.h"
#include "MaterialParameter.h"
#include "IDescriptorHeap.h"
#include "IResource.h"
#include "SceneObject.h"

IRootParameter::IRootParameter()
{

}
IRootParameter::~IRootParameter()
{
	dk_delete _constantBuffer;
	dk_delete _descriptor;
}

Material::~Material()
{
	// #todo- RootSignature�� PSO�� RenderModule�� Container���� ������ �����Դϴ�.
	// RefCount�� ���ҽ��Ѿ��մϴ�.
	//dk_delete _rootSignature;
	//dk_delete _pipelineStateObject;

	if (_parameters.size() != 0)
	{
		dk_delete_array& _parameters[0];
		_parameters.clear();
	}
}

void Material::UpdateTechnique(const SceneObject* sceneObject) noexcept
{
	RenderModule* rm = DuckingEngine::GetRenderModuleWritable();
	RenderModuleDX12* rmDX12 = static_cast<RenderModuleDX12*>(rm);
	// #todo- ���� rootSignature, pso�� ����� ���� ������ �� ������.. UpdateTechnique�� ���� �������� �ʴ´ٰ� �����մϴ�.
	// �ݵ�� �׻� ������ �� �ִ� ����� �ִ��� ����ؾ��մϴ�.
	// defaultMaterial�� ���? ���� ���۽� ������� defaultMaterial�� ��ȯ?
	bool success = rmDX12->LoadRootSignature(_type, _rootSignature);
	CHECK_BOOL_AND_CUSTOMRETURN(success, );
	success = rmDX12->LoadPipelineStateObject(_type, *_rootSignature.get(), _pipelineStateObject);
	CHECK_BOOL_AND_CUSTOMRETURN(success, );

	// Create Parameters
	switch (_type)
	{
	case MaterialType::SKINNEDMESH:
	{
		_constantBufferParamters.resize(3);
		_constantBufferParamters[0]._rootParameterIndex = 0;
		_constantBufferParamters[0]._constantBuffer = Camera::gCameraConstantBuffer;

		_constantBufferParamters[1]._rootParameterIndex = 1;
		_constantBufferParamters[1]._constantBuffer = sceneObject->_sceneObjectConstantBuffer;

		_constantBufferParamters[2]._rootParameterIndex = 2;
		_constantBufferParamters[2]._constantBuffer = sceneObject->_skeletonConstantBuffer;

		IDescriptorHeap* descriptor = dk_new IDescriptorHeap;
		const bool success = rmDX12->CreateDescriptorHeap(_type, *descriptor);
		if (success == false)
		{
			DK_ASSERT_LOG(false, "�߸��� MaterialType�� ���Ͽ� UpdateTechnique�� �õ����Դϴ�. �ݵ�� ����ٶ��ϴ�.");
			return;
		}

		_descriptorHeapParameters.resize(1);
		_descriptorHeapParameters[0]._rootParameterIndex = 3;
		_descriptorHeapParameters[0]._descriptor = descriptor;

		// #todo- �ϴ� diffuseTexture�� �����ؼ� ����!
		// #todo- ������ ModelProperty���� ���� �޾ƿ;��ϴµ�.. �ϴ��� ModelProperty ������ �ȵǾ������� ���⼭ �ϵ��ڵ��մϴ�.
		static const char* diffuseTexturePath = "Resource/Character/Model/ganfaul_m_aure/textures/Ganfaul_diffuse.png";
		D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapHandle = descriptor->GetCPUDescriptorHandleForHeapStart();
		ITexture diffuseTexture = rmDX12->CreateTexture(diffuseTexturePath, descriptorHeapHandle);

		// #todo- MaterialEditor���ԵǸ� MaterialParameter���� �־���ҵ�?
		//_parameters.resize(1);
		//_parameters[0] = dk_new MaterialParameterTexture(std::move(diffuseTexture));
	}
	break;
	case MaterialType::STATICMESH:
	default:
		DK_ASSERT_LOG(false, "�߸��� MaterialType�� ���Ͽ� UpdateTechnique�� �õ����Դϴ�. �ݵ�� ����ٶ��ϴ�.");
	return;
	}
}

void Material::UploadParameters() const noexcept
{

}