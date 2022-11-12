#include "stdafx.h"
#include "SceneRenderer.h"

#pragma region Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "d3dx12.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#endif
#pragma endregion

#include "Matrix4x4.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "TextureManager.h"
#include "SceneObjectManager.h"
#include "Camera.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"
#include "SceneObject.h"
#include "Material.h"
#include "EditorDebugDrawManager.h"

struct SceneConstantBufferStruct
{
	Matrix4x4 cameraWorldMatrix;
	Matrix4x4 cameraProjectionMatrix;
};

bool SceneRenderer::initialize()
{
	if (initialize_createRenderPass() == false) return false;
	if (initialize_createMaterialDefinition() == false) return false;
	if (initialize_createSceneConstantBuffer() == false) return false;

	return true;
}

constexpr static const char* ShaderVariableTypeString[static_cast<uint32>(ShaderVariableType::Count)] =
{
	"Buffer",
	"StructuredBuffer"
};
ShaderVariableType convertStringToEnum2(const char* str)
{
	for (uint32 i = 0; i < static_cast<uint32>(ShaderVariableType::Count); ++i)
	{
		if (strcmp(str, ShaderVariableTypeString[i]) == 0)
		{
			return static_cast<ShaderVariableType>(i);
		}
	}

	return ShaderVariableType::Count;
};
#if defined(USE_TINYXML)
bool parseShaderVariable(TiXmlElement* variableNode, ShaderVariable& outVariable)
#else
static_assert("현재 지원되는 XML 로더가 없습니다.");
#endif
{
	const char* variableName = variableNode->ToElement()->Attribute("Name");
	const char* variableTypeRaw = variableNode->ToElement()->Attribute("Type");
	const char* variableRegisterRaw = variableNode->ToElement()->Attribute("Register");

	DK_ASSERT_LOG(variableName != nullptr && variableTypeRaw != nullptr && variableRegisterRaw != nullptr, "RenderPass의 Parameter가 비정상인 상황. 엔진이 비정상 작동할 수 있습니다.");

	const ShaderVariableType variableType = convertStringToEnum2(variableTypeRaw);
	DK_ASSERT_LOG(variableType != ShaderVariableType::Count, "RenderPass의 ParameterType이 정확하지 않습니다.\nType: %s", variableTypeRaw);
	const uint32 variableRegister = atoi(variableRegisterRaw);
	const char* variableValue = variableNode->ToElement()->Value();

	if (variableType == ShaderVariableType::Count)
	{
		DK_ASSERT_LOG(false, "현재 지원하지 않는 MaterialType을 사용하였습니다. ParameterName: %s, ParameterType: %d", variableName, variableType);
		return false;
	}

	outVariable._name = variableName;
	outVariable._type = variableType;
	outVariable._register = variableRegister;

	return true;
}
bool SceneRenderer::initialize_createRenderPass()
{
	static const char* renderPassGroupPath = "C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/RenderPass/RenderPassGroup.xml";
#if defined(USE_TINYXML)
	TiXmlDocument doc;
	doc.LoadFile(renderPassGroupPath);
	TiXmlElement* rootNode = doc.FirstChildElement("RenderPassGroup");
	if (rootNode == nullptr) return false;

	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	for (TiXmlElement* renderPassNode = rootNode->FirstChildElement(); renderPassNode != nullptr; renderPassNode = renderPassNode->NextSiblingElement())
	{
		// parsing
		RenderPass::CreateInfo info;
		info._renderPassName = renderPassNode->Attribute("Name");

		DKVector<ShaderVariable> shaderVariables;
		for (TiXmlElement* childNode = renderPassNode->FirstChildElement(); childNode != nullptr; childNode = childNode->NextSiblingElement())
		{
			if (childNode->Type() == 2)	//NODETYPE::TINYXML_COMMENT
				continue;

			DKString nodeName = childNode->Value();
			if (nodeName == "VertexShader")
			{
				info._vertexShaderEntry = childNode->Attribute("Entry");
				info._vertexShaderPath = childNode->GetText();
			}
			else if (nodeName == "PixelShader")
			{
				info._pixelShaderEntry = childNode->Attribute("Entry");
				info._pixelShaderPath = childNode->GetText();
			}
			else if (nodeName == "Parameter")
			{
				ShaderVariable variable;
				if (parseShaderVariable(childNode, variable) == false)
				{
					return false;
				}
				info._variables.push_back(std::move(variable));
			}
			else
			{
				DK_ASSERT_LOG(false, "지원하지 RenderPass ChildNode입니다. NodeName: %s", nodeName.c_str());
				return false;
			}
		}

		if (renderModule.createRenderPass(std::move(info)) == false)
			return false;
	}

	return true;
#else
	static_assert("현재 지원하지 XML 로더가 없습니다.");
	return false;
#endif
}

