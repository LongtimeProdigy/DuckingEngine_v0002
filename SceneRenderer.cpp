#include "stdafx.h"
#include "SceneRenderer.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "RaytracingRenderer.h"
#include "SceneObjectManager.h"
#include "Camera.h"
#include "StaticMeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "Model.h"
#include "SceneObject.h"
#include "Material.h"
#include "EditorDebugDrawManager.h"
#include "SceneManager.h"
#include "ShaderCompiler.h"

#include <random>

namespace DK
{
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

	const bool parseRootConstant32BitParameter(TiXmlElement* pipelineChildNode, const ScopeString<DK_MAX_PATH>& renderPassGroupPath, Pipeline::CreateInfo& pipelineCreateInfo)
	{
		const char* bufferName = pipelineChildNode->Attribute("Buffer");
		if (bufferName == nullptr || bufferName[0] == '\0')
		{
			DK_ASSERT_LOG(false, "RootConstant항목에는 반드시 Buffer Attribute가 존재해야합니다.\nFile: %s", renderPassGroupPath.c_str());
			return false;
		}

		RootConstant32BitParameter parameter;
		parameter._bufferName = bufferName;
		pipelineCreateInfo._rootConstant32BitParameter.push_back(DK::move(parameter));

		return true;
	}

	bool appendPipelinePermutations(const TiXmlElement& node, const DKString& name, const Pipeline::CreateInfo& base, RenderPass::CreateInfo& renderPass)
	{
		DKVector<DKVector<DKString>> combinations;
		DKString error;
		if (!parsePipelinePermutations(node, combinations, error))
		{
			DK_ASSERT_LOG(false, "Pipeline %s: %s", name.c_str(), error.c_str());
			return false;
		}
		for (uint32 index = 0; index < combinations.size(); ++index)
		{
			Pipeline::CreateInfo variant = base;
			variant._defines = DK::move(combinations[index]);
			const DKString variantName = pipelinePermutationName(name, index);
#if defined(_DK_DEBUG_)
			variant._pipelineName = variantName;
#endif
			renderPass._pipelineArr.push_back(std::make_pair(variantName, DK::move(variant)));
		}
		return true;
	}

