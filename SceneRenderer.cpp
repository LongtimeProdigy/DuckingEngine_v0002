#include "stdafx.h"
#include "SceneRenderer.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "TextureManager.h"
#include "SceneObjectManager.h"
#include "Camera.h"
#include "StaticMeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"
#include "SceneObject.h"
#include "Material.h"
#include "EditorDebugDrawManager.h"
#include "SceneManager.h"

namespace DK
{
	struct SceneConstantBufferStruct
	{
		float4x4 cameraWorldMatrix;
		float4x4 cameraProjectionMatrix;
	};

	bool SceneRenderer::initialize()
	{
		if (initialize_createRenderPass() == false) 
			return false;
		if (initialize_createMaterialDefinition() == false)
			return false;
		if (initialize_createSceneConstantBuffer() == false) 
			return false;

		return true;
	}

	constexpr static const char* ShaderVariableTypeString[static_cast<uint32>(ShaderParameterType::Count)] =
	{
		"Buffer",
		"StructuredBuffer"
	};
	ShaderParameterType convertStringToEnum2(const char* str)
	{
		for (uint32 i = 0; i < static_cast<uint32>(ShaderParameterType::Count); ++i)
		{
			if (strcmp(str, ShaderVariableTypeString[i]) == 0)
			{
				return static_cast<ShaderParameterType>(i);
			}
		}

		return ShaderParameterType::Count;
	};
	bool parseShaderParameter(const TiXmlElement* variableNode, DKString& outName, ShaderParameter& outShaderParameter)
	{
		const char* parameterNameStr = variableNode->ToElement()->Attribute("Name");
		const char* parameterTypeStr = variableNode->ToElement()->Attribute("Type");
		const char* parameterRegisterStr = variableNode->ToElement()->Attribute("Register");

		DK_ASSERT_LOG(parameterNameStr != nullptr && parameterTypeStr != nullptr && parameterRegisterStr != nullptr, "RenderPass의 Parameter가 비정상인 상황. 엔진이 비정상 작동할 수 있습니다.");

		const ShaderParameterType variableType = convertStringToEnum2(parameterTypeStr);
		DK_ASSERT_LOG(variableType != ShaderParameterType::Count, "RenderPass의 ParameterType이 정확하지 않습니다.\nType: %s", parameterTypeStr);
		const uint32 variableRegister = atoi(parameterRegisterStr);

		if (variableType == ShaderParameterType::Count)
		{
			DK_ASSERT_LOG(false, "현재 지원하지 않는 MaterialType을 사용하였습니다. ParameterName: %s, ParameterType: %d", parameterNameStr, variableType);
			return false;
		}

		outName = parameterNameStr;
		outShaderParameter._type = variableType;
		outShaderParameter._register = variableRegister;

		return true;
	}
	bool SceneRenderer::initialize_createRenderPass()
	{
		ScopeString<DK_MAX_PATH> renderPassGroupPath = GlobalPath::makeResourceFullPath("RenderPass/RenderPassGroup.xml");

		TiXmlDocument doc;
		doc.LoadFile(renderPassGroupPath.c_str());
		TiXmlElement* rootNode = doc.FirstChildElement("RenderPassGroup");
		if (rootNode == nullptr) return false;

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		for (TiXmlElement* renderPassNode = rootNode->FirstChildElement(); renderPassNode != nullptr; renderPassNode = renderPassNode->NextSiblingElement())
		{
			DKString renderPassName = renderPassNode->Attribute("Name");

			RenderPass::CreateInfo renderPassCreateInfo;
			for (TiXmlElement* renderPassChildNode = renderPassNode->FirstChildElement(); renderPassChildNode != nullptr; renderPassChildNode = renderPassChildNode->NextSiblingElement())
			{
				DKString renderPassChildNodeName = renderPassChildNode->Value();
				if (renderPassChildNodeName == "Parameter")
				{
					DKString name;
					ShaderParameter shaderParameter;
					if (parseShaderParameter(renderPassChildNode, name, shaderParameter) == false)
						return false;

					renderPassCreateInfo._shaderParameterMap.insert(DKPair<DKString, ShaderParameter>(name, DK::move(shaderParameter)));
				}
				else if (renderPassChildNodeName == "Pipeline")
				{
					Pipeline::CreateInfo pipelineCreateInfo;
					DKString pipelineName = renderPassChildNode->Attribute("Name");
					pipelineCreateInfo._primitiveTopologyType = renderPassChildNode->Attribute("PrimitiveTopologyType");
					pipelineCreateInfo._depthEnable = renderPassChildNode->Attribute("DepthEnable");

					for (TiXmlElement* pipelineChildNode = renderPassChildNode->FirstChildElement(); pipelineChildNode != nullptr; pipelineChildNode = pipelineChildNode->NextSiblingElement())
					{
						if (pipelineChildNode->Type() == 2)	//NODETYPE::TINYXML_COMMENT
							continue;

						DKString pipelineChildNodeName = pipelineChildNode->Value();
						if (pipelineChildNodeName == "LayoutInfo")
						{
							for (TiXmlElement* layoutElement = pipelineChildNode->FirstChildElement(); layoutElement != nullptr; layoutElement = layoutElement->NextSiblingElement())
							{
								DKString layoutElementType = layoutElement->Attribute("Type");
								DKString layoutElementName = layoutElement->GetText();
								DK_ASSERT_LOG(layoutElementType.empty() == false, "Type이 비어있으면 안됨");
								if (layoutElementType == "uint4")
									pipelineCreateInfo._layout.push_back({ Pipeline::CreateInfo::LayoutInfo::Type::UINT4, layoutElementName });
								else if (layoutElementType == "float2")
									pipelineCreateInfo._layout.push_back({ Pipeline::CreateInfo::LayoutInfo::Type::FLOAT2, layoutElementName });
								else if (layoutElementType == "float3")
									pipelineCreateInfo._layout.push_back({ Pipeline::CreateInfo::LayoutInfo::Type::FLOAT3, layoutElementName });
								else if (layoutElementType == "float4")
									pipelineCreateInfo._layout.push_back({ Pipeline::CreateInfo::LayoutInfo::Type::FLOAT4, layoutElementName });
							}
						}
						else if (pipelineChildNodeName == "VertexShader")
						{
							pipelineCreateInfo._vertexShaderEntry = pipelineChildNode->Attribute("Entry");
							pipelineCreateInfo._vertexShaderPath = pipelineChildNode->GetText();
						}
						else if (pipelineChildNodeName == "PixelShader")
						{
							pipelineCreateInfo._pixelShaderEntry = pipelineChildNode->Attribute("Entry");
							pipelineCreateInfo._pixelShaderPath = pipelineChildNode->GetText();
						}
						else if (pipelineChildNodeName == "Parameter")
						{
							DKString name;
							ShaderParameter shaderParameter;
							if (parseShaderParameter(pipelineChildNode, name, shaderParameter) == false)
								return false;

							pipelineCreateInfo._shaderParameterMap.insert(DKPair<DKString, ShaderParameter>(name, DK::move(shaderParameter)));
						}
						else
						{
							DK_ASSERT_LOG(false, "지원하지 Pipeline ChildNode입니다. NodeName: %s", pipelineChildNodeName.c_str());
							return false;
						}

					}

					renderPassCreateInfo._pipelineArr.push_back(std::make_pair(pipelineName, DK::move(pipelineCreateInfo)));
				}
				else
				{
					DK_ASSERT_LOG(false, "지원하지 RenderPass ChildNode입니다. NodeName: %s", renderPassChildNodeName.c_str());
					return false;
				}
			}

			if (renderModule.createRenderPass(renderPassName, DK::move(renderPassCreateInfo)) == false)
				return false;
		}

		return true;
	}
	bool SceneRenderer::initialize_createMaterialDefinition()
	{
		ScopeString<DK_MAX_PATH> materialDefinitionFilePath = GlobalPath::makeResourceFullPath("Material/MaterialDefinition.xml");

		TiXmlDocument materialDefinitionDocument;
		if (materialDefinitionDocument.LoadFile(materialDefinitionFilePath.c_str()) == false)
		{
			DK_ASSERT_LOG(false, "MaterialDefinition XML 파일의 경로가 올바르지 않습니다.");
			return false;
		}
		TiXmlElement* materialDefinitionRootNode = materialDefinitionDocument.RootElement();
		for (TiXmlElement* materialNode = materialDefinitionRootNode->FirstChildElement(); materialNode != nullptr; materialNode = materialNode->NextSiblingElement())
		{
			ScopeString<DK_MAX_BUFFER> materialNodeName = materialNode->Value();
			if (materialNodeName != "Material")
			{
				DK_ASSERT_LOG(false, "MaterialDefinition의 Child중 올바르지 않은 Node가 있습니다. NodeName: %s", materialNodeName.c_str());
				continue;
			}

			ScopeString<DK_MAX_PATH> materialDefinitionGroupPass = GlobalPath::makeResourceFullPath(materialNode->Attribute("Path"));

			TiXmlDocument doc;
			doc.LoadFile(materialDefinitionGroupPass.c_str());
			TiXmlElement* rootNode = doc.FirstChildElement("Material");
			if (rootNode == nullptr)
				return false;

			MaterialDefinition materialDefinition;
			const DKString materialName = rootNode->Attribute("Name");

			using FindResult = DKHashMap<DKString, MaterialDefinition>::iterator;
			FindResult findResult = _materialDefinitionMap.find(materialName);
			if (findResult != _materialDefinitionMap.end())
			{
				DK_ASSERT_LOG(false, "중복된 이름의 MaterialDefinition이 있습니다.");
				return false;
			}

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

				materialDefinition._parameters.push_back(DK::move(parameterDefinition));
			}

			using InsertResult = DKPair<DKHashMap<DKString, MaterialDefinition>::iterator, bool>;
			InsertResult insertResult = _materialDefinitionMap.insert(std::make_pair(materialName, DK::move(materialDefinition)));
			if (insertResult.second == false)
			{
				DK_ASSERT_LOG(false, "HashMap Insert 실패!");
				return false;
			}
		}

