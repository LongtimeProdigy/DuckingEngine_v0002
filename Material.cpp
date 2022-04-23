#include "stdafx.h"
#include "Material.h"

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#include <DirectXMath.h>

// #todo- Camera삭제할 것
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
	// #todo- RootSignature와 PSO는 RenderModule의 Container에서 관리될 예정입니다.
	// RefCount만 감소시켜야합니다.
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
	// #todo- 현재 rootSignature, pso를 만드는 데에 실패할 수 있지만.. UpdateTechnique는 절대 실패하지 않는다고 가정합니다.
	// 반드시 항상 성공할 수 있는 방법이 있는지 고민해야합니다.
	// defaultMaterial은 어떤가? 엔진 시작시 만들어진 defaultMaterial을 반환?
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
			DK_ASSERT_LOG(false, "잘못된 MaterialType에 대하여 UpdateTechnique를 시도중입니다. 반드시 검토바랍니다.");
			return;
		}

		_descriptorHeapParameters.resize(1);
		_descriptorHeapParameters[0]._rootParameterIndex = 3;
		_descriptorHeapParameters[0]._descriptor = descriptor;

		// #todo- 일단 diffuseTexture만 생성해서 넣자!
		// #todo- 원래는 ModelProperty에서 값을 받아와야하는데.. 일단은 ModelProperty 구현이 안되어있으니 여기서 하드코딩합니다.
		static const char* diffuseTexturePath = "Resource/Character/Model/ganfaul_m_aure/textures/Ganfaul_diffuse.png";
		D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapHandle = descriptor->GetCPUDescriptorHandleForHeapStart();
		ITexture diffuseTexture = rmDX12->CreateTexture(diffuseTexturePath, descriptorHeapHandle);

		// #todo- MaterialEditor도입되면 MaterialParameter만들어서 넣어야할듯?
		//_parameters.resize(1);
		//_parameters[0] = dk_new MaterialParameterTexture(std::move(diffuseTexture));
	}
	break;
	case MaterialType::STATICMESH:
	default:
		DK_ASSERT_LOG(false, "잘못된 MaterialType에 대하여 UpdateTechnique를 시도중입니다. 반드시 검토바랍니다.");
	return;
	}
}

void Material::UploadParameters() const noexcept
{

}