	bool SceneRenderer::initialize_createRenderPass()
	{
		ScopeString<DK_MAX_PATH> renderPassGroupPath = GlobalPath::makeResourceFullPath("RenderPass/RenderPassGroup.xml");

		TiXmlDocument doc;
		doc.LoadFile(renderPassGroupPath.c_str());
		TiXmlElement* rootNode = doc.FirstChildElement("RenderPassGroup");
		if (rootNode == nullptr)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		ShaderCompiler shaderCompiler;
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		for (TiXmlElement* renderPassNode = rootNode->FirstChildElement(); renderPassNode != nullptr; renderPassNode = renderPassNode->NextSiblingElement())
		{
			DKString renderPassName = renderPassNode->Attribute("Name");

			RenderPass::CreateInfo renderPassCreateInfo;
			for (TiXmlElement* renderPassChildNode = renderPassNode->FirstChildElement(); renderPassChildNode != nullptr; renderPassChildNode = renderPassChildNode->NextSiblingElement())
			{
				DKString renderPassChildNodeName = renderPassChildNode->Value();
				if (renderPassChildNodeName == "Pipeline")
				{
					Pipeline::CreateInfo pipelineCreateInfo;
					DKString pipelineName = renderPassChildNode->Attribute("Name");
					const char* targetFormat = renderPassChildNode->Attribute("RenderTargetFormat");
					if (targetFormat != nullptr)
					{
						if (StringUtil::strcmp(targetFormat, "LDR") == 0)
							pipelineCreateInfo._hdrRenderTarget = false;
						else if (StringUtil::strcmp(targetFormat, "HDR") != 0)
							return false;
					}
					pipelineCreateInfo._primitiveTopologyType = renderPassChildNode->Attribute("PrimitiveTopologyType");
					const char* depthEnable = renderPassChildNode->Attribute("DepthEnable");
					pipelineCreateInfo._depthEnable = depthEnable == nullptr || StringUtil::strcmp(depthEnable, "True") != 0 ? false : true;
					const char* fillMode = renderPassChildNode->Attribute("FillMode");
					pipelineCreateInfo._fillMode = fillMode == nullptr || StringUtil::strcmp(fillMode, "WireFrame") != 0 ? Pipeline::CreateInfo::FillMode::SOLID : Pipeline::CreateInfo::FillMode::WIREFRAME;
					const char* cullMode = renderPassChildNode->Attribute("CullMode");
					pipelineCreateInfo._cullMode = Pipeline::CreateInfo::CullMode::BACK;
					if (cullMode != nullptr)
					{
						if (StringUtil::strcmp(cullMode, "Back") == 0)
						{
							pipelineCreateInfo._cullMode = Pipeline::CreateInfo::CullMode::BACK;
						}
						else if (StringUtil::strcmp(cullMode, "Front") == 0)
						{
							pipelineCreateInfo._cullMode = Pipeline::CreateInfo::CullMode::FRONT;
						}
						else if (StringUtil::strcmp(cullMode, "None") == 0)
						{
							pipelineCreateInfo._cullMode = Pipeline::CreateInfo::CullMode::NONE;
						}
						else
						{
							DK_ASSERT_LOG(false, "Incorrect Pipeline CullMode(%s).", cullMode);
							return false;
						}
					}

					for (TiXmlElement* pipelineChildNode = renderPassChildNode->FirstChildElement(); pipelineChildNode != nullptr; pipelineChildNode = pipelineChildNode->NextSiblingElement())
					{
						if (pipelineChildNode->Type() == 2)	//NODETYPE::TINYXML_COMMENT
							continue;

						DKString pipelineChildNodeName = pipelineChildNode->Value();
						if (pipelineChildNodeName == "Permutation")
							continue;
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
								else
								{
									DK_ASSERT_LOG(false, "");
									return false;
								}
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
						else if (pipelineChildNodeName == "RootConstant")
						{
							if (parseRootConstant32BitParameter(pipelineChildNode, renderPassGroupPath, pipelineCreateInfo) == false)
							{
								return false;
							}
						}
						else
						{
							DK_ASSERT_LOG(false, "지원하지 Pipeline ChildNode입니다. NodeName: %s", pipelineChildNodeName.c_str());
							return false;
						}
					}

					if (!appendPipelinePermutations(*renderPassChildNode, pipelineName, pipelineCreateInfo, renderPassCreateInfo))
						return false;
				}
				else if (renderPassChildNodeName == "ComputePipeline")
				{
					Pipeline::CreateInfo pipelineCreateInfo;
					DKString pipelineName = renderPassChildNode->Attribute("Name");

					for (TiXmlElement* pipelineChildNode = renderPassChildNode->FirstChildElement(); pipelineChildNode != nullptr; pipelineChildNode = pipelineChildNode->NextSiblingElement())
					{
						if (pipelineChildNode->Type() == 2)	//NODETYPE::TINYXML_COMMENT
							continue;

						DKString pipelineChildNodeName = pipelineChildNode->Value();
						if (pipelineChildNodeName == "Permutation")
							continue;
						if (pipelineChildNodeName == "Shader")
						{
							pipelineCreateInfo._computeShaderEntry = pipelineChildNode->Attribute("Entry");
							pipelineCreateInfo._computeShaderPath = pipelineChildNode->GetText();
						}
						else if (pipelineChildNodeName == "RootConstant")
						{
							if (parseRootConstant32BitParameter(pipelineChildNode, renderPassGroupPath, pipelineCreateInfo) == false)
							{
								return false;
							}
						}
						else
						{
							DK_ASSERT_LOG(false, "지원하지 ComputePipeline ChildNode입니다. NodeName: %s", pipelineChildNodeName.c_str());
							return false;
						}
					}

					if (!appendPipelinePermutations(*renderPassChildNode, pipelineName, pipelineCreateInfo, renderPassCreateInfo))
						return false;
				}
				else if (renderPassChildNodeName == "RaytracingPipeline")
				{
					Pipeline::CreateInfo pipelineCreateInfo;
					DKString pipelineName = renderPassChildNode->Attribute("Name");
#if defined(_DK_DEBUG_)
					pipelineCreateInfo._pipelineName = pipelineName;
#endif
					if (const char* hitGroup = renderPassChildNode->Attribute("HitGroup"))
						pipelineCreateInfo._hitGroupExport = hitGroup;
					const char* configNames[] = { "MaxPayloadSize", "MaxAttributeSize", "MaxTraceRecursionDepth" };
					uint32* configValues[] = { &pipelineCreateInfo._maxPayloadSize, &pipelineCreateInfo._maxAttributeSize, &pipelineCreateInfo._maxTraceRecursionDepth };
					for (uint32 i = 0; i < DK_COUNT_OF(configNames); ++i)
					{
						int value = 0;
						const int result = renderPassChildNode->QueryIntAttribute(configNames[i], &value);
						if (result == TIXML_NO_ATTRIBUTE)
							continue;
						if (result != TIXML_SUCCESS || value < 0)
							return false;
						*configValues[i] = static_cast<uint32>(value);
					}

					for (TiXmlElement* pipelineChildNode = renderPassChildNode->FirstChildElement(); pipelineChildNode != nullptr; pipelineChildNode = pipelineChildNode->NextSiblingElement())
					{
						if (pipelineChildNode->Type() == 2)	//NODETYPE::TINYXML_COMMENT
							continue;

						DKString pipelineChildNodeName = pipelineChildNode->Value();
						if (pipelineChildNodeName == "Permutation")
							continue;
						if (pipelineChildNodeName == "RaygenShader")
						{
							pipelineCreateInfo._raygenEntry = pipelineChildNode->Attribute("Entry");
							pipelineCreateInfo._raygenShaderPath = pipelineChildNode->GetText();
						}
						else if (pipelineChildNodeName == "MissShader")
						{
							pipelineCreateInfo._missEntry = pipelineChildNode->Attribute("Entry");
							pipelineCreateInfo._missShaderPath = pipelineChildNode->GetText();
						}
						else if (pipelineChildNodeName == "ClosestHitShader")
						{
							pipelineCreateInfo._closestEntry = pipelineChildNode->Attribute("Entry");
							pipelineCreateInfo._closestShaderPath = pipelineChildNode->GetText();
						}
						else if (pipelineChildNodeName == "RootConstant")
						{
							if (parseRootConstant32BitParameter(pipelineChildNode, renderPassGroupPath, pipelineCreateInfo) == false)
							{
								return false;
							}
						}
						else
						{
							DK_ASSERT_LOG(false, "지원하지 ComputePipeline ChildNode입니다. NodeName: %s", pipelineChildNodeName.c_str());
							return false;
						}
					}

					if (!appendPipelinePermutations(*renderPassChildNode, pipelineName, pipelineCreateInfo, renderPassCreateInfo))
						return false;
				}
				else
				{
					DK_ASSERT_LOG(false, "지원하지 RenderPass ChildNode입니다. NodeName: %s", renderPassChildNodeName.c_str());
					return false;
				}
			}

			if (renderModule.createRenderPass(shaderCompiler, renderPassName, DK::move(renderPassCreateInfo)) == false)
				return false;
		}

		return true;
	}
	bool SceneRenderer::initialize_createMaterialDefinition()
	{
		ScopeString<DK_MAX_PATH> materialDefinitionFilePath = GlobalPath::makeResourceFullPath(ConstPath::gMaterialDefinition);

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
				DK_ASSERT_LOG(false, "MaterialDefinition의 Child중 올바르지 않은 Node가 있습니다.");
				continue;
			}

			ScopeString<DK_MAX_PATH> materialDefinitionPath = GlobalPath::makeResourceFullPath(materialNode->Attribute("Path"));

			TiXmlDocument doc;
			doc.LoadFile(materialDefinitionPath.c_str());
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