bool SceneRenderer::initialize_createMaterialDefinition()
{
	// #todo- 나중에 MaterialGroup을 추가하고 Technique로 RenderPass와 연결시켜줘야할듯?
	static const char* materialDefinitionGroupPass = "C:/Users/Lee/Desktop/Projects/DuckingEngine_v0002/Resource/Material/SkinnedMeshStandard.xml";
#if defined(USE_TINYXML)
	TiXmlDocument doc;
	doc.LoadFile(materialDefinitionGroupPass);
	TiXmlElement* rootNode = doc.FirstChildElement("Material");
	if (rootNode == nullptr) return false;

	MaterialDefinition materialDefinition;
	const DKString materialName = rootNode->Attribute("Name");

	using FindResult = DKHashMap<DKString, MaterialDefinition>::iterator;
	FindResult findResult = _materialDefinitionMap.find(materialName);
	if (findResult != _materialDefinitionMap.end())
	{
		DK_ASSERT_LOG(false, "중복된 이름의 MaterialDefinition이 있습니다.");
		return false;
	}

	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	for (TiXmlNode* parameterNode = rootNode->FirstChild(); parameterNode != nullptr; parameterNode = parameterNode->NextSibling())
	{
		const char* name = parameterNode->ToElement()->Attribute("Name");
		const char* typeRaw = parameterNode->ToElement()->Attribute("Type");
		const MaterialParameterType type = convertStringToEnum(typeRaw);
		const char* value = parameterNode->ToElement()->GetText();

		MaterialParameterDefinition parameterDefinition;
		parameterDefinition._name = name;
		parameterDefinition._type = type;
		switch (type)
		{
		case MaterialParameterType::FLOAT:
		{
			parameterDefinition._value = value;
		}
		break;
		case MaterialParameterType::TEXTURE:
			parameterDefinition._value = value;
		break;
		default:
			DK_ASSERT_LOG(false, "지원하지 않는 MaterialParameterDefinition Type입니다.");
		break;
		}

		materialDefinition._parameters.push_back(std::move(parameterDefinition));
	}
#else
	static_assert("현재 지원하지 XML 로더가 없습니다.");
	return false;
#endif
	
	using InsertResult = DKPair<DKHashMap<DKString, MaterialDefinition>::iterator, bool>;
	InsertResult insertResult = _materialDefinitionMap.insert(std::make_pair(materialName, std::move(materialDefinition)));
	if (insertResult.second == false)
	{
		DK_ASSERT_LOG(false, "HashMap Insert 실패!");
		return false;
	}

	return true;
}

bool SceneRenderer::initialize_createSceneConstantBuffer()
{
	DK_ASSERT_LOG(Camera::gMainCamera != nullptr, "MainCamera가 먼저 생성되어야합니다.");

	// 사실 UpdateRender함수에서 _sceneConstantBuffer를 Upload하기 때문에 여기서 Camera가 필요하진 않을 수 있음
	SceneConstantBufferStruct cameraConstanceBufferData;
	Camera::gMainCamera->GetCameraWorldMatrix(cameraConstanceBufferData.cameraWorldMatrix);
	Camera::gMainCamera->GetCameraProjectionMaterix(cameraConstanceBufferData.cameraProjectionMatrix);
	_sceneConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createUploadBuffer(&cameraConstanceBufferData, sizeof(cameraConstanceBufferData));

	return true;
}

const MaterialDefinition* SceneRenderer::getMaterialDefinition(const DKString& materialName) const
{
	using FindResult = DKHashMap<DKString, MaterialDefinition>::const_iterator;
	FindResult findResult = _materialDefinitionMap.find(materialName);
	if (findResult == _materialDefinitionMap.end())
	{
		return nullptr;
	}

	return &findResult->second;
}