		return true;
	}
	bool SceneRenderer::initialize_createSceneConstantBuffer()
	{
		DK_ASSERT_LOG(Camera::gMainCamera != nullptr, "MainCamera가 먼저 생성되어야합니다.");

		// 사실 UpdateRender함수에서 _sceneConstantBuffer를 Upload하기 때문에 여기서 Camera가 필요하진 않을 수 있음
		SceneConstantBufferStruct cameraConstanceBufferData;
		Camera::gMainCamera->getCameraWorldMatrix(cameraConstanceBufferData.cameraWorldMatrix);
		Camera::gMainCamera->getCameraProjectionMatrix(cameraConstanceBufferData.cameraProjectionMatrix);
		_sceneConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createUploadBuffer(sizeof(cameraConstanceBufferData));

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
		Camera::gMainCamera->getCameraWorldMatrix(cameraConstantBufferData.cameraWorldMatrix);
		Camera::gMainCamera->getCameraProjectionMatrix(cameraConstantBufferData.cameraProjectionMatrix);
		_sceneConstantBuffer->upload(&cameraConstantBufferData);

		// Render Character SceneObject
		DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getSceneObjectsWritable();
		for (DKHashMap<const uint32, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
		{
			SceneObject& sceneObject = iter->second;

			SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
			sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
			sceneObject._sceneObjectConstantBuffer->upload(&sceneObjectConstantBufferData);
		}

		// Render Character SceneObject
		DKHashMap<uint32, SceneObject>& characterSceneObjectArr = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getCharacterSceneObjectsWritable();
		for (DKHashMap<const uint32, SceneObject>::iterator iter = characterSceneObjectArr.begin(); iter != characterSceneObjectArr.end(); ++iter)
		{
			SceneObject& sceneObject = iter->second;

			SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
			sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
			sceneObject._sceneObjectConstantBuffer->upload(&sceneObjectConstantBufferData);

			uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
			for (uint32 i = 0; i < componentCount; ++i)
			{
				// #todo- component 완전 개편 필요해보임.
				// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
				// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
				SkinnedMeshComponent* skinnedMeshComponent = static_cast<SkinnedMeshComponent*>(sceneObject._components[i].get());

				// SkinnedMesh ConstantBuffer
				const DKVector<float4x4>& currentCharacterSpaceBoneAnimation = skinnedMeshComponent->get_currentCharacterSpaceBoneAnimation();
				if (currentCharacterSpaceBoneAnimation.empty() == false)
				{
					Ptr<IBuffer>& skeletonBuffer = skinnedMeshComponent->get_skeletonConstantBufferWritable();
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
			const float3& cameraPosition = Camera::gMainCamera->get_worldTransform().get_translation();
			sprintf_s(cameraPosBuffer, MAX_BUFFER_LENGTH, "Position: x: %f, y: %f, z: %f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
			ImGui::Text(cameraPosBuffer);

			char cameraRotationBuffer[MAX_BUFFER_LENGTH];
			const Quaternion& cameraRotation = Camera::gMainCamera->get_worldTransform().get_rotation();
			sprintf_s(cameraRotationBuffer, MAX_BUFFER_LENGTH, "Rotation: x: %f, y: %f, z: %f, w: %f", cameraRotation.x, cameraRotation.y, cameraRotation.z, cameraRotation.w);
			ImGui::Text(cameraRotationBuffer);

			char cameraRotationEulerBuffer[MAX_BUFFER_LENGTH];
			float3 eulerRotation;
			cameraRotation.toEuler(eulerRotation);
			sprintf_s(cameraRotationEulerBuffer, MAX_BUFFER_LENGTH, "Rotation: roll: %f, yaw: %f, pitch: %f", eulerRotation.x, eulerRotation.y, eulerRotation.z);
			ImGui::Text(cameraRotationEulerBuffer);

			char cameraRotationMatrixBuffer[MAX_BUFFER_LENGTH];
			float4x4 transformMatrix;
			Camera::gMainCamera->get_worldTransform().tofloat4x4(transformMatrix);
			sprintf_s(cameraRotationMatrixBuffer, MAX_BUFFER_LENGTH, "%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f",
				transformMatrix._11, transformMatrix._12, transformMatrix._13, transformMatrix._14,
				transformMatrix._21, transformMatrix._22, transformMatrix._23, transformMatrix._24,
				transformMatrix._31, transformMatrix._32, transformMatrix._33, transformMatrix._34, 
				transformMatrix._41, transformMatrix._42, transformMatrix._43, transformMatrix._44
			);
			ImGui::Text(cameraRotationMatrixBuffer);

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

		// Render Character SceneObject
		startRenderPass(renderModule, "MainRenserPass");

		startPipeline("SkyDomePipeline");
		{
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());

			SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
			SceneManager::SkyDome& skyDome = sceneManager.getSkyDomeWritable();

			renderModule.setVertexBuffers(0, 1, skyDome._vertexBufferView.get());
			renderModule.setIndexBuffer(skyDome._indexBufferView.get());
			renderModule.drawIndexedInstanced(static_cast<UINT>(skyDome._indexCount), 1, 0, 0, 0);
		}
		endPipeline();

		startPipeline("TerrainClipmapPipeline");
		{
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());

			SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
			DKVector<SceneManager::ClipMapTerrain>& terrainContainer = sceneManager.getClipMapTerrainContainerWritable();

			SceneManager::ClipMapTerrain& terrain = terrainContainer[0];
			Material* material = terrain._material.get();
			setConstantBuffer(material->get_materialName().c_str(), material->get_parameterBufferForGPUWritable()->getGPUVirtualAddress());

			const Transform& cameraMatrix = Camera::gMainCamera->get_worldTransform();
			float2 cameraPos = float2::Zero;
			cameraPos = float2(cameraMatrix.get_translation().x, cameraMatrix.get_translation().z);
			//cameraPos = float2(0, 0);

			uint32 terrainConstantBufferTypeIndex = 0;
			float vertexScale = 0.125f;
			// Draw Cross
			{
				float tileScale = vertexScale;
				float gridScale = static_cast<float>(SceneManager::TILE_RESOLUTION * tileScale);
				float2 snappedCameraPos = DK::Math::floor(cameraPos / tileScale) * tileScale;

				TerrainMeshConstantBuffer meshCBuffer;
				meshCBuffer._baseXY_scale_rotate = float4(snappedCameraPos.x, snappedCameraPos.y, vertexScale, 0.0f);
				meshCBuffer._type = 0;
				Ptr<IBuffer>& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
				terrainConstantBuffer->upload(&meshCBuffer);
				setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer->getGPUVirtualAddress());

				renderModule.setVertexBuffers(0, 1, terrain._cross._vertexBufferView.get());
				renderModule.setIndexBuffer(terrain._cross._indexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._cross._indexCount), 1, 0, 0, 0);
			}

			for (uint32 i = 0; i < SceneManager::NUM_CLIPMAP_LEVELS; ++i)
			{
				float tileScale = (1 << i) * vertexScale;
				float gridScale = static_cast<float>(SceneManager::TILE_RESOLUTION * tileScale);
				float2 snappedCameraPos = DK::Math::floor(cameraPos / tileScale) * tileScale;

				float2 base = snappedCameraPos - gridScale * 2;

				// Draw Tile
				for (uint32 x = 0; x < 4; ++x)
				{
					for (uint32 y = 0; y < 4; ++y)
					{
						if (i != 0 && (x == 1 || x == 2) && (y == 1 || y == 2))
							continue;

						float2 fill = float2(x >= 2 ? tileScale : 0, y >= 2 ? tileScale : 0);
						float2 tile_bl = base + float2(x, y) * gridScale + fill;

						TerrainMeshConstantBuffer meshCBuffer;
						meshCBuffer._baseXY_scale_rotate = float4(tile_bl.x, tile_bl.y, tileScale, 0.0f);
						meshCBuffer._type = 1;
						Ptr<IBuffer>& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
						terrainConstantBuffer->upload(&meshCBuffer);
						setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer->getGPUVirtualAddress());

						renderModule.setVertexBuffers(0, 1, terrain._tile._vertexBufferView.get());
						renderModule.setIndexBuffer(terrain._tile._indexBufferView.get());
						renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._tile._indexCount), 1, 0, 0, 0);
					}
				}

				// Draw Filter
				{
					TerrainMeshConstantBuffer meshCBuffer;
					meshCBuffer._baseXY_scale_rotate = float4(snappedCameraPos.x, snappedCameraPos.y, tileScale, 0.0f);
					meshCBuffer._type = 2;
					Ptr<IBuffer>& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
					terrainConstantBuffer->upload(&meshCBuffer);
					setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer->getGPUVirtualAddress());

					renderModule.setVertexBuffers(0, 1, terrain._filter._vertexBufferView.get());
					renderModule.setIndexBuffer(terrain._filter._indexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._filter._indexCount), 1, 0, 0, 0);
				}

				float nextTileScale = tileScale * 2.0f;
				float nextGridScale = gridScale * 1;
				float2 nextSnappedPos = DK::Math::floor(cameraPos / nextTileScale) * nextTileScale;
				// Draw Seam
				if (i != SceneManager::NUM_CLIPMAP_LEVELS)
				{
					float2 next_base = nextSnappedPos - nextGridScale * 2;

					TerrainMeshConstantBuffer meshCBuffer;
					meshCBuffer._baseXY_scale_rotate = float4(next_base.x, next_base.y, tileScale, 0.0f);
					meshCBuffer._type = 3;
					Ptr<IBuffer>& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
					terrainConstantBuffer->upload(&meshCBuffer);
					setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer->getGPUVirtualAddress());

					renderModule.setVertexBuffers(0, 1, terrain._seam._vertexBufferView.get());
					renderModule.setIndexBuffer(terrain._seam._indexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._seam._indexCount), 1, 0, 0, 0);
				}

				// Draw Trim
				{
					float2 tile_centre = snappedCameraPos - float2(tileScale * 0.5f);

					float2 d = cameraPos - nextSnappedPos;
					uint32 r = 0;
					r |= d.x < tileScale ? 2 : 0;
					r |= d.y < tileScale ? 1 : 0;
					TerrainMeshConstantBuffer meshCBuffer;
					meshCBuffer._baseXY_scale_rotate = float4(snappedCameraPos + tileScale * 0.5f, tileScale, r);
					meshCBuffer._type = 4;
					Ptr<IBuffer>& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
					terrainConstantBuffer->upload(&meshCBuffer);
					setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer->getGPUVirtualAddress());

					renderModule.setVertexBuffers(0, 1, terrain._trim._vertexBufferView.get());
					renderModule.setIndexBuffer(terrain._trim._indexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._trim._indexCount), 1, 0, 0, 0);
				}
			}
		}
		endPipeline();

		startPipeline("StaticMeshStandardPipeline");
		{
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());

			DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getSceneObjectsWritable();
			for (DKHashMap<const uint32, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
			{
				SceneObject& sceneObject = iter->second;
				setConstantBuffer("SceneObjectConstantBuffer", sceneObject._sceneObjectConstantBuffer->getGPUVirtualAddress());

				uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
				for (uint32 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
				{
					// #todo- component 완전 개편 필요해보임.
					// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
					// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
					StaticMeshComponent* staticMeshComponent = static_cast<StaticMeshComponent*>(sceneObject._components[componentIndex].get());

					DKVector<StaticMeshModel::SubMeshType>& subMeshes = staticMeshComponent->get_modelWritable()->get_subMeshArrWritable();
					for (uint32 subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
					{
						StaticMeshModel::SubMeshType& subMesh = subMeshes[subMeshIndex];

						Material* material = subMesh._material.get();
						setConstantBuffer(material->get_materialName().c_str(), material->get_parameterBufferForGPUWritable()->getGPUVirtualAddress());

						//renderModule.setVertexBuffers(0, 1, subMesh._vertexBufferView.get());
						//renderModule.setIndexBuffer(subMesh._indexBufferView.get());
						//renderModule.drawIndexedInstanced(static_cast<UINT>(subMesh._indices.size()), 1, 0, 0, 0);
					}
				}
			}
		}
		endPipeline();

		startPipeline("SkinnedMeshStandardPipeline");
		{
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());

			DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getCharacterSceneObjectsWritable();
			for (DKHashMap<const uint32, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
			{
				SceneObject& sceneObject = iter->second;
				setConstantBuffer("SceneObjectConstantBuffer", sceneObject._sceneObjectConstantBuffer->getGPUVirtualAddress());

				uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
				for (uint32 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
				{
					// #todo- component 완전 개편 필요해보임.
					// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
					// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
					SkinnedMeshComponent* skinnedMeshComponent = static_cast<SkinnedMeshComponent*>(sceneObject._components[componentIndex].get());

					setConstantBuffer("SkeletonConstantBuffer", skinnedMeshComponent->get_skeletonConstantBufferWritable()->getGPUVirtualAddress());

					DKVector<SkinnedMeshModel::SubMeshType>& subMeshes = skinnedMeshComponent->get_modelWritable()->get_subMeshArrWritable();
					for (uint32 subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
					{
						SkinnedMeshModel::SubMeshType& subMesh = subMeshes[subMeshIndex];

						Material* material = subMesh._material.get();
						setConstantBuffer(material->get_materialName().c_str(), material->get_parameterBufferForGPUWritable()->getGPUVirtualAddress());

						renderModule.setVertexBuffers(0, 1, subMesh._vertexBufferView.get());
						renderModule.setIndexBuffer(subMesh._indexBufferView.get());
						renderModule.drawIndexedInstanced(static_cast<UINT>(subMesh._indices.size()), 1, 0, 0, 0);
					}
				}
			}
		}
		endPipeline();
		endRenderPass();
	}

#ifdef _DK_DEBUG_
	void SceneRenderer::updateRender_Editor() noexcept
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		EditorDebugDrawManager& debugDrawManager = EditorDebugDrawManager::getSingleton();
		debugDrawManager.prepareShaderData();

		startRenderPass(renderModule, "EditorMainRenserPass");
		{
			startPipeline("SpherePipeline");
			{
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());
				setShaderResourceView("SpherePrimitiveInfoBuffer", debugDrawManager.get_primitiveInfoSphereBufferWritable()->getGPUVirtualAddress());

				const DKVector<EditorDebugDrawManager::SpherePrimitiveInfo>& primitiveInfo = debugDrawManager.get_primitiveInfoSphereArr();
				const uint32 instanceCount = static_cast<uint32>(primitiveInfo.size());
				renderModule.setVertexBuffers(0, 1, EditorDebugDrawManager::SpherePrimitiveInfo::kVertexBufferView.get());
				renderModule.setIndexBuffer(EditorDebugDrawManager::SpherePrimitiveInfo::kIndexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(EditorDebugDrawManager::SpherePrimitiveInfo::indexCount), instanceCount, 0, 0, 0);
			}
			endPipeline();
			startPipeline("LinePipeline");
			{
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->getGPUVirtualAddress());
				setShaderResourceView("LinePrimitiveInfoBuffer", debugDrawManager.get_primitiveInfoLineBufferWritable()->getGPUVirtualAddress());

				const DKVector<EditorDebugDrawManager::LinePrimitiveInfo>& primitiveInfo = debugDrawManager.get_primitiveInfoLineArr();
				const uint32 instanceCount = static_cast<uint32>(primitiveInfo.size());
				renderModule.setVertexBuffers(0, 1, EditorDebugDrawManager::LinePrimitiveInfo::kVertexBufferView.get());
				renderModule.setIndexBuffer(EditorDebugDrawManager::LinePrimitiveInfo::kIndexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(EditorDebugDrawManager::LinePrimitiveInfo::indexCount), instanceCount, 0, 0, 0);
			}
			endPipeline();
		}
		endRenderPass();

		debugDrawManager.endUpdateRender();
	}
#endif

	void SceneRenderer::endRender() const noexcept
	{
		DuckingEngine::getInstance().GetRenderModuleWritable().endRender();
	}
}