		_sceneConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createConstantBuffer(sizeof(SceneConstantBuffer), L"SceneConstantBuffer");
		_atmosphereConstantBuffer = DuckingEngine::getInstance().GetRenderModuleWritable().createConstantBuffer(sizeof(AtmosphereConstantBuffer), L"AtmosphereConstantBuffer");

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
	static float gHeightScale = 750.f;	// TODO: Ocean쪽으로 옮겨야함
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::normal_distribution<float> dist(0, 1);
	void SceneRenderer::updateAtmosphereCamera(Camera& camera) const noexcept
	{
		const auto& atmosphere = _atmosphereConstantBufferData;
		const float3 center(atmosphere._planetCentre[0], atmosphere._planetCentre[1], atmosphere._planetCentre[2]);
		const Transform& transform = camera.get_worldTransform();
		const float3 position = transform.get_translation();
		const double x = static_cast<double>(position.x) - center.x;
		const double y = static_cast<double>(position.y) - center.y;
		const double z = static_cast<double>(position.z) - center.z;
		const double distance = std::sqrt(x * x + y * y + z * z);
		const double minimumRadius = static_cast<double>(atmosphere._planetRadius) + 2.0;
		if (distance < minimumRadius)
		{
			const double factor = distance > 0.0 ? minimumRadius / distance : 0.0;
			const float3 correctedPosition = distance > 0.0
				? float3(static_cast<float>(center.x + x * factor), static_cast<float>(center.y + y * factor), static_cast<float>(center.z + z * factor))
				: center + float3(0, static_cast<float>(minimumRadius), 0);
			camera.set_worldTransform(Transform(correctedPosition, transform.get_rotation(), transform.get_scale()));
			DuckingEngine::getInstance().GetRaytracingRendererWritable().resetAccumulation();
		}
		const float altitude = static_cast<float>(std::max(distance, minimumRadius) - atmosphere._planetRadius);
		camera.setMoveSpeed(Math::clamp(altitude * 0.2f, 5.0f, 2000000.0f));
		camera.setClipPlaneDistances(Math::max(0.1f, altitude * 0.00001f), Math::max(1000.0f, altitude * 4.0f));
	}
	void SceneRenderer::setOrbitView() const noexcept
	{
		const auto& atmosphere = _atmosphereConstantBufferData;
		const float sunAngle = atmosphere._sunDegree * Math::kToRadian;
		const float earthAngle = sunAngle - 25.0f * Math::kToRadian;
		const float3 earthDirection(0, std::sin(earthAngle), std::cos(earthAngle));
		const float3 sunDirection(0, std::sin(sunAngle), std::cos(sunAngle));
		const float3 center(atmosphere._planetCentre[0], atmosphere._planetCentre[1], atmosphere._planetCentre[2]);
		Camera::gMainCamera->setView(center - earthDirection * (4.0f * atmosphere._atmosphereRadius), earthDirection + sunDirection);
		updateAtmosphereCamera(*Camera::gMainCamera);
	}
	void SceneRenderer::prepareShaderData(const float deltaTime) noexcept
	{
		// UI may have changed the view or atmosphere since the input update.
		updateAtmosphereCamera(*Camera::gMainCamera);
		const float3 cameraPosition = Camera::gMainCamera->get_worldTransform().get_translation();
		const auto& atmosphere = _atmosphereConstantBufferData;
		const float3 planetCenter(atmosphere._planetCentre[0], atmosphere._planetCentre[1], atmosphere._planetCentre[2]);
		const double dx = static_cast<double>(cameraPosition.x) - planetCenter.x;
		const double dy = static_cast<double>(cameraPosition.y) - planetCenter.y;
		const double dz = static_cast<double>(cameraPosition.z) - planetCenter.z;
		_atmosphereConstantBufferData._cameraAltitude = static_cast<float>(std::sqrt(dx * dx + dy * dy + dz * dz) - atmosphere._planetRadius);
		// Existing flat terrain is a local patch at the planet's +Y surface.
		const double patchX = static_cast<double>(cameraPosition.x) - planetCenter.x;
		const double patchZ = static_cast<double>(cameraPosition.z) - planetCenter.z;
		_renderLocalScene = atmosphere._cameraAltitude < 10000.0f && cameraPosition.y > planetCenter.y
			&& patchX * patchX + patchZ * patchZ < 50000.0 * 50000.0;
		// Upload Scene ConstantBuffer
		{
			_sceneConstantBufferData._frameIndex = RenderModule::kCurrentFrameIndex;
			_sceneConstantBufferData._resolution[0] = RenderModule::kWidth;
			_sceneConstantBufferData._resolution[1] = RenderModule::kHeight;
			_sceneConstantBufferData._time += deltaTime;
			_sceneConstantBufferData._nearDistance = Camera::gMainCamera->getNearPlaneDistance();
			_sceneConstantBufferData._farDistance = Camera::gMainCamera->getFarPlaneDistance();
			_sceneConstantBufferData._rngState = static_cast<uint32>(gen());
			Camera::gMainCamera->get_worldTransform().tofloat4x4(_sceneConstantBufferData._cameraWorldMatrix);
			Camera::gMainCamera->getCameraWorldMatrix(_sceneConstantBufferData._cameraWorldMatrixInv);
			Camera::gMainCamera->getCameraProjectionMatrix(_sceneConstantBufferData._cameraProjectionMatrix);
			_sceneConstantBuffer->upload(&_sceneConstantBufferData);

			AtmosphereConstantBuffer relativeAtmosphere = atmosphere;
			relativeAtmosphere._planetCentre[0] = static_cast<float>(-dx);
			relativeAtmosphere._planetCentre[1] = static_cast<float>(-dy);
			relativeAtmosphere._planetCentre[2] = static_cast<float>(-dz);
			_atmosphereConstantBuffer->upload(&relativeAtmosphere);
		}

		// Upload Ocean ConstantBuffer
		if (_renderLocalScene)
		{
			SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
			SceneManager::Ocean& ocean = sceneManager.getOceanWritable();

			/*
				A       = 파도 에너지 (amplitude scale)		// 잔잔한 바다: A = 0.0002, 일반 해양: A = 0.0005, 폭풍: A = 0.001
				L       = 바람 영향 길이 스케일				// V² / g == (length(windDir))^2 / g
				N       = FFT 해상도						// 256 표준, 512 고품질
				Length  = 타일 물리 크기 (meters)			// 256 or 512, cascade시엔 64 / 256 / 512로
				WindDir = 바람 방향 + 세기
			*/
			// N과 L의 관계 : grid spacing = Length / N, 위 예제는 1m당 fft grid tile이 하나라는 건가? (AAA에서는 0.5m~2m를 유지)
			static float2 windDir = float2(20, 20);	//약한 바람	5–10, 일반 바다 10–20, 거친 바다 20–30, 폭풍 30–50
			static float waveEnergy = 0.0005f;
			static float g = 9.81f;
			const float windLength = windDir.length();
			const float time = _sceneConstantBufferData._time;
			const uint32 length = SceneManager::Ocean::OCEAN_LENGTH;
			const float A = waveEnergy;
			const float L = windLength * windLength / g;
			const uint32 N = SceneManager::Ocean::OCEAN_N;

			ITextureRef sourceTexture = ocean._ht[ocean._currentReadTextureIndex * kFrameCount];

			SceneManager::Ocean::OceanParams params(
				time, g, windDir, gHeightScale, length, A, L, N,
				ocean._h0[0]->getSRV(), ocean._h0[0]->getUAV(),
				sourceTexture->getUAV(),
				ocean._height[ocean._currentReadTextureIndex]->getSRV(), ocean._height[ocean._currentReadTextureIndex]->getUAV(),
				ocean._normal[ocean._currentReadTextureIndex]->getSRV(), ocean._normal[ocean._currentReadTextureIndex]->getUAV()
			);
			ocean._initialSpectrumConstantBuffer->upload(&params);
		}

		// Upload Terrain CBuffer
		if (_renderLocalScene)
		{
			SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
			SceneManager::ClipMapTerrain& terrain = sceneManager.getClipMapTerrainWritable();

			const Transform& cameraMatrix = Camera::gMainCamera->get_worldTransform();
			const float2 cameraPos = float2(cameraMatrix.get_translation().x, cameraMatrix.get_translation().z);

			uint32 terrainConstantBufferTypeIndex = 0;
			constexpr const float vertexScale = 0.125f;
			constexpr const uint32 morphCells = 4;				// 현재 레벨 정점 간격을 기준으로 한 전환 영역의 폭.
			// 부모의 안쪽 경계가 부모 자신의 모핑 영역에 들어가지 않게 제한한다.
			static_assert(SceneManager::ClipMapTerrain::TILE_RESOLUTION >= 4);
			static_assert(morphCells > 0);
			static_assert(morphCells <= SceneManager::ClipMapTerrain::TILE_RESOLUTION - 2);

			const auto snapToGrid = [](const float2 position, const float spacing)
				{
					return DK::Math::floor(position / spacing) * spacing;
				};

			uint32 bufferIndex = 0;
			const auto uploadPatch = [&](const uint32 level, const uint32 type, const float2 patchOrigin, const uint32 rotation)
				{
					const float spacing = static_cast<float>(1u << level) * vertexScale;
					const float parentSpacing = spacing * 2.0f;
					const float2 parentOrigin = snapToGrid(cameraPos, parentSpacing);

					// 현재 Clipmap 배치에서 실제 외곽 경계는
					// 카메라로부터 각 축 방향으로 최소 2*T*spacing만큼 떨어져 있다.
					const float morphEnd = 2.0f * static_cast<float>(SceneManager::ClipMapTerrain::TILE_RESOLUTION) * spacing;
					const float morphStart = morphEnd - static_cast<float>(morphCells) * spacing;

					TerrainMeshConstantBuffer cb{};
					cb._baseXY_scale_rotate = float4( patchOrigin.x, patchOrigin.y, spacing, static_cast<float>(rotation));
					cb._morphCenter_start_end = float4( cameraPos.x, cameraPos.y, morphStart, morphEnd);
					cb._parentOrigin_spacing = float4( parentOrigin.x, parentOrigin.y, parentSpacing, 0.0f);
					cb._type = type;
					cb._level = level;
					cb._hasParent = (level + 1u < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS) ? 1u : 0u;
					cb._padding = 0;
					DK_ASSERT_LOG(bufferIndex < terrain._terrainConstantBuffer.size(), "Clipmap constant buffer count mismatch.");
					terrain._terrainConstantBuffer[bufferIndex++]->upload(&cb);
				};

			// Draw Cross
			uploadPatch(0u, 0u, snapToGrid(cameraPos, vertexScale), 0u);

			for (uint32 level = 0; level < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS; ++level)
			{
				const float spacing = static_cast<float>(1u << level) * vertexScale;
				const float gridScale = static_cast<float>(SceneManager::ClipMapTerrain::TILE_RESOLUTION) * spacing;
				const float2 snappedCameraPos = snapToGrid(cameraPos, spacing);
				const float2 base = snappedCameraPos - gridScale * 2.0f;

				// Tile
				for (uint32 x = 0; x < 4; ++x)
				{
					for (uint32 y = 0; y < 4; ++y)
					{
						if (level != 0 && (x == 1 || x == 2) && (y == 1 || y == 2))
							continue;

						const float2 fill(x >= 2 ? spacing : 0.0f, y >= 2 ? spacing : 0.0f);
						const float2 tileOrigin = base + float2(x, y) * gridScale + fill;
						uploadPatch(level, 1u, tileOrigin, 0u);
					}
				}

				// Filler: 현재 코드에서는 Filter라는 이름을 사용한다.
				uploadPatch(level, 2u, snappedCameraPos, 0u);

				const float2 parentOrigin = snapToGrid(cameraPos, spacing * 2.0f);

				// Seam: 부모가 있는 레벨만 생성한다.
				if (level + 1u < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS)
				{
					const float2 seamOrigin = parentOrigin - gridScale * 2.0f;
					uploadPatch(level, 3u, seamOrigin, 0u);
				}

				// Trim
				if ((level + 1) < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS)
				{
					const float2 d = cameraPos - parentOrigin;
					const uint32 rotation = (d.x < spacing ? 2u : 0u) | (d.y < spacing ? 1u : 0u);
					const float2 trimOrigin = snappedCameraPos + float2(spacing * 0.5f, spacing * 0.5f);
					uploadPatch(level, 4u, trimOrigin, rotation);
				}
			}
		}

		// Upload SceneObject
		DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getSceneObjectsWritable();
		for (DKHashMap<const uint32, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
		{
			SceneObject& sceneObject = iter->second;

			SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
			sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
			sceneObject._sceneObjectConstantBuffer->upload(&sceneObjectConstantBufferData);
		}

		// Render SkinnedMesh SceneObject
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
					IBufferRef& skeletonBuffer = skinnedMeshComponent->get_currentAnimationBufferCSWritable();
					skeletonBuffer->upload(currentCharacterSpaceBoneAnimation.data());
				}
			}
		}

