#include "stdafx.h"
#include "SceneRenderer.h"

#include "Matrix4x4.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "Camera.h"

struct SceneConstantBufferStruct
{
	Matrix4x4 cameraWorldMatrix;
	Matrix4x4 cameraProjectionMatrix;
};

void SceneRenderer::createSceneConstantBuffer() noexcept
{
	DK_ASSERT_LOG(Camera::gMainCamera != nullptr, "MainCamera가 먼저 생성되어야합니다.");

	// 사실 UpdateRender함수에서 _sceneConstantBuffer를 Upload하기 때문에 여기서 Camera가 필요하진 않을 수 있음
	SceneConstantBufferStruct cameraConstanceBufferData;
	Camera::gMainCamera->GetCameraWorldMatrix(cameraConstanceBufferData.cameraWorldMatrix);
	Camera::gMainCamera->GetCameraProjectionMaterix(cameraConstanceBufferData.cameraProjectionMatrix);
	_sceneConstantBuffer = DuckingEngine::getInstance().GetRenderModule().createUploadBuffer(&cameraConstanceBufferData, sizeof(cameraConstanceBufferData), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void SceneRenderer::uploadSceneConstantBuffer() const noexcept
{
	SceneConstantBufferStruct cameraConstantBufferData;
	Camera::gMainCamera->GetCameraProjectionMaterix(cameraConstantBufferData.cameraProjectionMatrix);
	Camera::gMainCamera->GetCameraWorldMatrix(cameraConstantBufferData.cameraWorldMatrix);
	cameraConstantBufferData.cameraProjectionMatrix.Transpose();
	cameraConstantBufferData.cameraWorldMatrix.Transpose();
	uint8* cameraConstantBufferAddress = _sceneConstantBuffer->Map();
	memcpy(cameraConstantBufferAddress, &cameraConstantBufferData, sizeof(cameraConstantBufferData));
}

void SceneRenderer::PreRender() const noexcept
{
#if defined(USE_IMGUI)
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	{
		static float f = 0.0f;
		static int counter = 0;
		static char buf[200] = {};

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

#define MAX_BUFFER_LENGTH 200
		ImGui::Text("MainCameraPosition");

		char cameraPosBuffer[MAX_BUFFER_LENGTH];
		const float3& cameraPosition = Camera::gMainCamera->GetWorldTransform().GetPosition();
		sprintf_s(cameraPosBuffer, MAX_BUFFER_LENGTH, "Position: x: %f, y: %f, z: %f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
		ImGui::Text(cameraPosBuffer);

		char cameraRotationBuffer[MAX_BUFFER_LENGTH];
		const float3 cameraRotation = Camera::gMainCamera->GetWorldTransform().GetRotation();
		sprintf_s(cameraRotationBuffer, MAX_BUFFER_LENGTH, "Rotation: x: %f, y: %f, z: %f", cameraRotation.x, cameraRotation.y, cameraRotation.z);
		ImGui::Text(cameraRotationBuffer);

		char cameraRotationMatrixBuffer[MAX_BUFFER_LENGTH];
		Matrix3x3 rotationMatrix;
		Camera::gMainCamera->GetWorldTransform().GetRoationMatrix(rotationMatrix);
		sprintf_s(cameraRotationMatrixBuffer, MAX_BUFFER_LENGTH, "%f, %f, %f\n%f, %f, %f\n%f, %f, %f",
			rotationMatrix._11, rotationMatrix._12, rotationMatrix._13,
			rotationMatrix._21, rotationMatrix._22, rotationMatrix._23,
			rotationMatrix._31, rotationMatrix._32, rotationMatrix._33
		);
		ImGui::Text(cameraRotationMatrixBuffer);

		char cameraForwardBuffer[MAX_BUFFER_LENGTH];
		float3 cameraForward = Camera::gMainCamera->GetWorldTransform().GetForward();
		sprintf_s(cameraForwardBuffer, MAX_BUFFER_LENGTH, "Forward: x: %f, y: %f, z: %f", cameraForward.x, cameraForward.y, cameraForward.z);
		ImGui::Text(cameraForwardBuffer);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	ImGui::Render();
#endif

	WaitForPreviousFrame();

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _renderTargetResources[_currentFrame]->GetResourceWritable();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &barrier);

	const UINT rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += rtvDescriptorSize * _currentFrame;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float clearColor[] = { 0.5f, 0.5f, 0.9f, 1.0f };
	_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	_commandList->RSSetViewports(1, &_viewport);
	_commandList->RSSetScissorRects(1, &_scissorRect);
	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SceneRenderer::RenderSkinnedMesh(const SkinnedMeshComponent* skinnedMeshComponent) const noexcept
{
	if (skinnedMeshComponent == nullptr)
	{
		return;
	}

	// SkinnedMesh ConstantBuffer
	const DKVector<Matrix4x4>& currentCharacterSpaceBoneAnimation = skinnedMeshComponent->GetCurrentCharacterSpaceBoneAnimation();
	if (currentCharacterSpaceBoneAnimation.empty() == false)
	{
		const IResource* skeletonBuffer = skinnedMeshComponent->getSkeletonConstantBuffer();
		uint8* skeletonConstantBufferAddress = skeletonBuffer->Map();
		memcpy(skeletonConstantBufferAddress, &currentCharacterSpaceBoneAnimation[0], sizeof(Matrix4x4) * currentCharacterSpaceBoneAnimation.size());
		skeletonBuffer->UnMap();
	}

	// Material Parameters
	const DKVector<SubMesh>& subMeshes = skinnedMeshComponent->GetModel()->GetSubMeshes();
	for (uint i = 0; i < subMeshes.size(); ++i)
	{
		const SubMesh& subMesh = subMeshes[i];

		_commandList->SetGraphicsRootSignature(subMesh._material->_rootSignature.get());
		_commandList->SetPipelineState(subMesh._material->_pipelineStateObject.get());

		for (uint i = 0; i < subMesh._material->_constant32BitParamters.size(); ++i)
		{
			_commandList->SetGraphicsRoot32BitConstant(
				subMesh._material->_constant32BitParamters[i]._rootParameterIndex,
				subMesh._material->_constant32BitParamters[i]._constanceData,
				0
			);
		}
		for (uint i = 0; i < subMesh._material->_constantBufferParamters.size(); ++i)
		{
			if (subMesh._material->_constantBufferParamters[i]._constantBuffer == nullptr)
			{
				continue;
			}

			_commandList->SetGraphicsRootConstantBufferView(
				subMesh._material->_constantBufferParamters[i]._rootParameterIndex,
				subMesh._material->_constantBufferParamters[i]._constantBuffer->GetResourceWritable()->GetGPUVirtualAddress()
			);
		}

		DKVector<ID3D12DescriptorHeap*> descriptorArr;
		descriptorArr.resize(subMesh._material->_descriptorHeapParameters.size());
		for (uint i = 0; i < subMesh._material->_descriptorHeapParameters.size(); ++i)
		{
			descriptorArr[i] = subMesh._material->_descriptorHeapParameters[i]._descriptor->GetDescriptorHeapWritable();
		}
		_commandList->SetDescriptorHeaps(descriptorArr);
		for (uint i = 0; i < subMesh._material->_descriptorHeapParameters.size(); ++i)
		{
			_commandList->SetGraphicsRootDescriptorTable(
				subMesh._material->_descriptorHeapParameters[i]._rootParameterIndex,
				subMesh._material->_descriptorHeapParameters[i]._descriptor->GetGPUDescriptorHandleForHeapStart()
			);
		}

		_commandList->IASetVertexBuffers(0, 1, &subMesh._vertexBufferView.get()->GetViewWritable());
		_commandList->IASetIndexBuffer(&subMesh._indexBufferView.get()->GetViewWritable());
		uint maxValue = 0;
		for (uint i = 0; i < subMesh._indices.size(); ++i)
		{
			if (subMesh._indices[i] > maxValue)
			{
				maxValue = subMesh._indices[i];
			}
		}
		_commandList->DrawIndexedInstanced(static_cast<UINT>(subMesh._indices.size()), 1, 0, 0, 0);
	}
}

void SceneRenderer::RenderUI() const noexcept
{
#ifdef USE_IMGUI
	DKVector<ID3D12DescriptorHeap*> descriptorHeaps;
	descriptorHeaps.push_back(g_pd3dSrvDescHeap.GetDescriptorHeapWritable());
	_commandList->SetDescriptorHeaps(descriptorHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList->GetCommandListWritable());
#endif // USE_IMGUI
}

void SceneRenderer::EndRender() const noexcept
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _renderTargetResources[_currentFrame]->GetResourceWritable();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &barrier);

	ExcuteCommandList();

	UINT syncInterval = 0; //gVSync ? 1 : 0;
	UINT presentFlags = 0; // CheckTearingSupport() && !gVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	bool success = _swapChain->Present(syncInterval, presentFlags);
}