void SceneRenderer::prepareShaderData() noexcept
{
	// Upload Scene ConstantBuffer
	SceneConstantBufferStruct cameraConstantBufferData;
	Camera::gMainCamera->GetCameraWorldMatrix(cameraConstantBufferData.cameraWorldMatrix);
	cameraConstantBufferData.cameraWorldMatrix.Transpose();
	Camera::gMainCamera->GetCameraProjectionMaterix(cameraConstantBufferData.cameraProjectionMatrix);
	cameraConstantBufferData.cameraProjectionMatrix.Transpose();
	_sceneConstantBuffer->upload(&cameraConstantBufferData);

	// Render Character SceneObject
	DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getCharacterSceneObjectsWritable();
	for (DKHashMap<const uint, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
	{
		SceneObject& sceneObject = iter->second;

		SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
		sceneObject._worldTransform.ToMatrix4x4(sceneObjectConstantBufferData._worldMatrix);
		sceneObjectConstantBufferData._worldMatrix.Transpose();
		sceneObject._sceneObjectConstantBuffer->upload(&sceneObjectConstantBufferData);

		uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
		for (uint i = 0; i < componentCount; ++i)
		{
			// #todo- component 완전 개편 필요해보임.
			// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
			// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
			SkinnedMeshComponent* skinnedMeshComponent = static_cast<SkinnedMeshComponent*>(sceneObject._components[i].get());

			// SkinnedMesh ConstantBuffer
			const DKVector<Matrix4x4>& currentCharacterSpaceBoneAnimation = skinnedMeshComponent->GetCurrentCharacterSpaceBoneAnimation();
			if (currentCharacterSpaceBoneAnimation.empty() == false)
			{
				Ptr<IBuffer>& skeletonBuffer = skinnedMeshComponent->getSkeletonConstantBufferWritable();
				skeletonBuffer->upload(currentCharacterSpaceBoneAnimation.data());
			}
		}
	}
}
void SceneRenderer::preRender() const noexcept
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

	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	renderModule.preRender();
}
void SceneRenderer::updateRender() noexcept
{
	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

	// Render StaticMesh SceneObject
	{
		__noop;
	}

	// Render Character SceneObject
	static const char* skinnedMeshRenderPassName = "SkinnedMeshStandardRenderPass";
	startRenderPass(renderModule, skinnedMeshRenderPassName)
	{
		setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());

		DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getCharacterSceneObjectsWritable();
		for (DKHashMap<const uint, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
		{
			SceneObject& sceneObject = iter->second;
			setConstantBuffer("SceneObjectConstantBuffer", sceneObject._sceneObjectConstantBuffer->getGPUVirtualAddress());

			uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
			for (uint i = 0; i < componentCount; ++i)
			{
				// #todo- component 완전 개편 필요해보임.
				// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
				// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
				SkinnedMeshComponent* skinnedMeshComponent = static_cast<SkinnedMeshComponent*>(sceneObject._components[i].get());

				setConstantBuffer("SkeletonConstantBuffer", skinnedMeshComponent->getSkeletonConstantBufferWritable()->getGPUVirtualAddress());

				// Material Parameters
				DKVector<SubMesh>& subMeshes = skinnedMeshComponent->GetModelWritable()->GetSubMeshesWritable();
				for (uint i = 0; i < subMeshes.size(); ++i)
				{
					SubMesh& subMesh = subMeshes[i];

					Material& material = subMesh._material;
					// 랜더패스 메터리얼 이름에 대응하는 버퍼를 알아야할듯..
					setConstantBuffer(material.getMaterialName().c_str(), material.getParameterBufferWritable()->getGPUVirtualAddress());

					renderModule.setVertexBuffers(0, 1, subMesh._vertexBufferView.get());
					renderModule.setIndexBuffer(subMesh._indexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(subMesh._indices.size()), 1, 0, 0, 0);
				}
			}
		}
	}
	endRenderPass();
}

#ifdef _DK_DEBUG_
void SceneRenderer::updateRender_Editor() noexcept
{
	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	EditorDebugDrawManager& debugDrawManager = EditorDebugDrawManager::getSingleton();
	debugDrawManager.prepareShaderData();

	static const char* debugDrawElementPassName = "DebugDrawElementRenderPass";
	startRenderPass(renderModule, debugDrawElementPassName)
	{
		setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());
		setShaderResourceView("SpherePrimitiveInfoBuffer", debugDrawManager.getSphereDebugDrawElementBufferWritable()->getGPUVirtualAddress());

		const DKVector<EditorDebugDrawManager::SpherePrimitiveInfo>& primitiveInfo = debugDrawManager.getSpherePrimitiveInfo();
		const uint32 sphereCount = static_cast<uint32>(primitiveInfo.size());
		renderModule.setVertexBuffers(0, 1, EditorDebugDrawManager::SpherePrimitiveInfo::kVertexBufferView.get());
		renderModule.setIndexBuffer(EditorDebugDrawManager::SpherePrimitiveInfo::kIndexBufferView.get());
		renderModule.drawIndexedInstanced(static_cast<UINT>(EditorDebugDrawManager::SpherePrimitiveInfo::indexCount), sphereCount, 0, 0, 0);
	}
	endRenderPass();

	debugDrawManager.endUpdateRender();
}
#endif

void SceneRenderer::EndRender() const noexcept
{
	DuckingEngine::getInstance().GetRenderModuleWritable().endRender();
}