#ifdef _DK_DEBUG_
		EditorDebugDrawManager& debugDrawManager = EditorDebugDrawManager::getSingleton();
		debugDrawManager.prepareShaderData();
#endif
	}

#if defined(USE_IMGUI)
	// Consistent label/value columns, including multi-component editors.
	template<typename DrawControl>
	void InspectorProperty(const char* label, DrawControl drawControl)
	{
		ImGui::PushID(label);
		if (ImGui::BeginTable("Property", 2, ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.46f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.54f);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::TextWrapped("%s", label);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			drawControl();
			ImGui::EndTable();
		}
		ImGui::PopID();
	}

	template<typename Enum>
	bool EnumCombo(const char* label, Enum& value, const char* const* names, int count)
	{
		int current = static_cast<int>(value);
		if (ImGui::Combo(label, &current, names, count))
		{
			value = static_cast<Enum>(current);
			return true;
		}

		return false;
	}
#endif
	void SceneRenderer::preRender() const noexcept
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

#if defined(_DK_DEBUG_)
		static bool gIsReload = false;
		if (gIsReload)
		{
			renderModule.reloadShader();
			gIsReload = false;
		}
#endif
#if defined(USE_IMGUI)
		ImGui_ImplWin32_NewFrame();
		ImGui_ImplDX12_NewFrame();
		ImGui::NewFrame();

		static bool showUI = true;
		static bool showSettings = false;
		static bool resetSettingsLayout = false;
		if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
			showUI = !showUI;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		// Transparent central node keeps the rendered scene visible and interactive.
		ImGui::DockSpaceOverViewport(0, viewport, ImGuiDockNodeFlags_PassthruCentralNode
			| (showUI ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly));
		const float uiScale = ImGui::GetFontSize() / 15.0f;
		const float margin = 12.0f * uiScale;
		float toolbarHeight = ImGui::GetFrameHeight() + 16.0f;
		if (showUI)
		{
			ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + margin, viewport->WorkPos.y + margin));
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::SetNextWindowBgAlpha(0.90f);
			const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking;
			if (ImGui::Begin("Renderer status", nullptr, flags))
			{
				if (ImGui::Button(showSettings ? "Close inspector" : "Inspector"))
					showSettings = !showSettings;
				ImGui::SameLine();
				const float fps = ImGui::GetIO().Framerate;
				ImGui::TextDisabled("%.0f fps", fps);
				ImGui::SetItemTooltip("Frame time: %.2f ms", fps > 0.0f ? 1000.0f / fps : 0.0f);
				ImGui::SameLine();
				if (ImGui::Button("..."))
					ImGui::OpenPopup("View options");
				if (ImGui::BeginPopup("View options"))
				{
					if (ImGui::MenuItem("Bring inspector back"))
					{
						showSettings = true;
						resetSettingsLayout = true;
					}
					if (ImGui::MenuItem("Hide interface", "F1"))
						showUI = false;
					ImGui::EndPopup();
				}
				toolbarHeight = ImGui::GetWindowHeight();
			}
			ImGui::End();
		}
		if (showUI && showSettings)
		{
			// Seed a default layout once; allow moving to other OS windows and persist it.
			const float width = Math::min(400.0f * uiScale, Math::max(1.0f, viewport->WorkSize.x - margin * 2.0f));
			const float top = margin * 2.0f + toolbarHeight;
			const float height = Math::min(460.0f * uiScale, Math::max(1.0f, viewport->WorkSize.y - top - margin));
			const ImGuiCond layoutCondition = resetSettingsLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
			if (resetSettingsLayout)
			{
				ImGui::SetNextWindowDockID(0);
				ImGui::SetNextWindowViewport(viewport->ID);
			}
			ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - width - margin, viewport->WorkPos.y + top), layoutCondition);
			ImGui::SetNextWindowSize(ImVec2(width, height), layoutCondition);
			resetSettingsLayout = false;
			ImGui::SetNextWindowSizeConstraints(ImVec2(340.0f * uiScale, 260.0f * uiScale), ImVec2(FLT_MAX, FLT_MAX));
			const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
			if (ImGui::Begin("Scene inspector###SceneInspector", &showSettings, flags))
			{
				auto& atmosphere = _atmosphereConstantBufferData;
				ImGui::TextDisabled("EARTH / METERS");
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight());
				ImGui::TextDisabled("(?)");
				ImGui::SetItemTooltip("Drag the title bar out to detach this panel.\nF1 hides or restores the interface.\nUse ... > Bring inspector back to reset its position.");
				if (ImGui::BeginTabBar("Renderer tabs"))
				{
					if (ImGui::BeginTabItem("Scene"))
					{
						if (ImGui::Button("Orbit", ImVec2((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f, 0)))
							setOrbitView();
						ImGui::SameLine();
						if (ImGui::Button("Surface", ImVec2(-FLT_MIN, 0)))
							Camera::gMainCamera->setView(float3(atmosphere._planetCentre[0] + 3.0f, atmosphere._planetCentre[1] + atmosphere._planetRadius + 23.0f, atmosphere._planetCentre[2] - 50.0f), float3(0, 0, 1));
						ImGui::Text("Altitude: %.1f km", atmosphere._cameraAltitude / 1000.0f);
						if (ImGui::CollapsingHeader("Shape & scale", ImGuiTreeNodeFlags_DefaultOpen))
						{
							if (ImGui::Button("Reset Earth atmosphere"))
							{
								atmosphere._planetRadius = 6360000.0f;
								atmosphere._planetCentre[0] = atmosphere._planetCentre[2] = 0.0f;
								atmosphere._planetCentre[1] = -6360000.0f;
								atmosphere._atmosphereRadius = 6390000.0f;
								atmosphere._rayleighHeight = 7994.0f;
								atmosphere._mieHeight = 1200.0f;
								atmosphere._scatteringScale = 1.0f;
							}
							InspectorProperty("Center (XYZ)", [&] { ImGui::InputFloat3("##value", atmosphere._planetCentre, "%.3g"); });
							float thickness = atmosphere._atmosphereRadius - atmosphere._planetRadius;
							InspectorProperty("Radius", [&] { ImGui::InputFloat("##value", &atmosphere._planetRadius); });
							InspectorProperty("Air thickness", [&] { ImGui::InputFloat("##value", &thickness, 0, 0, "%.6f"); });
							atmosphere._planetRadius = Math::max(atmosphere._planetRadius, 0.01f);
							atmosphere._atmosphereRadius = atmosphere._planetRadius + Math::max(thickness, atmosphere._planetRadius * 0.000001f);
						}
						if (ImGui::CollapsingHeader("Ocean"))
							InspectorProperty("Height scale", [&] { ImGui::InputFloat("##value", &gHeightScale); });
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Light"))
					{
						if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
						{
							InspectorProperty("Sun angle (deg)", [&] { ImGui::DragFloat("##value", &_atmosphereConstantBufferData._sunDegree, 0.1f, 0.0f, 0.0f, "%.3f"); });
							InspectorProperty("Irradiance", [&] { ImGui::DragFloat("##value", &atmosphere._sunIntensity, 0.1f, 0.0f, 100000.0f, "%.2f"); });
							InspectorProperty("Disk radius (deg)", [&] { ImGui::DragFloat("##value", &atmosphere._sunAngularRadius, 0.01f, 0.01f, 5.0f, "%.3f"); });
							InspectorProperty("Disk brightness", [&] { ImGui::DragFloat("##value", &atmosphere._sunRadianceScale, 0.01f, 0.0f, 100.0f, "%.2f"); });
						}
						if (ImGui::CollapsingHeader("Scattering"))
						{
							InspectorProperty("Rayleigh height", [&] { ImGui::InputFloat("##value", &atmosphere._rayleighHeight, 0, 0, "%.6f"); });
							InspectorProperty("Mie height", [&] { ImGui::InputFloat("##value", &atmosphere._mieHeight, 0, 0, "%.6f"); });
							InspectorProperty("Density multiplier", [&] { ImGui::DragFloat("##value", &atmosphere._scatteringScale, 0.01f, 0.0f, 0.0f, "%.2f"); });
						}
						if (ImGui::CollapsingHeader("Sampling quality"))
						{
							InspectorProperty("Scattering samples", [&] { ImGui::DragInt("##value", reinterpret_cast<int*>(&_atmosphereConstantBufferData._numInScatteringPoints), 1.0f, 1, 128); });
							InspectorProperty("Optical depth samples", [&] { ImGui::DragInt("##value", reinterpret_cast<int*>(&_atmosphereConstantBufferData._opticalDepthPointCount), 1.0f, 1, 64); });
						}
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Image"))
					{
						ImGui::SeparatorText("Tone mapping");
						InspectorProperty("Exposure (EV)", [&] { ImGui::DragFloat("##value", &atmosphere._exposureEV, 0.1f, -16.0f, 16.0f, "%.1f"); });
						ImGui::SeparatorText("Bloom");
						InspectorProperty("Intensity", [&] { ImGui::SliderFloat("##value", &atmosphere._bloomStrength, 0.0f, 1.0f, "%.2f"); });
						InspectorProperty("Radius (px)", [&] { ImGui::DragFloat("##value", &atmosphere._bloomRadius, 1.0f, 1.0f, 32.0f, "%.0f"); });
						InspectorProperty("Threshold", [&] { ImGui::DragFloat("##value", &atmosphere._bloomThreshold, 0.05f, 0.0f, 0.0f, "%.2f"); });
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Debug"))
					{
#if defined(_DK_DEBUG_)
						if (ImGui::Button("Reload shaders"))
							gIsReload = true;
#endif
						const float3& position = Camera::gMainCamera->get_worldTransform().get_translation();
						float3 rotation;
						Camera::gMainCamera->get_worldTransform().get_rotation().toEuler(rotation);
						ImGui::TextWrapped("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
						ImGui::TextWrapped("Rotation (rad): %.2f, %.2f, %.2f", rotation.x, rotation.y, rotation.z);
						ImGui::Spacing();
						if (ImGui::CollapsingHeader("Experimental ray tracing"))
						{
							RaytracingRenderer& rtRenderer = DuckingEngine::getInstance().GetRaytracingRendererWritable();
							ImGui::Text("Accumulated samples: %u", rtRenderer._sampleCount);
							const char* kSamplingModeStr[] =
							{
								"Uniform",
								"Cosine",
							};
							const char* kRaytracingMode[] =
							{
								"Brute force",
								"Next event estimation",
							};
							if (EnumCombo("Sampling", rtRenderer._samplingMode, kSamplingModeStr, static_cast<uint32>(RaytracingRenderer::SamplingMode::COUNT)))
							{
								rtRenderer.resetAccumulation();
							}
							if (EnumCombo("Integrator", rtRenderer._mode, kRaytracingMode, static_cast<uint32>(RaytracingRenderer::Mode::COUNT)))
							{
								rtRenderer.resetAccumulation();
							}
						}
						ImGui::TextDisabled("F1: show / hide all UI");
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				atmosphere._rayleighHeight = Math::max(atmosphere._rayleighHeight, 0.000001f);
				atmosphere._mieHeight = Math::max(atmosphere._mieHeight, 0.000001f);
				atmosphere._scatteringScale = Math::max(atmosphere._scatteringScale, 0.0f);
				atmosphere._sunIntensity = Math::clamp(atmosphere._sunIntensity, 0.0f, 100000.0f);
				atmosphere._sunAngularRadius = Math::clamp(atmosphere._sunAngularRadius, 0.01f, 5.0f);
				atmosphere._sunRadianceScale = Math::clamp(atmosphere._sunRadianceScale, 0.0f, 100.0f);
				atmosphere._exposureEV = Math::clamp(atmosphere._exposureEV, -16.0f, 16.0f);
				atmosphere._bloomStrength = Math::clamp(atmosphere._bloomStrength, 0.0f, 1.0f);
				atmosphere._bloomRadius = Math::clamp(atmosphere._bloomRadius, 1.0f, 32.0f);
				atmosphere._bloomThreshold = Math::max(atmosphere._bloomThreshold, 0.0f);
			}
			ImGui::End();
		}

		ImGui::Render();
#endif
	}
	void SceneRenderer::updateRender() noexcept
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		// Finish uploads after UI changes, before preRender closes the upload phase.
		renderModule.preRender();

        //RaytracingRenderer& raytracingRenderer = DuckingEngine::getInstance().GetRaytracingRendererWritable();
        //const bool raytraced = raytracingRenderer.updateRaytracingRenderer(renderModule);
        //if (raytraced)
        //    raytracingRenderer.dispatchRay(renderModule, _sceneConstantBuffer);
        //if (!raytraced)
        {
#if 1
		if (_renderLocalScene)
		{
		startRenderPass(renderModule, "OceanRenderPass", 0);
		{
			SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
			SceneManager::Ocean& ocean = sceneManager.getOceanWritable();

			ITextureRef sourceTexture = ocean._ht[ocean._currentReadTextureIndex * kFrameCount];
			ITextureRef targetTexture = ocean._ht[ocean._currentReadTextureIndex * kFrameCount + 1];

			static bool initial = false;
			if (initial == false)
			{
				startPipeline("InitialSpectrum");
				{
					renderModule.resourceBarrierTransition(ocean._h0[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					setConstantBuffer("OceanParams", ocean._initialSpectrumConstantBuffer);
					renderModule.dispatch(ocean.OCEAN_N, ocean.OCEAN_N, 1);
				}
				endPipeline();

				renderModule.resourceBarrierTransition(ocean._h0[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				initial = true;
			}

			startPipeline("UpdateSpectrum");
			{
				setConstantBuffer("OceanParams", ocean._initialSpectrumConstantBuffer);
				renderModule.resourceBarrierTransition(sourceTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				renderModule.resourceBarrierTransition(targetTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				renderModule.dispatch(ocean.OCEAN_N, ocean.OCEAN_N, 1);
			}
			endPipeline();

			renderModule.resourceBarrierTransition(sourceTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			const uint32 stages = static_cast<uint32>(log2(static_cast<float>(ocean.OCEAN_N)));

			startPipeline("FFTButterflyHorizontal");
			{
				setConstantBuffer("OceanParams", ocean._initialSpectrumConstantBuffer);
				for (UINT stage = 0; stage < stages; ++stage)
				{
					setRootConstantParameter("_stage", stage);
					setRootConstantParameter("_sourceSRV", sourceTexture->getSRV());
					setRootConstantParameter("_targetUAV", targetTexture->getUAV());

					renderModule.dispatch(ocean.OCEAN_N, ocean.OCEAN_N, 1);
					renderModule.resourceBarrierTransition(sourceTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					renderModule.resourceBarrierTransition(targetTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

					ITextureRef tempTexture = sourceTexture;
					sourceTexture = targetTexture;
					targetTexture = tempTexture;
				}
			}
			endPipeline();
			startPipeline("FFTButterflyVertical");
			{
				setConstantBuffer("OceanParams", ocean._initialSpectrumConstantBuffer);
				for (UINT stage = 0; stage < stages; ++stage)
				{
					setRootConstantParameter("_stage", stage);
					setRootConstantParameter("_sourceSRV", sourceTexture->getSRV());
					setRootConstantParameter("_targetUAV", targetTexture->getUAV());

					renderModule.dispatch(ocean.OCEAN_N, ocean.OCEAN_N, 1);
					renderModule.resourceBarrierTransition(sourceTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					renderModule.resourceBarrierTransition(targetTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

					ITextureRef tempTexture = sourceTexture;
					sourceTexture = targetTexture;
					targetTexture = tempTexture;
				}
			}
			endPipeline();

			renderModule.resourceBarrierTransition(targetTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			renderModule.resourceBarrierTransition(ocean._height[ocean._currentReadTextureIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			startPipeline("Finalize");
			{
				setRootConstantParameter("_sourceSRV", sourceTexture->getSRV());
				setConstantBuffer("OceanParams", ocean._initialSpectrumConstantBuffer);
				renderModule.dispatch(ocean.OCEAN_N, ocean.OCEAN_N, 1);
			}
			endPipeline();

			renderModule.resourceBarrierTransition(ocean._normal[ocean._currentReadTextureIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			startPipeline("ComputeOceanNormal");
			{
				setRootConstantParameter("_sourceSRV", sourceTexture->getSRV());
				setConstantBuffer("OceanParams", ocean._initialSpectrumConstantBuffer);
				renderModule.dispatch(ocean.OCEAN_N, ocean.OCEAN_N, 1);
			}
			endPipeline();

			renderModule.resourceBarrierTransition(ocean._normal[ocean._currentReadTextureIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			renderModule.resourceBarrierTransition(ocean._height[ocean._currentReadTextureIndex], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			startPipeline("RenderOcean");
			{
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
				setConstantBuffer("OceanParams", ocean._initialSpectrumConstantBuffer);

				renderModule.setVertexBuffers(0, 1, ocean._mesh._vertexBufferView.get());
				renderModule.setIndexBuffer(ocean._mesh._indexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(ocean._mesh._indexCount), 1, 0, 0, 0);
			}
			endPipeline();

			ocean._currentReadTextureIndex = (ocean._currentReadTextureIndex + 1) % DK_COUNT_OF(ocean._h0);
		}
		endRenderPass();
		}
#endif

		// MainRender
		startRenderPass(renderModule, "MainRenderPass", 0);
		if (_renderLocalScene)
		{
#if 0
			startPipeline("SkyDomePipeline");
			{
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer->GetGPUVirtualAddress());

				SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
				SceneManager::SkyDome& skyDome = sceneManager.getSkyDomeWritable();

				renderModule.setVertexBuffers(0, 1, skyDome._mesh._vertexBufferView.get());
				renderModule.setIndexBuffer(skyDome._mesh._indexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(skyDome._mesh._indexCount), 1, 0, 0, 0);
			}
			endPipeline();
#endif

#if 1
			startPipeline("TerrainClipmapPipeline");
			{
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);

				SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
				SceneManager::ClipMapTerrain& terrain = sceneManager.getClipMapTerrainWritable();
				Material* material = terrain._material.get();
				setConstantBuffer(material->get_materialName().c_str(), material->get_parameterBufferForGPUWritable());

				uint32 terrainConstantBufferTypeIndex = 0;
				// Draw Cross
				{
					IBufferRef& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
					setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer);

					renderModule.setVertexBuffers(0, 1, terrain._cross._vertexBufferView.get());
					renderModule.setIndexBuffer(terrain._cross._indexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._cross._indexCount), 1, 0, 0, 0);
				}

				for (uint32 i = 0; i < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS; ++i)
				{
					// Draw Tile
					for (uint32 x = 0; x < 4; ++x)
					{
						for (uint32 y = 0; y < 4; ++y)
						{
							if (i != 0 && (x == 1 || x == 2) && (y == 1 || y == 2))
								continue;

							IBufferRef& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
							setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer);

							renderModule.setVertexBuffers(0, 1, terrain._tile._vertexBufferView.get());
							renderModule.setIndexBuffer(terrain._tile._indexBufferView.get());
							renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._tile._indexCount), 1, 0, 0, 0);
						}
					}

					// Draw Filter
					{
						IBufferRef& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
						setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer);

						renderModule.setVertexBuffers(0, 1, terrain._filter._vertexBufferView.get());
						renderModule.setIndexBuffer(terrain._filter._indexBufferView.get());
						renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._filter._indexCount), 1, 0, 0, 0);
					}

					// Draw Seam
					if ((i + 1) < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS)
					{
						IBufferRef& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
						setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer);

						renderModule.setVertexBuffers(0, 1, terrain._seam._vertexBufferView.get());
						renderModule.setIndexBuffer(terrain._seam._indexBufferView.get());
						renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._seam._indexCount), 1, 0, 0, 0);
					}

					// Draw Trim
					if ((i + 1) < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS)
					{
						IBufferRef& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
						setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer);

						renderModule.setVertexBuffers(0, 1, terrain._trim._vertexBufferView.get());
						renderModule.setIndexBuffer(terrain._trim._indexBufferView.get());
						renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._trim._indexCount), 1, 0, 0, 0);
					}
				}
			}
			endPipeline();
#endif

			startPipeline("StaticMeshStandardPipeline");
			{
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);

				DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getSceneObjectsWritable();
				for (DKHashMap<const uint32, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
				{
					SceneObject& sceneObject = iter->second;
					setConstantBuffer("SceneObjectConstantBuffer", sceneObject._sceneObjectConstantBuffer);

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
							setConstantBuffer(material->get_materialName().c_str(), material->get_parameterBufferForGPU());

							renderModule.setVertexBuffers(0, 1, subMesh._vertexBufferView.get());
							renderModule.setIndexBuffer(subMesh._indexBufferView.get());
							renderModule.drawIndexedInstanced(static_cast<UINT>(subMesh._indices.size()), 1, 0, 0, 0);
						}
					}
				}
			}
			endPipeline();

			startPipeline("SkinnedMeshStandardPipeline");
			{
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);

				DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManagerWritable().getCharacterSceneObjectsWritable();
				for (DKHashMap<const uint32, SceneObject>::iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
				{
					SceneObject& sceneObject = iter->second;
					setConstantBuffer("SceneObjectConstantBuffer", sceneObject._sceneObjectConstantBuffer);

					uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
					for (uint32 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
					{
						// #todo- component 완전 개편 필요해보임.
						// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
						// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
						SkinnedMeshComponent* skinnedMeshComponent = static_cast<SkinnedMeshComponent*>(sceneObject._components[componentIndex].get());

						setConstantBuffer("SkeletonConstantBuffer", skinnedMeshComponent->get_currentAnimationBufferCS());

						DKVector<SkinnedMeshModel::SubMeshType>& subMeshes = skinnedMeshComponent->get_modelWritable()->get_subMeshArrWritable();
						for (uint32 subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
						{
							SkinnedMeshModel::SubMeshType& subMesh = subMeshes[subMeshIndex];

							Material* material = subMesh._material.get();
							setConstantBuffer(material->get_materialName().c_str(), material->get_parameterBufferForGPU());

							renderModule.setVertexBuffers(0, 1, subMesh._vertexBufferView.get());
							renderModule.setIndexBuffer(subMesh._indexBufferView.get());
							renderModule.drawIndexedInstanced(static_cast<UINT>(subMesh._indices.size()), 1, 0, 0, 0);
						}
					}
				}
			}
			endPipeline();

#ifdef _DK_DEBUG_
			EditorDebugDrawManager& debugDrawManager = EditorDebugDrawManager::getSingleton();
			const DKVector<EditorDebugDrawManager::SpherePrimitiveInfo>& spherePrimitiveInfo = debugDrawManager.get_primitiveInfoSphereArr();
			const uint32 sphereInstanceCount = static_cast<uint32>(spherePrimitiveInfo.size());
			if (sphereInstanceCount)
			{
				startPipeline("SpherePipeline");
				{
					setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
					setShaderResourceView("SpherePrimitiveInfoBuffer", debugDrawManager.get_primitiveInfoSphereBuffer());
					renderModule.setVertexBuffers(0, 1, EditorDebugDrawManager::SpherePrimitiveInfo::kVertexBufferView.get());
					renderModule.setIndexBuffer(EditorDebugDrawManager::SpherePrimitiveInfo::kIndexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(EditorDebugDrawManager::SpherePrimitiveInfo::indexCount), sphereInstanceCount, 0, 0, 0);
				}
				endPipeline();
			}
			const DKVector<EditorDebugDrawManager::LinePrimitiveInfo>& linePrimitiveInfo = debugDrawManager.get_primitiveInfoLineArr();
			const uint32 lineInstanceCount = static_cast<uint32>(linePrimitiveInfo.size());
			if (lineInstanceCount)
			{
				startPipeline("LinePipeline");
				{
					setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
					setShaderResourceView("LinePrimitiveInfoBuffer", debugDrawManager.get_primitiveInfoLineBuffer());
					renderModule.setVertexBuffers(0, 1, EditorDebugDrawManager::LinePrimitiveInfo::kVertexBufferView.get());
					renderModule.setIndexBuffer(EditorDebugDrawManager::LinePrimitiveInfo::kIndexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(EditorDebugDrawManager::LinePrimitiveInfo::indexCount), lineInstanceCount, 0, 0, 0);
				}
				endPipeline();
			}
			debugDrawManager.endUpdateRender();
#endif
		}
		endRenderPass();

		startRenderPass(renderModule, "AtmosphereRenderPass", 1);
		{
			startPipeline("AtmospherePipeline");
			{
				RenderTarget& rt = renderModule.getRenderTarget(0);
				renderModule.resourceBarrierTransition(rt._renderTarget.getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				renderModule.resourceBarrierTransition(rt._depthStencil.getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);

				SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
				SceneManager::PostProcess& postProcess = sceneManager.getPostProcessWritable();

				renderModule.setVertexBuffers(0, 1, postProcess._mesh._vertexBufferView.get());
				renderModule.setIndexBuffer(postProcess._mesh._indexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(postProcess._mesh._indexCount), 1, 0, 0, 0);
			}
			endPipeline();
		}
		endRenderPass();
        }

		// RT0 is no longer needed by atmosphere; reuse it for horizontal HDR bloom.
		if (_atmosphereConstantBufferData._bloomStrength > 0.0f)
		{
			SceneManager::PostProcess& postProcess = DuckingEngine::getInstance().getSceneManagerWritable().getPostProcessWritable();
			startRenderPass(renderModule, "BloomHorizontalRenderPass", 0);
			startPipeline("BloomHorizontalPipeline");
			RenderTarget& hdr = renderModule.getRenderTarget(1);
			renderModule.resourceBarrierTransition(hdr._renderTarget.getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
			setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
			renderModule.setVertexBuffers(0, 1, postProcess._mesh._vertexBufferView.get());
			renderModule.setIndexBuffer(postProcess._mesh._indexBufferView.get());
			renderModule.drawIndexedInstanced(static_cast<UINT>(postProcess._mesh._indexCount), 1, 0, 0, 0);
			endPipeline();
			endRenderPass();

			// The second bloom pass blurs vertically and composes with original HDR into RT2.
			startRenderPass(renderModule, "BloomVerticalRenderPass", 2);
			startPipeline("BloomVerticalPipeline");
			renderModule.resourceBarrierTransition(renderModule.getRenderTarget(0)._renderTarget.getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
			setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
			renderModule.setVertexBuffers(0, 1, postProcess._mesh._vertexBufferView.get());
			renderModule.setIndexBuffer(postProcess._mesh._indexBufferView.get());
			renderModule.drawIndexedInstanced(static_cast<UINT>(postProcess._mesh._indexCount), 1, 0, 0, 0);
			endPipeline();
			endRenderPass();
		}

		// Final output only applies exposure and tone mapping to the selected HDR source.
		startRenderPass(renderModule, "GBufferRenderPass", RenderModule::kRenderTargetTextureCount + RenderModule::kCurrentBackBufferIndex);
		{
			startPipeline("GBufferPipeline");
			{
				const uint32 outputSourceSlot = _atmosphereConstantBufferData._bloomStrength > 0.0f ? 2u : 1u;
				RenderTarget& rt = renderModule.getRenderTarget(outputSourceSlot);
				renderModule.resourceBarrierTransition(rt._renderTarget.getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				const uint32 outputSourceSRV = rt._renderTarget.getSRV();
				setRootConstantParameter("_sourceTextureIndex", outputSourceSRV);
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);

				SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
				SceneManager::GBuffer& gBuffer = sceneManager.getGBufferWritable();

				renderModule.setVertexBuffers(0, 1, gBuffer._mesh._vertexBufferView.get());
				renderModule.setIndexBuffer(gBuffer._mesh._indexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(gBuffer._mesh._indexCount), 1, 0, 0, 0);
			}
			endPipeline();
		}
		endRenderPass();
	}

	void SceneRenderer::endRender() const noexcept
	{
		DuckingEngine::getInstance().GetRenderModuleWritable().endRender();
	}
}
