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
#include "Resource/Material/GrassTiles.h"

#include <DirectXCollision.h>
#include <random>

namespace DK
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	//static std::normal_distribution<float> dist(0, 1);

	bool SceneRenderer::initialize()
	{
		if (initialize_createRenderPass() == false) 
			return false;
		if (initialize_renderPassHandles() == false || initialize_createMaterialDefinition() == false)
			return false;
		return initialize_createSceneConstantBuffer();
	}

	bool SceneRenderer::initialize_renderPassHandles()
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

		_transmittancePass = renderModule.findRenderPass("AtmosphereTransmittancePass");
		_multipleScatteringPass = renderModule.findRenderPass("AtmosphereMultipleScatteringPass");
		_surfaceBouncePass = renderModule.findRenderPass("AtmosphereSurfaceBouncePass");
		_skyLightingPass = renderModule.findRenderPass("AtmosphereSkyLightingPass");
		_oceanPass = renderModule.findRenderPass("OceanRenderPass");
		_gbufferResolvePass = renderModule.findRenderPass("GBufferRenderPass");
		_atmospherePass = renderModule.findRenderPass("AtmosphereRenderPass");
		_bloomHorizontalPass = renderModule.findRenderPass("BloomHorizontalRenderPass");
		_bloomVerticalPass = renderModule.findRenderPass("BloomVerticalRenderPass");
		_finalOutputPass = renderModule.findRenderPass("FinalOutputRenderPass");

		_mainGeometryPass = renderModule.findRenderPass("MainRenderPass");
		_mainGeometryClearPass = renderModule.findRenderPassInstance("MainGeometryClearPass");
		_surfaceCapturePass = renderModule.findRenderPassInstance("SurfaceCapturePass");

		const char* nearShadowPassNames[] = { "NearShadowPass0", "NearShadowPass1", "NearShadowPass2" };
		for (uint32 i = 0; i < DK_COUNT_OF(nearShadowPassNames); ++i)
		{
			_nearShadowPasses[i] = renderModule.findRenderPassInstance(nearShadowPassNames[i]);
			if (_nearShadowPasses[i] == nullptr)
				return false;
		}

		if (_transmittancePass == nullptr || _multipleScatteringPass == nullptr ||
			_surfaceBouncePass == nullptr || _skyLightingPass == nullptr || _oceanPass == nullptr ||
			_gbufferResolvePass == nullptr || _atmospherePass == nullptr || _bloomHorizontalPass == nullptr ||
			_bloomVerticalPass == nullptr || _finalOutputPass == nullptr || _mainGeometryPass == nullptr ||
			_mainGeometryClearPass == nullptr || _surfaceCapturePass == nullptr)
			return false;

		using Size = RenderTargetDefinition::Size;
		const auto hasGeometryOutputs = [](const RenderTarget* target)
		{
			if (target == nullptr)
				return false;

			const RenderTargetDefinition& definition = target->_definition;
			return definition._colorCount == 3 && definition._depth._srv &&
				definition._colors[0]._name == "Albedo" && definition._colors[0]._srv &&
				definition._colors[1]._name == "Normal" && definition._colors[1]._srv &&
				definition._colors[2]._name == "Material" && definition._colors[2]._srv;
		};
		const auto isCapture = [&](const RenderPassInstance& pass)
		{
			const RenderTarget* target = pass._binding._target;
			return hasGeometryOutputs(target) && target->_definition._size == Size::Fixed
				&& target->_definition._depth._format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
				&& target->_targets._width == target->_targets._height && pass._binding._store == RenderPassStoreAction::ShaderRead;
		};
		const RenderTarget* gbuffer = _mainGeometryPass->_binding._target;
		if (hasGeometryOutputs(gbuffer) == false || gbuffer->_definition._size != Size::Window || _oceanPass->_binding._target != gbuffer
			|| gbuffer->_definition._depth._format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT
			|| _mainGeometryClearPass->_binding._target != gbuffer || isCapture(*_surfaceCapturePass) == false || _surfaceCapturePass->_binding._target->_targets._width % 32u != 0)
			return false;

		for (const RenderPassInstance* pass : _nearShadowPasses)
		{
			if (isCapture(*pass) == false)
				return false;
		}

		const RenderPass* postProcess[] = { _gbufferResolvePass, _atmospherePass, _bloomHorizontalPass, _bloomVerticalPass };
		for (const RenderPass* pass : postProcess)
		{
			const RenderTarget* target = pass->_binding._target;
			if (target == nullptr || target->_definition._size != Size::Window || target->_definition._colorCount != 1 || target->_definition._colors[0]._srv == false || target == gbuffer)
				return false;
		}
		const RenderTarget* lighting = _gbufferResolvePass->_binding._target;
		const RenderTarget* atmosphere = _atmospherePass->_binding._target;
		const RenderTarget* horizontal = _bloomHorizontalPass->_binding._target;
		const RenderTarget* vertical = _bloomVerticalPass->_binding._target;
		if (lighting == atmosphere || horizontal == atmosphere || vertical == atmosphere || vertical == horizontal)
			return false;

		const RenderTarget* finalTarget = _finalOutputPass->_binding._target;
		if (finalTarget == nullptr || finalTarget->_definition._size != Size::SwapChain)
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
		if (parsePipelinePermutations(node, combinations) == false)
			return false;

		for (uint32 index = 0; index < combinations.size(); ++index)
		{
			Pipeline::CreateInfo variant = base;
			variant._defines = DK::move(combinations[index]);
			const DKString variantName = pipelinePermutationName(name, index);
#if defined(_DK_DEBUG_)
			variant._pipelineName = variantName;
#endif
			renderPass._pipelineArr.push_back(DKPair<DKString, Pipeline::CreateInfo>(variantName, DK::move(variant)));
		}
		return true;
	}

	static bool parseTargetExtent(const char* text, uint32& extent)
	{
		if (text == nullptr || *text == '\0')
			return false;

		uint32 value = 0;
		for (; *text != '\0'; ++text)
		{
			if (*text < '0' || *text > '9')
				return false;
			value = value * 10 + (*text - '0');
			if (value > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
				return false;
		}
		extent = value;
		return extent != 0;
	}

	static bool parseTargetAttachment(const TiXmlElement& node, bool depth, RenderTargetDefinition::Attachment& attachment)
	{
		const char* format = node.Attribute("Format");
		if (format == nullptr)
			return false;

		struct FormatEntry { const char* name; DXGI_FORMAT format; bool depth; };
		const FormatEntry formats[] = {
			{ "R8G8B8A8_UNORM", DXGI_FORMAT_R8G8B8A8_UNORM, false },
			{ "R16G16B16A16_FLOAT", DXGI_FORMAT_R16G16B16A16_FLOAT, false },
			{ "R32G32B32A32_FLOAT", DXGI_FORMAT_R32G32B32A32_FLOAT, false },
			{ "R11G11B10_FLOAT", DXGI_FORMAT_R11G11B10_FLOAT, false },
			{ "D16_UNORM", DXGI_FORMAT_D16_UNORM, true },
			{ "D24_UNORM_S8_UINT", DXGI_FORMAT_D24_UNORM_S8_UINT, true },
			{ "D32_FLOAT", DXGI_FORMAT_D32_FLOAT, true },
			{ "D32_FLOAT_S8X24_UINT", DXGI_FORMAT_D32_FLOAT_S8X24_UINT, true },
		};
		for (const FormatEntry& entry : formats)
		{
			if (entry.depth == depth && StringUtil::strcmp(entry.name, format) == 0)
			{
				attachment._format = entry.format;
				break;
			}
		}

		if (attachment._format == DXGI_FORMAT_UNKNOWN)
			return false;

		const char* srv = node.Attribute("SRV");
		if (srv != nullptr)
		{
			if (StringUtil::strcmp(srv, "True") == 0)
				attachment._srv = true;
			else if (StringUtil::strcmp(srv, "False") != 0)
				return false;
		}

		return true;
	}

	static bool parseRenderTarget(const TiXmlElement& node, RenderTargetDefinition& definition)
	{
		using Size = RenderTargetDefinition::Size;
		const char* source = node.Attribute("Source");
		const char* size = node.Attribute("Size");
		const char* width = node.Attribute("Width");
		const char* height = node.Attribute("Height");
		if (source != nullptr)
		{
			definition._size = Size::SwapChain;
			return StringUtil::strcmp(source, "SwapChain") == 0 && size == nullptr &&
				width == nullptr && height == nullptr && node.FirstChildElement() == nullptr;
		}
		if (size != nullptr)
		{
			if (StringUtil::strcmp(size, "Window") != 0 || width != nullptr || height != nullptr)
				return false;
		}
		else
		{
			definition._size = Size::Fixed;
			if (parseTargetExtent(width, definition._width) == false || parseTargetExtent(height, definition._height) == false)
				return false;
		}

		for (const TiXmlElement* child = node.FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
		{
			if (StringUtil::strcmp(child->Value(), "Color") == 0)
			{
				const char* name = child->Attribute("Name");
				if (name == nullptr || *name == '\0' || definition._colorCount >= DK_COUNT_OF(definition._colors))
					return false;

				for (uint32 i = 0; i < definition._colorCount; ++i)
				{
					if (definition._colors[i]._name == name)
						return false;
				}

				RenderTargetDefinition::Attachment& color = definition._colors[definition._colorCount++];
				color._name = name;
				if (parseTargetAttachment(*child, false, color) == false)
					return false;
			}
			else if (StringUtil::strcmp(child->Value(), "Depth") == 0)
			{
				if (definition._depth._format != DXGI_FORMAT_UNKNOWN || parseTargetAttachment(*child, true, definition._depth) == false)
					return false;
			}
			else
				return false;
		}

		return definition._colorCount != 0 || definition._depth._format != DXGI_FORMAT_UNKNOWN;
	}

	static bool parseRenderPassBinding(const TiXmlElement& node, const RenderModule& renderModule, RenderPassBinding& binding)
	{
		const char* target = node.Attribute("Target");
		const char* load = node.Attribute("Load");
		const char* store = node.Attribute("Store");
		if (target == nullptr)
			return load == nullptr && store == nullptr;

		binding._target = renderModule.findRenderTarget(target);
		if (binding._target == nullptr)
			return false;

		if (load != nullptr)
		{
			if (StringUtil::strcmp(load, "Clear") == 0)
				binding._load = RenderPassLoadAction::Clear;
			else if (StringUtil::strcmp(load, "Load") != 0)
				return false;
		}

		if (store != nullptr && StringUtil::strcmp(store, "Keep") != 0)
		{
			if (StringUtil::strcmp(store, "ShaderRead") != 0)
				return false;

			const RenderTargetDefinition& definition = binding._target->_definition;
			for (uint32 i = 0; i < definition._colorCount; ++i)
			{
				if (definition._colors[i]._srv == false)
					return false;
			}

			if (definition._depth._format != DXGI_FORMAT_UNKNOWN && definition._depth._srv == false)
				return false;

			binding._store = RenderPassStoreAction::ShaderRead;
		}

		return true;
	}

	bool SceneRenderer::initialize_createRenderPass()
	{
		ScopeString<DK_MAX_PATH> renderPassGroupPath = GlobalPath::makeResourceFullPath("RenderPass/RenderPassGroup.xml");

		TiXmlDocument doc;
		if (doc.LoadFile(renderPassGroupPath.c_str()) == false)
			return false;

		TiXmlElement* rootNode = doc.FirstChildElement("RenderPassGroup");
		if (rootNode == nullptr)
			return false;

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		const TiXmlElement* targetGroup = rootNode->FirstChildElement("RenderTargets");
		if (targetGroup == nullptr || targetGroup->NextSiblingElement("RenderTargets") != nullptr)
			return false;

		for (const TiXmlElement* node = targetGroup->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
		{
			const char* name = node->Attribute("Name");
			RenderTargetDefinition definition;
			if (StringUtil::strcmp(node->Value(), "RenderTarget") != 0 || name == nullptr || *name == '\0' || parseRenderTarget(*node, definition) == false || renderModule.createRenderTarget(name, DK::move(definition)) == false)
			{
				DK_LOG("Invalid render target definition: %s", name != nullptr ? name : "(unnamed)");
				return false;
			}
		}

		ShaderCompiler shaderCompiler(renderModule.getShaderModel());
		for (TiXmlElement* renderPassNode = rootNode->FirstChildElement(); renderPassNode != nullptr; renderPassNode = renderPassNode->NextSiblingElement())
		{
			const char* nodeType = renderPassNode->Value();
			if (renderPassNode == targetGroup || StringUtil::strcmp(nodeType, "RenderPassInstance") == 0)
				continue;
			if (StringUtil::strcmp(nodeType, "RenderPass") != 0)
				return false;

			const char* renderPassName = renderPassNode->Attribute("Name");
			if (renderPassName == nullptr || renderPassName[0] == '\0')
				return false;

			RenderPass::CreateInfo renderPassCreateInfo;
			if (parseRenderPassBinding(*renderPassNode, renderModule, renderPassCreateInfo._binding) == false)
			{
				DK_LOG("Invalid output binding for render pass %s", renderPassName);
				return false;
			}
			for (TiXmlElement* renderPassChildNode = renderPassNode->FirstChildElement(); renderPassChildNode != nullptr; renderPassChildNode = renderPassChildNode->NextSiblingElement())
			{
				DKString renderPassChildNodeName = renderPassChildNode->Value();
				if (renderPassChildNodeName == "Pipeline")
				{
					const RenderPassBinding& binding = renderPassCreateInfo._binding;
					if (binding._target == nullptr)
						return false;
					Pipeline::CreateInfo pipelineCreateInfo;
					DKString pipelineName = renderPassChildNode->Attribute("Name");
					pipelineCreateInfo._target = binding._target;
					pipelineCreateInfo._primitiveTopologyType = renderPassChildNode->Attribute("PrimitiveTopologyType");
					const char* depthEnable = renderPassChildNode->Attribute("DepthEnable");
					pipelineCreateInfo._depthEnable = depthEnable == nullptr || StringUtil::strcmp(depthEnable, "True") != 0 ? false : true;
					if (pipelineCreateInfo._depthEnable && binding._target->_definition._depth._format == DXGI_FORMAT_UNKNOWN)
						return false;
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

					if (appendPipelinePermutations(*renderPassChildNode, pipelineName, pipelineCreateInfo, renderPassCreateInfo) == false)
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

		for (const TiXmlElement* instance = rootNode->FirstChildElement("RenderPassInstance"); instance != nullptr; instance = instance->NextSiblingElement("RenderPassInstance"))
		{
			const char* name = instance->Attribute("Name");
			const char* definitionName = instance->Attribute("Pass");
			if (name == nullptr || name[0] == '\0' || definitionName == nullptr || definitionName[0] == '\0')
				return false;

			RenderPassBinding binding;
			if (parseRenderPassBinding(*instance, renderModule, binding) == false || (binding._target == nullptr) || renderModule.createRenderPassInstance(name, definitionName, binding) == false)
			{
				DK_LOG("Invalid render pass instance %s", name);
				return false;
			}
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
			InsertResult insertResult = _materialDefinitionMap.insert(DKPair<DKString, MaterialDefinition>(materialName, DK::move(materialDefinition)));
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
		_atmosphereTransmittanceLut = DuckingEngine::getInstance().GetRenderModuleWritable().createTexture("AtmosphereTransmittanceLut", kAtmosphereLutWidth, kAtmosphereLutHeight * 2u, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, true);
		if (_atmosphereTransmittanceLut == nullptr)
			return false;

        RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		for (uint32 i = 0; i < 2u; ++i)
		{
			_multipleScatteringLut[i] = renderModule.createTexture(i == 0 ? "AtmosphereMultipleScattering0" : "AtmosphereMultipleScattering1", 32, 32, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true, true);
			if (_multipleScatteringLut[i] == nullptr)
				return false;
		}

		_skyRadiance = renderModule.createTexture("AtmosphereSkyRadiance", 64, 32, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true, true);
		if (_skyRadiance == nullptr)
			return false;
		_skyFiltered = renderModule.createTexture("AtmosphereSkyFiltered", 64, 192, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, true);
		if (_skyFiltered == nullptr)
			return false;

        _surfaceSceneConstantBuffer = renderModule.createConstantBuffer(sizeof(SceneConstantBuffer), L"SurfaceCaptureSceneConstantBuffer");
		if (_surfaceSceneConstantBuffer == nullptr)
			return false;
        _surfaceBounce = renderModule.createTexture("Atmosphere surface bounce field", 256, 16, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true, true);
        if (_surfaceBounce == nullptr)
			return false;

		for (uint32 i = 0; i < DK_COUNT_OF(_nearShadowSceneConstants); ++i)
		{
			_nearShadowSceneConstants[i] = renderModule.createConstantBuffer(sizeof(SceneConstantBuffer), L"ShadowCascadeSceneConstants");
			if (_nearShadowSceneConstants[i] == nullptr)
				return false;
		}

		AtmosphereConstantBuffer& surface = _atmosphereConstantBufferData;
		const RenderPassTargets& capture = _surfaceCapturePass->_binding._target->_targets;
		surface._surfaceMapSize = capture._width;
		surface._transmittanceLutIndex = _atmosphereTransmittanceLut->getSRV();
        surface._multipleScatteringLutIndex = _multipleScatteringLut[0]->getSRV();
        surface._skyRadianceIndex = _skyRadiance->getSRV();
        surface._skyFilteredIndex = _skyFiltered->getSRV();
        surface._surfaceAlbedoIndex = capture._colors[0]._texture->getSRV();
        surface._surfaceNormalIndex = capture._colors[1]._texture->getSRV();
        surface._surfaceMaterialIndex = capture._colors[2]._texture->getSRV();
        surface._surfaceDepthIndex = capture._depth._texture->getSRV();
        surface._surfaceBounceIndex = _surfaceBounce->getSRV();

		return true;
	}

	const MaterialDefinition* SceneRenderer::getMaterialDefinition(const DKString& materialName) const
	{
		using FindResult = DKHashMap<DKString, MaterialDefinition>::const_iterator;
		FindResult findResult = _materialDefinitionMap.find(materialName);
		if (findResult == _materialDefinitionMap.end())
			return nullptr;

		return &findResult->second;
	}

	void SceneRenderer::updateAtmosphereCamera(Camera& camera) const noexcept
	{
		const AtmosphereConstantBuffer& atmosphere = _atmosphereConstantBufferData;
		const float3 center(atmosphere._planetCentre[0], atmosphere._planetCentre[1], atmosphere._planetCentre[2]);
		const Transform& transform = camera.get_worldTransform();
		const float3 position = transform.get_translation();
		const double x = static_cast<double>(position.x) - center.x;
		const double y = static_cast<double>(position.y) - center.y;
		const double z = static_cast<double>(position.z) - center.z;
		const double distance = std::sqrt(x * x + y * y + z * z);
		// The density reference sphere is not a collision surface.
		// Keep camera controls positive even inside that radius.
		const float altitude = static_cast<float>(std::max(distance - atmosphere._densityReferenceRadius, 0.0));
		camera.setMoveSpeed(Math::clamp(altitude * 0.2f, 5.0f, 2000000.0f));
		camera.setClipPlaneDistances(Math::max(0.1f, altitude * 0.00001f));
	}
	void SceneRenderer::prepareShaderData(const float deltaTime) noexcept
	{
		const float3 cameraPosition = Camera::gMainCamera->get_worldTransform().get_translation();
		const AtmosphereConstantBuffer& atmosphere = _atmosphereConstantBufferData;
		const float3 planetCenter(atmosphere._planetCentre[0], atmosphere._planetCentre[1], atmosphere._planetCentre[2]);
		const double dx = static_cast<double>(cameraPosition.x) - planetCenter.x;
		const double dy = static_cast<double>(cameraPosition.y) - planetCenter.y;
		const double dz = static_cast<double>(cameraPosition.z) - planetCenter.z;
		_atmosphereConstantBufferData._cameraAltitude = static_cast<float>(std::sqrt(dx * dx + dy * dy + dz * dz) - atmosphere._densityReferenceRadius);

        // The demo geometry has its own world-space region, independent of atmospheric radii.
        const double gx = double(cameraPosition.x) - _localGeometryCentre.x;
        const double gy = double(cameraPosition.y) - _localGeometryCentre.y;
        const double gz = double(cameraPosition.z) - _localGeometryCentre.z;
        _renderLocalScene = _localGeometryEnabled && gx * gx + gy * gy + gz * gz < double(_localGeometryRadius) * _localGeometryRadius;

		// Upload Scene ConstantBuffer
		{
			_sceneConstantBufferData._frameIndex = RenderModule::kCurrentFrameIndex;
			_sceneConstantBufferData._resolution[0] = RenderModule::kWidth;
			_sceneConstantBufferData._resolution[1] = RenderModule::kHeight;
			_sceneConstantBufferData._time += deltaTime;
			_sceneConstantBufferData._nearDistance = Camera::gMainCamera->getNearPlaneDistance();
			_sceneConstantBufferData._rngState = static_cast<uint32>(gen());

			Camera::gMainCamera->get_worldTransform().tofloat4x4(_sceneConstantBufferData._cameraWorldMatrix);
			Camera::gMainCamera->getCameraWorldMatrix(_sceneConstantBufferData._cameraWorldMatrixInv);
			Camera::gMainCamera->getCameraProjectionMatrix(_sceneConstantBufferData._cameraProjectionMatrix);
			_sceneConstantBuffer->upload(&_sceneConstantBufferData);

			prepareSurfaceCapture(cameraPosition);
		}

		{
			// The last ping-pong output is determined by the number of scattering passes.
            const uint32 passes = atmosphere._multipleScatteringMaxOrder > 1u ? atmosphere._multipleScatteringMaxOrder - 1u : 0u;
            _atmosphereConstantBufferData._multipleScatteringLutIndex = _multipleScatteringLut[passes ? (passes - 1u) & 1u : 0u]->getSRV();
            AtmosphereConstantBuffer relativeAtmosphere = _atmosphereConstantBufferData;
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
				time, g, windDir, ocean._heightScale, length, A, L, N,
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
		for (const SceneObject& sceneObject : DuckingEngine::getInstance().GetSceneObjectManager().getSceneObjects())
		{
			if (sceneObject._sceneObjectConstantBuffer == nullptr)
				continue;

			SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
			sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
			sceneObject._sceneObjectConstantBuffer->upload(&sceneObjectConstantBufferData);
		}

		// Render SkinnedMesh SceneObject
		for (const SceneObject& sceneObject : DuckingEngine::getInstance().GetSceneObjectManager().getCharacterSceneObjects())
		{
			if (sceneObject._sceneObjectConstantBuffer == nullptr)
				continue;

			SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
			sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
			sceneObject._sceneObjectConstantBuffer->upload(&sceneObjectConstantBufferData);

			uint32 componentCount = static_cast<uint32>(sceneObject.getComponents().size());
			for (uint32 i = 0; i < componentCount; ++i)
			{
				// #todo- component 완전 개편 필요해보임.
				// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
				// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
				const SkinnedMeshComponent* skinnedMeshComponent = static_cast<const SkinnedMeshComponent*>(sceneObject.getComponents()[i].get());

				if (skinnedMeshComponent == nullptr || skinnedMeshComponent->_currentAnimationBufferCS == nullptr)
					continue;

				const DKVector<float4x4>& currentCharacterSpaceBoneAnimation = skinnedMeshComponent->_currentCharacterSpaceBoneAnimation;
				if (currentCharacterSpaceBoneAnimation.empty() == false)
					skinnedMeshComponent->_currentAnimationBufferCS->upload(currentCharacterSpaceBoneAnimation.data());
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
	void SceneRenderer::renderUI() noexcept
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
		ImGui::DockSpaceOverViewport(0, viewport, ImGuiDockNodeFlags_PassthruCentralNode | (showUI ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly));

		const float uiScale = ImGui::GetFontSize() / 15.0f;
		const float margin = 12.0f * uiScale;
		float toolbarHeight = ImGui::GetFrameHeight() + 16.0f;
		if (showUI)
		{
			ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + margin, viewport->WorkPos.y + margin));
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::SetNextWindowBgAlpha(0.90f);
			const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking;
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
					if (ImGui::MenuItem("Bring inspector back")) { showSettings = true; resetSettingsLayout = true; }
					if (ImGui::MenuItem("Hide interface", "F1")) showUI = false;
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
				AtmosphereConstantBuffer& atmosphere = _atmosphereConstantBufferData;

				ImGui::TextDisabled("EARTH / METERS");
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight());
				ImGui::TextDisabled("(?)");
				ImGui::SetItemTooltip("Drag the title bar out to detach this panel.\nF1 hides or restores the interface.\nUse ... > Bring inspector back to reset its position.");

				if (ImGui::BeginTabBar("Renderer tabs"))
				{
					if (ImGui::BeginTabItem("Scene"))
					{
						if (ImGui::Button("Orbit", ImVec2((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f, 0)))
						{
							const AtmosphereConstantBuffer& atmosphere = _atmosphereConstantBufferData;
							const float sunAngle = atmosphere._sunDegree * Math::kToRadian;
							const float earthAngle = sunAngle - 25.0f * Math::kToRadian;
							const float3 earthDirection(0, Math::sin(earthAngle), Math::cos(earthAngle));
							const float3 sunDirection(0, Math::sin(sunAngle), Math::cos(sunAngle));
							const float3 center(atmosphere._planetCentre[0], atmosphere._planetCentre[1], atmosphere._planetCentre[2]);
							Camera::gMainCamera->setView(center - earthDirection * (4.0f * atmosphere._atmosphereRadius), earthDirection + sunDirection);
						}

						ImGui::SameLine();
						if (ImGui::Button("Surface", ImVec2(-FLT_MIN, 0)))
							Camera::gMainCamera->setView(float3(atmosphere._planetCentre[0] + 3.0f, atmosphere._planetCentre[1] + atmosphere._densityReferenceRadius + 23.0f, atmosphere._planetCentre[2] - 50.0f), float3(0, 0, 1));

						ImGui::Text("Altitude: %.1f km", atmosphere._cameraAltitude / 1000.0f);

						if (ImGui::CollapsingHeader("Shape & scale", ImGuiTreeNodeFlags_DefaultOpen))
						{
							if (ImGui::Button("Reset Earth atmosphere"))
							{
                                _compareExtendedAtmosphereTop = false;
								atmosphere._densityReferenceRadius = 6360000.0f;
								atmosphere._densityTransitionRadius = 6358000.0f;
								atmosphere._planetCentre[0] = atmosphere._planetCentre[2] = 0.0f;
								atmosphere._planetCentre[1] = -6360000.0f;
								atmosphere._atmosphereRadius = 6390000.0f;
								atmosphere._rayleighHeight = 7994.0f;
								atmosphere._mieHeight = 1200.0f;
								atmosphere._scatteringScale = 1.0f;
                                atmosphere._rayleighScale = atmosphere._mieScale = 1.0f;
                                atmosphere._mieAbsorptionScale = 0.0f;
                                atmosphere._mieAnisotropy = 0.76f;
                                for (AtmosphereAbsorptionLayer& layer : atmosphere._absorptionLayers)
									layer = AtmosphereAbsorptionLayer();
							}
							ImGui::InputFloat3("Center (XYZ)", atmosphere._planetCentre, "%.3g");
							ImGui::Text("Density reference radius (m): %.1f km", atmosphere._densityReferenceRadius / 1000.0f);
							ImGui::InputFloat("Density transition radius (m)", &atmosphere._densityTransitionRadius, 100.0f, 1000.0f, "%.1f");
							ImGui::InputFloat("Atmosphere radius (m)", &atmosphere._atmosphereRadius, 100.0f, 1000.0f, "%.1f");

                            if (ImGui::Checkbox("Compare extended atmosphere top", &_compareExtendedAtmosphereTop))
                            {
                                if (_compareExtendedAtmosphereTop)
                                {
                                    float top = atmosphere._densityReferenceRadius + 12.0f * Math::max(atmosphere._rayleighHeight, atmosphere._mieHeight);
									for (const AtmosphereAbsorptionLayer& layer : atmosphere._absorptionLayers)
									{
										if (layer.strength > 0.0f)
											top = Math::max(top, layer.peakRadius + layer.upperWidth);
									}

                                    atmosphere._atmosphereRadius = Math::max(top, atmosphere._atmosphereRadius);
                                }
								else
								{
									atmosphere._atmosphereRadius = Math::max(kEarthAtmosphereRadius, atmosphere._densityReferenceRadius + 1.0f);
								}
                            }
						}

                        if (ImGui::CollapsingHeader("Local geometry region"))
                        {
                            ImGui::Checkbox("Enable demo geometry", &_localGeometryEnabled);
                            ImGui::InputFloat3("Region centre", &_localGeometryCentre.x);
                            ImGui::InputFloat("Region radius (m)", &_localGeometryRadius);
                            ImGui::TextWrapped("This controls the existing demo scene, independently of atmosphere density radii. Disable for a gas-only scene.");

                            _localGeometryRadius = Math::max(_localGeometryRadius, 1.0f);
                        }

						if (ImGui::CollapsingHeader("Ocean"))
						{
							SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
							SceneManager::Ocean& ocean = sceneManager.getOceanWritable();
							InspectorProperty("Height scale", [&] { ImGui::InputFloat("##value", &ocean._heightScale); });
						}

						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Light"))
					{
						if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
						{
							InspectorProperty("Sun angle (deg)", [&] { ImGui::DragFloat("##value", &_atmosphereConstantBufferData._sunDegree, 0.1f, 0.0f, 0.0f, "%.3f"); });
							InspectorProperty("Solar irradiance (relative)", [&] { ImGui::DragFloat("##value", &atmosphere._sunIrradiance, 0.1f, 0.0f, 100000.0f, "%.2f"); });
							InspectorProperty("Sun color (linear RGB)", [&] { ImGui::ColorEdit3("##value", atmosphere._sunColor, ImGuiColorEditFlags_Float); });
							ImGui::TextDisabled("Relative HDR units; color normalized to unit luminance.");
							InspectorProperty("Disk radius (deg)", [&] { ImGui::DragFloat("##value", &atmosphere._sunAngularRadius, 0.01f, 0.01f, 5.0f, "%.3f"); });
							InspectorProperty("Disk brightness", [&] { ImGui::DragFloat("##value", &atmosphere._sunRadianceScale, 0.01f, 0.0f, 100.0f, "%.2f"); });
						}
						if (ImGui::CollapsingHeader("Scattering"))
						{
							InspectorProperty("Rayleigh height", [&] { ImGui::InputFloat("##value", &atmosphere._rayleighHeight, 0, 0, "%.6f"); });
							InspectorProperty("Mie height", [&] { ImGui::InputFloat("##value", &atmosphere._mieHeight, 0, 0, "%.6f"); });
                            InspectorProperty("Rayleigh amount", [&] { ImGui::DragFloat("##value", &atmosphere._rayleighScale, 0.01f, 0.0f, 10.0f); });
                            InspectorProperty("Mie scattering", [&] { ImGui::DragFloat("##value", &atmosphere._mieScale, 0.01f, 0.0f, 10.0f); });
                            InspectorProperty("Mie absorption", [&] { ImGui::DragFloat("##value", &atmosphere._mieAbsorptionScale, 0.01f, 0.0f, 10.0f); });
                            InspectorProperty("Mie anisotropy", [&] { ImGui::SliderFloat("##value", &atmosphere._mieAnisotropy, -0.95f, 0.95f); });
                            InspectorProperty("Multiple scattering", [&] { ImGui::SliderFloat("##value", &atmosphere._multipleScatteringStrength, 0.0f, 1.0f); });
                            InspectorProperty("Max scattering order", [&] {
                                int order = static_cast<int>(atmosphere._multipleScatteringMaxOrder);
                                if (ImGui::SliderInt("##value", &order, 1, 8))
                                    atmosphere._multipleScatteringMaxOrder = static_cast<uint32>(order);
                            });
                            InspectorProperty("Sky lighting", [&] { ImGui::SliderFloat("##value", &atmosphere._skyLightingStrength, 0.0f, 2.0f); });
							InspectorProperty("Density multiplier", [&] { ImGui::DragFloat("##value", &atmosphere._scatteringScale, 0.01f, 0.0f, 0.0f, "%.2f"); });
						}
                        if (ImGui::CollapsingHeader("Surface to atmosphere"))
                        {
                            InspectorProperty("Sun shadow strength", [&] { ImGui::SliderFloat("##value", &atmosphere._surfaceShadowStrength, 0.0f, 1.0f); });
                            InspectorProperty("Reflected light strength", [&] { ImGui::SliderFloat("##value", &atmosphere._surfaceBounceStrength, 0.0f, 2.0f); });
                            InspectorProperty("Capture half-width (m)", [&] { ImGui::InputFloat("##value", &_surfaceCaptureExtent); });
                            InspectorProperty("Capture depth (m)", [&] { ImGui::InputFloat("##value", &_surfaceCaptureDepth); });
                            InspectorProperty("Shadow bias (m)", [&] { ImGui::InputFloat("##value", &atmosphere._surfaceBias); });
                            InspectorProperty("Bounce samples", [&] { ImGui::SliderInt("##value", reinterpret_cast<int*>(&atmosphere._surfaceBounceSamples), 8, 64); });
                            _surfaceCaptureExtent = Math::max(_surfaceCaptureExtent, 1.0f);
							_surfaceCaptureDepth = Math::max(_surfaceCaptureDepth, 1.0f);
                            atmosphere._surfaceBias = Math::max(atmosphere._surfaceBias, 0.0f);
                            ImGui::TextWrapped("Four 512x512 shadow cascades (1/8, 1/4, 1/2, full half-width), with blended boundaries. The outer capture feeds the 16x16x16 bounce field.");
                        }
                        if (ImGui::CollapsingHeader("Gas absorption layers"))
                        {
                            ImGui::TextWrapped("Independent RGB absorption shells. Radii are measured from the planet centre; zero amount disables a layer.");
                            if (ImGui::Button("Earth ozone example (layer 0)"))
                            {
                                AtmosphereAbsorptionLayer& layer = atmosphere._absorptionLayers[0];
                                layer.extinction[0] = 0.650e-6f;
								layer.extinction[1] = 1.881e-6f;
								layer.extinction[2] = 0.085e-6f;
                                layer.strength = 1.0f;
								layer.peakRadius = atmosphere._densityReferenceRadius + 25000.0f;
                                layer.lowerWidth = layer.upperWidth = 15000.0f;
                            }
                            for (uint32 i = 0; i < 2u; ++i)
                            {
                                ImGui::PushID(static_cast<int>(i));
                                ImGui::Text("Layer %u", i);
                                AtmosphereAbsorptionLayer& layer = atmosphere._absorptionLayers[i];
                                InspectorProperty("Amount", [&] { ImGui::DragFloat("##value", &layer.strength, 0.01f, 0.0f, 10.0f); });
                                float inverseKm[3] = { layer.extinction[0] * 1000, layer.extinction[1] * 1000, layer.extinction[2] * 1000 };
                                InspectorProperty("Absorption RGB (1/km)", [&] { ImGui::InputFloat3("##value", inverseKm, "%.6f"); });
                                InspectorProperty("Peak radius (m)", [&] { ImGui::InputFloat("##value", &layer.peakRadius, 100, 1000, "%.1f"); });
                                InspectorProperty("Lower width (m)", [&] { ImGui::InputFloat("##value", &layer.lowerWidth); });
                                InspectorProperty("Upper width (m)", [&] { ImGui::InputFloat("##value", &layer.upperWidth); });
								for (uint32 c = 0; c < 3u; ++c)
									layer.extinction[c] = Math::max(inverseKm[c], 0.0f) / 1000.0f;
                                layer.peakRadius = Math::max(layer.peakRadius, 0.0f);
                                layer.lowerWidth = Math::max(layer.lowerWidth, 0.001f);
								layer.upperWidth = Math::max(layer.upperWidth, 0.001f);
                                ImGui::PopID();
                            }
                        }
						if (ImGui::CollapsingHeader("Sampling quality"))
						{
							const char* samplingModes[] = { "Production", "Direct reference", "Reference limits", "Production work", "Production transmittance" };
							InspectorProperty("Atmosphere integration", [&] { ImGui::Combo("##value", reinterpret_cast<int*>(&atmosphere._atmosphereSamplingMode), samplingModes, 5); });
                            if (atmosphere._atmosphereSamplingMode == 1u || atmosphere._atmosphereSamplingMode == 2u)
                                ImGui::TextWrapped("Direct integration is limited to the centre 16x16 pixels. Surrounding pixels use Production. Move the camera to inspect a feature at the centre.");
							InspectorProperty("View work budget", [&] { ImGui::SliderInt("##value", reinterpret_cast<int*>(&atmosphere._viewNodeBudget), 128, 256); });
                            InspectorProperty("Light work budget", [&] { ImGui::SliderInt("##value", reinterpret_cast<int*>(&atmosphere._opticalNodeBudget), 64, 128); });
                            InspectorProperty("Relative error", [&] { ImGui::DragFloat("##value", &atmosphere._integrationRelativeTolerance, 0.0001f, 0.00001f, 0.05f, "%.5f"); });
							InspectorProperty("Subdivision depth", [&] { ImGui::DragInt("##value", reinterpret_cast<int*>(&atmosphere._integrationMaxDepth), 1.0f, 0, 10); });
							InspectorProperty("Initial view intervals", [&] { ImGui::DragInt("##value", reinterpret_cast<int*>(&_atmosphereConstantBufferData._numInScatteringPoints), 1.0f, 1, 128); });
							InspectorProperty("Initial light intervals", [&] { ImGui::DragInt("##value", reinterpret_cast<int*>(&_atmosphereConstantBufferData._opticalDepthPointCount), 1.0f, 1, 64); });
						}
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Image"))
					{
						ImGui::SeparatorText("Tone mapping");
						InspectorProperty("Exposure compensation (EV)", [&] { ImGui::DragFloat("##value", &atmosphere._exposureEV, 0.1f, -16.0f, 16.0f, "%.1f"); });
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
                        {
                            gIsReload = true;
                            _atmosphereLutValid = false;
                            _skyLightingValid = false;
                            _surfaceTransportValid = false;
                        }
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
						RenderModule& timing = DuckingEngine::getInstance().GetRenderModuleWritable();
                        const char* scopeNames[] = { "Transmittance LUT", "Sky lighting", "Surface lighting", "Atmosphere", "Surface capture / bounce" };
                        for (uint32 scope = 0; scope < 5; ++scope)
                            ImGui::Text("%s: %.3f ms (peak %.3f)", scopeNames[scope], timing.gpuMilliseconds(scope), timing.gpuPeakMilliseconds(scope));
                        if (ImGui::Button("Reset GPU peaks")) timing.resetGpuTimingPeaks();
                        ImGui::TextDisabled("GPU timestamps; -1 = unavailable. Cached passes approach 0 ms.");
                        ImGui::TextDisabled("F1: show / hide all UI");
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}

				atmosphere._rayleighHeight = Math::max(atmosphere._rayleighHeight, 0.000001f);
				atmosphere._mieHeight = Math::max(atmosphere._mieHeight, 0.000001f);
				atmosphere._multipleScatteringMaxOrder = Math::clamp(atmosphere._multipleScatteringMaxOrder, 1u, 8u);
				atmosphere._scatteringScale = Math::max(atmosphere._scatteringScale, 0.0f);
				atmosphere._sunIrradiance = Math::clamp(atmosphere._sunIrradiance, 0.0f, 100000.0f);
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
    void SceneRenderer::prepareSurfaceCapture(const float3& cameraPosition)
    {
		AtmosphereConstantBuffer& atmosphereData = _atmosphereConstantBufferData;

#if defined(_DK_DEBUG_)
		// HLSL의 SURFACE_BOUNCE_GRID와 반드시 동일해야 합니다.
		constexpr uint32 kSurfaceBounceGrid = 16u;
		const uint32 mapSize = atmosphereData._surfaceMapSize;
		// 격자 중심이 텍셀 경계에 위치하도록 강제합니다. 예: 512 / 16 = 32 → 짝수이므로 유효.
		const bool validGrid = mapSize > 0u && mapSize % (2u * kSurfaceBounceGrid) == 0u;
		// 셰이더가 사용하는 해상도와 실제 텍스처 해상도 일치.
		const bool validMapSize = mapSize == _surfaceCapturePass->_binding._target->_targets._width;
		const bool validCapture = std::isfinite(_surfaceCaptureExtent) && _surfaceCaptureExtent > 0.0f && std::isfinite(_surfaceCaptureDepth) && _surfaceCaptureDepth > 0.0f;
		const bool valid = validGrid && validMapSize && validCapture;
		DK_ASSERT_LOG(valid, "Invalid surface capture: map size must match the texture and be an even multiple of the bounce grid.");

		// Assert가 비활성화된 빌드에서도 잘못된 설정을 사용하지 않습니다.
		if (!valid)
		{
			atmosphereData._surfaceMapEnabled = 0u;
			_surfaceTransportValid = false;
			return;
		}
#endif

        atmosphereData._surfaceMapEnabled = _renderLocalScene && (atmosphereData._surfaceShadowStrength > 0 || atmosphereData._surfaceBounceStrength > 0) ? 1u : 0u;

        const float depth = Math::max(_surfaceCaptureDepth, 1.0f);
        const float directionalLightAngle = atmosphereData._sunDegree * Math::kToRadian;
        const float3 directionalLightDir(0, -Math::sin(directionalLightAngle), -Math::cos(directionalLightAngle));
		const float4 directionalLightDir4 = float4(directionalLightDir.x, directionalLightDir.y, directionalLightDir.z, 0);
        const float3 hint = Math::abs(directionalLightDir.y) < 0.99f ? float3::Up : float3::Right;

		float3 r3 = hint.cross(directionalLightDir);
		r3.normalize();
		float3 u3 = directionalLightDir.cross(r3);
		u3.normalize();

        const float3 right(r3.x, r3.y, r3.z), up(u3.x, u3.y, u3.z);
		const float3 basis[3] = { right, up, directionalLightDir };

        auto doublePrecisionDot = [](const float3& a, const float3& b) { return double(a.x) * b.x + double(a.y) * b.y + double(a.z) * b.z; };
        for (uint32 cascadeIndex = 0; cascadeIndex < atmosphereData._shadowCascadeCount; ++cascadeIndex)
        {
            // Nested light-space coverage also serves off-screen atmospheric samples.
            const float extent = Math::max(_surfaceCaptureExtent, 1.0f) / float(1u << (3u - cascadeIndex));
            const RenderPassInstance* pass = cascadeIndex == 3u ? _surfaceCapturePass : _nearShadowPasses[cascadeIndex];
            const RenderPassTargets& map = pass->_binding._target->_targets;
            const float cell = 2.0f * extent / map._width;
            const float snapRight = std::round(doublePrecisionDot(cameraPosition, right) / cell) * cell - doublePrecisionDot(cameraPosition, right);
            const float snapUp = std::round(doublePrecisionDot(cameraPosition, up) / cell) * cell - doublePrecisionDot(cameraPosition, up);
            const float snapForward = std::round(doublePrecisionDot(cameraPosition, directionalLightDir) / (depth / 16.0)) * (depth / 16.0) - doublePrecisionDot(cameraPosition, directionalLightDir);
            const float3 eye(float(double(cameraPosition.x) + right.x * snapRight + up.x * snapUp + directionalLightDir.x * (snapForward - depth * 0.5)),
                             float(double(cameraPosition.y) + right.y * snapRight + up.y * snapUp + directionalLightDir.y * (snapForward - depth * 0.5)),
                             float(double(cameraPosition.z) + right.z * snapRight + up.z * snapUp + directionalLightDir.z * (snapForward - depth * 0.5)));
            const float3 origin = eye - cameraPosition;

            float* target[3] = { atmosphereData._surfaceRight, atmosphereData._surfaceUp, atmosphereData._surfaceForward };
			for (uint32 i = 0; i < 3; ++i)
			{
				target[i][0] = basis[i].x;
				target[i][1] = basis[i].y;
				target[i][2] = basis[i].z;
			}

			atmosphereData._surfaceRight[3] = extent;
			atmosphereData._surfaceUp[3] = extent;
			atmosphereData._surfaceForward[3] = depth;
            atmosphereData._surfaceOrigin[0] = origin.x;
			atmosphereData._surfaceOrigin[1] = origin.y;
			atmosphereData._surfaceOrigin[2] = origin.z;

			// cascade constant buffer에 덮어 씌울 것이기 때문에 복사한다음 cascade로 upload 수행
            SceneConstantBuffer scene = _sceneConstantBufferData;
            scene._resolution[0] = map._width;
            scene._resolution[1] = map._height;
            scene._cameraWorldMatrixInv = float4x4(
				right.x, up.x, directionalLightDir.x, 0,
				right.y, up.y, directionalLightDir.y, 0,
				right.z, up.z, directionalLightDir.z, 0,
				float(-doublePrecisionDot(eye, right)), float(-doublePrecisionDot(eye, up)), float(-doublePrecisionDot(eye, directionalLightDir)), 1
			);
            scene._cameraWorldMatrix = float4x4(right.x, right.y, right.z, 0, up.x, up.y, up.z, 0, directionalLightDir.x, directionalLightDir.y, directionalLightDir.z, 0, eye.x, eye.y, eye.z, 1);
            // Orthographic reversed Z: capture near -> 1, far -> 0.
            scene._cameraProjectionMatrix = float4x4(1 / extent, 0, 0, 0, 0, 1 / extent, 0, 0, 0, 0, -1 / depth, 0, 0, 0, 1, 1);

			IBufferRef& surfaceSceneConstantBuffer = (cascadeIndex == 3u ? _surfaceSceneConstantBuffer : _nearShadowSceneConstants[cascadeIndex]);
			surfaceSceneConstantBuffer->upload(&scene);

			SurfaceShadowCascade& cascade = atmosphereData._shadowCascades[cascadeIndex];
            DK::memcpy(cascade.right, atmosphereData._surfaceRight, sizeof(cascade.right));
            DK::memcpy(cascade.up, atmosphereData._surfaceUp, sizeof(cascade.up));
            DK::memcpy(cascade.forward, atmosphereData._surfaceForward, sizeof(cascade.forward));
            DK::memcpy(cascade.origin, atmosphereData._surfaceOrigin, sizeof(cascade.origin));


            cascade.albedoIndex = map._colors[0]._texture->getSRV();
			cascade.depthIndex = map._depth._texture->getSRV();
			cascade.mapSize = map._width;
        } // Keep the outermost capture in the legacy fields for bounce transport.
    }
    void SceneRenderer::updateSurfaceTransport()
    {
        if (_atmosphereConstantBufferData._surfaceMapEnabled == false)
        {
            _surfaceTransportValid = false;
            return;
        }

        const AtmosphereConstantBuffer& surface = _atmosphereConstantBufferData;
        const uint64 now = GetTickCount64();
        const bool changed = _surfaceTransportValid == false || memcmp(&surface, &_cachedSurfaceParameters, sizeof(surface)) != 0;
        const bool updateBounce = changed || now - _lastSurfaceTransportUpdate >= 100u;
        if (updateBounce == false && surface._surfaceShadowStrength <= 0.0f)
			return;

        RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
        renderModule.beginGpuTiming(4);
		if (surface._surfaceShadowStrength > 0.0f)
		{
			for (uint32 i = 0; i < DK_COUNT_OF(_nearShadowPasses); ++i)
			{
				startRenderPass(renderModule, *_nearShadowPasses[i]);
				drawSceneGeometry(renderModule, _nearShadowSceneConstants[i], false);
				endRenderPass();
			}
		}

		startRenderPass(renderModule, *_surfaceCapturePass);
		drawSceneGeometry(renderModule, _surfaceSceneConstantBuffer, false);
		endRenderPass();

        if (updateBounce && _atmosphereConstantBufferData._surfaceBounceStrength > 0.0f)
        {
            startRenderPass(renderModule, *_surfaceBouncePass);
			{
				startPipeline("AtmosphereSurfaceBouncePipeline");
				{
					renderModule.resourceBarrierTransition(_surfaceBounce, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
					setRootConstantParameter("_surfaceBounceOutputUav", _surfaceBounce->getUAV());
					renderModule.dispatch(256u, 16u, 1u);
					renderModule.resourceBarrierTransition(_surfaceBounce, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				}
				endPipeline();
			}
			endRenderPass();
        }
        renderModule.endGpuTiming(4);

		if (updateBounce == false)
			return;

        _cachedSurfaceParameters = surface;
        _lastSurfaceTransportUpdate = now;
        _surfaceTransportValid = true;
        _skyLightingValid = false; // Only invalidate when capture/bounce data were refreshed.
    }
    void SceneRenderer::drawSceneGeometry(RenderModule& renderModule, const IBufferRef& sceneConstants, bool includeDebug)
    {
        RenderModule& currentRenderModule = renderModule;
		if (_renderLocalScene)
		{
#if 0
			startPipeline("SkyDomePipeline");
			{
				setConstantBuffer("SceneConstantBuffer", sceneConstants->GetGPUVirtualAddress());

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
				setConstantBuffer("SceneConstantBuffer", sceneConstants);

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
				setConstantBuffer("SceneConstantBuffer", sceneConstants);

				for (const SceneObject& sceneObject : DuckingEngine::getInstance().GetSceneObjectManager().getSceneObjects())
				{
					if (sceneObject._sceneObjectConstantBuffer == nullptr)
						continue;

					setConstantBuffer("SceneObjectConstantBuffer", sceneObject._sceneObjectConstantBuffer);

					uint32 componentCount = static_cast<uint32>(sceneObject.getComponents().size());
					for (uint32 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
					{
						// #todo- component 완전 개편 필요해보임.
						// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
						// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
						const StaticMeshComponent* staticMeshComponent = dynamic_cast<const StaticMeshComponent*>(sceneObject.getComponents()[componentIndex].get());
						if (staticMeshComponent == nullptr || staticMeshComponent->get_model() == nullptr)
							continue;

						const DKVector<StaticMeshModel::SubMeshType>& subMeshes = staticMeshComponent->get_model()->get_subMeshArr();
						for (uint32 subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
						{
							const StaticMeshModel::SubMeshType& subMesh = subMeshes[subMeshIndex];

							const Material* material = subMesh._material.get();
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
				setConstantBuffer("SceneConstantBuffer", sceneConstants);

				for (const SceneObject& sceneObject : DuckingEngine::getInstance().GetSceneObjectManager().getCharacterSceneObjects())
				{
					if (sceneObject._sceneObjectConstantBuffer == nullptr)
						continue;

					setConstantBuffer("SceneObjectConstantBuffer", sceneObject._sceneObjectConstantBuffer);

					uint32 componentCount = static_cast<uint32>(sceneObject.getComponents().size());
					for (uint32 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
					{
						// #todo- component 완전 개편 필요해보임.
						// for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
						// 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
						const SkinnedMeshComponent* skinnedMeshComponent = static_cast<const SkinnedMeshComponent*>(sceneObject.getComponents()[componentIndex].get());
						if (skinnedMeshComponent == nullptr || skinnedMeshComponent->_model == nullptr)
							continue;

						setConstantBuffer("SkeletonConstantBuffer", skinnedMeshComponent->_currentAnimationBufferCS);

						const DKVector<SkinnedMeshModel::SubMeshType>& subMeshes = skinnedMeshComponent->_model->get_subMeshArr();
						for (uint32 subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
						{
							const SkinnedMeshModel::SubMeshType& subMesh = subMeshes[subMeshIndex];

							const Material* material = subMesh._material.get();
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
            if (includeDebug)
            {
			EditorDebugDrawManager& debugDrawManager = EditorDebugDrawManager::getSingleton();
			const DKVector<EditorDebugDrawManager::SpherePrimitiveInfo>& spherePrimitiveInfo = debugDrawManager.get_primitiveInfoSphereArr();
			const uint32 sphereInstanceCount = static_cast<uint32>(spherePrimitiveInfo.size());
			if (sphereInstanceCount)
			{
				startPipeline("SpherePipeline");
				{
					setConstantBuffer("SceneConstantBuffer", sceneConstants);
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
					setConstantBuffer("SceneConstantBuffer", sceneConstants);
					setShaderResourceView("LinePrimitiveInfoBuffer", debugDrawManager.get_primitiveInfoLineBuffer());
					renderModule.setVertexBuffers(0, 1, EditorDebugDrawManager::LinePrimitiveInfo::kVertexBufferView.get());
					renderModule.setIndexBuffer(EditorDebugDrawManager::LinePrimitiveInfo::kIndexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(EditorDebugDrawManager::LinePrimitiveInfo::indexCount), lineInstanceCount, 0, 0, 0);
				}
				endPipeline();
			}
			debugDrawManager.endUpdateRender();
            }
#endif
		}
    }

    void SceneRenderer::drawTerrainGrass(RenderModule& renderModule)
    {
        const SceneManager::ClipMapTerrain& terrain = DuckingEngine::getInstance().getSceneManagerWritable().getClipMapTerrainWritable();
        if (terrain._grassIndexBuffer == nullptr)
            return;

        const float3 camera = Camera::gMainCamera->get_worldTransform().get_translation();
        // The current heightmap is 2048 samples at 1 meter per sample.
        constexpr float mapHalfExtent = 1024.0f;
        constexpr float tileMeters = GRASS_TILE_METERS;
        constexpr float maxDistance = GRASS_LOD4_END;
        // Heights that admit grass lie between sea level and the height encoding's top.
        constexpr float maxGrassY = 113.2f;
        const auto tileAt = [&](float position) { return static_cast<int>(std::floor(position / tileMeters)); };
        const float minX = std::max(-mapHalfExtent, camera.x - maxDistance);
        const float minZ = std::max(-mapHalfExtent, camera.z - maxDistance);
        const float maxX = std::min(mapHalfExtent, camera.x + maxDistance);
        const float maxZ = std::min(mapHalfExtent, camera.z + maxDistance);
        if (minX >= maxX || minZ >= maxZ)
            return;

        const auto toDirectXMatrix = [](const float4x4& matrix)
        {
            return DirectX::XMMATRIX(
                matrix._11, matrix._12, matrix._13, matrix._14,
                matrix._21, matrix._22, matrix._23, matrix._24,
                matrix._31, matrix._32, matrix._33, matrix._34,
                matrix._41, matrix._42, matrix._43, matrix._44);
        };
        DirectX::BoundingFrustum viewFrustum;
        // Grass already has a finite distance budget. Build its culling volume
        // explicitly: unprojecting depth zero from infinite reversed Z has w=0.
        viewFrustum.RightSlope = 1.0f / _sceneConstantBufferData._cameraProjectionMatrix._11;
        viewFrustum.LeftSlope = -viewFrustum.RightSlope;
        viewFrustum.TopSlope = 1.0f / _sceneConstantBufferData._cameraProjectionMatrix._22;
        viewFrustum.BottomSlope = -viewFrustum.TopSlope;
        viewFrustum.Near = _sceneConstantBufferData._nearDistance;
        viewFrustum.Far = maxDistance + 2.0f * maxGrassY + 4.0f * tileMeters;
        DirectX::BoundingFrustum worldFrustum;
        viewFrustum.Transform(worldFrustum, toDirectXMatrix(_sceneConstantBufferData._cameraWorldMatrix));

        constexpr float lodEnds[] = { GRASS_LOD0_END, GRASS_LOD1_END, GRASS_LOD2_END, GRASS_LOD3_END, GRASS_LOD4_END };
        const int firstTileX = tileAt(minX);
        const int lastTileX = tileAt(maxX);
        const int firstTileZ = tileAt(minZ);
        const int lastTileZ = tileAt(maxZ);
        constexpr float bladeOverhang = 1.2f;
        RenderModule& currentRenderModule = renderModule;
        startPipeline("GrassPatchPipeline");
        setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
        setConstantBuffer("TerrainClipMap", terrain._material->get_parameterBufferForGPU());
        renderModule.setIndexBuffer(terrain._grassIndexBufferView.get());
        for (int tileZ = firstTileZ; tileZ <= lastTileZ; ++tileZ)
        {
            for (int tileX = firstTileX; tileX <= lastTileX; ++tileX)
            {
                const float x = tileX * tileMeters;
                const float z = tileZ * tileMeters;
                const float dx = camera.x - Math::clamp(camera.x, x, x + tileMeters);
                const float dy = camera.y - Math::clamp(camera.y, 0.0f, maxGrassY);
                const float dz = camera.z - Math::clamp(camera.z, z, z + tileMeters);
                const float distanceSquared = dx * dx + dy * dy + dz * dz;
                if (distanceSquared >= maxDistance * maxDistance)
                    continue;

                const DirectX::BoundingBox bounds(
                    DirectX::XMFLOAT3(x + 0.5f * tileMeters, 0.5f * maxGrassY, z + 0.5f * tileMeters),
                    DirectX::XMFLOAT3(0.5f * tileMeters + bladeOverhang, 0.5f * maxGrassY, 0.5f * tileMeters + bladeOverhang));
                if (!worldFrustum.Intersects(bounds))
                    continue;

                uint32 lod = 0;
                while (distanceSquared >= lodEnds[lod] * lodEnds[lod])
                    ++lod;
                const uint32 stride = 1u << lod;
                const uint32 gridSide = GRASS_TILE_GRID_SIDE / stride;
                setRootConstantParameter("_grassTileX", tileX);
                setRootConstantParameter("_grassTileZ", tileZ);
                setRootConstantParameter("_grassCellStride", stride);
                renderModule.drawIndexedInstanced(SceneManager::ClipMapTerrain::GRASS_INDEX_COUNT, gridSide * gridSide, 0, 0, 0);
            }
        }
        endPipeline();
    }

	void SceneRenderer::updateAtmosphereTransmittanceLut()
	{
		const AtmosphereConstantBuffer& p = _atmosphereConstantBufferData;
		// The sky-lighting compute pass always uses the bounded transmittance LUT.
		const AtmosphereConstantBuffer& previous = _cachedAtmosphereLutParameters;
		const bool dirty = _atmosphereLutValid == false
			|| memcmp(p._absorptionLayers, previous._absorptionLayers, sizeof(p._absorptionLayers)) != 0
			|| p._densityReferenceRadius != previous._densityReferenceRadius
			|| p._densityTransitionRadius != previous._densityTransitionRadius
			|| p._atmosphereRadius != previous._atmosphereRadius
			|| p._rayleighHeight != previous._rayleighHeight
			|| p._mieHeight != previous._mieHeight
			|| p._scatteringScale != previous._scatteringScale
            || p._rayleighScale != previous._rayleighScale
            || p._mieScale != previous._mieScale
            || p._mieAbsorptionScale != previous._mieAbsorptionScale
            || p._opticalNodeBudget != previous._opticalNodeBudget
			|| p._integrationRelativeTolerance != previous._integrationRelativeTolerance
			|| p._integrationMaxDepth != previous._integrationMaxDepth
			|| p._opticalDepthPointCount != previous._opticalDepthPointCount;
		if (dirty == false)
			return;

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		renderModule.beginGpuTiming(0);
        startRenderPass(renderModule, *_transmittancePass);
		{
			startPipeline("AtmosphereTransmittancePipeline");
			{
				renderModule.resourceBarrierTransition(_atmosphereTransmittanceLut, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
				setRootConstantParameter("_atmosphereLutUav", _atmosphereTransmittanceLut->getUAV());
				// RenderModule::dispatch accepts thread counts, not group counts.
				renderModule.dispatch(kAtmosphereLutWidth, kAtmosphereLutHeight, 1);
				renderModule.resourceBarrierTransition(_atmosphereTransmittanceLut, static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
			}
			endPipeline();
		}
		endRenderPass();
        renderModule.endGpuTiming(0);

		_skyLightingValid = false;
        _multipleScatteringValid = false;
		_cachedAtmosphereLutParameters = p;
		_atmosphereLutValid = true;
	}

    void SceneRenderer::updateAtmosphereMultipleScattering()
    {
        const AtmosphereConstantBuffer& p = _atmosphereConstantBufferData;
        if (p._multipleScatteringStrength <= 0.0f || p._multipleScatteringMaxOrder <= 1u)
			return;
        if (_multipleScatteringValid && p._mieAnisotropy == _cachedMultipleAnisotropy && p._multipleScatteringMaxOrder == _cachedMultipleMaxOrder)
			return;

        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        startRenderPass(rm, *_multipleScatteringPass);
        for (uint32 iteration = 0; iteration < p._multipleScatteringMaxOrder - 1u; ++iteration)
        {
			const uint32 output = iteration & 1u;
			const uint32 input = output ^ 1u;
            startPipeline("AtmosphereMultipleScatteringPipeline");
			{
				rm.resourceBarrierTransition(_multipleScatteringLut[output], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
				setRootConstantParameter("_multipleOutputUav", _multipleScatteringLut[output]->getUAV());
				setRootConstantParameter("_multipleInputSrv", _multipleScatteringLut[input]->getSRV());
				setRootConstantParameter("_multipleIteration", iteration);
				rm.dispatch(32, 32, 1);
				rm.resourceBarrierTransition(_multipleScatteringLut[output], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			}
            endPipeline();
        }
        const uint32 final = (p._multipleScatteringMaxOrder - 2u) & 1u;
        rm.resourceBarrierTransition(_multipleScatteringLut[final], static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
        endRenderPass();

        _cachedMultipleAnisotropy = p._mieAnisotropy;
        _cachedMultipleMaxOrder = p._multipleScatteringMaxOrder;
        _multipleScatteringValid = true;
        _skyLightingValid = false;
    }

    void SceneRenderer::updateAtmosphereSkyLighting()
    {
        const AtmosphereConstantBuffer& p = _atmosphereConstantBufferData;
        const AtmosphereConstantBuffer& old = _cachedSkyParameters;
        const float3 position = Camera::gMainCamera->get_worldTransform().get_translation();
        bool dirty = !_skyLightingValid || p._multipleScatteringStrength != old._multipleScatteringStrength
            || p._multipleScatteringMaxOrder != old._multipleScatteringMaxOrder || p._surfaceMapEnabled != old._surfaceMapEnabled
            || p._surfaceShadowStrength != old._surfaceShadowStrength || p._surfaceBounceStrength != old._surfaceBounceStrength
            || p._sunDegree != old._sunDegree
            || p._sunIrradiance != old._sunIrradiance || p._mieAnisotropy != old._mieAnisotropy;
        for (uint32 i = 0; i < 3; ++i)
            dirty = dirty || p._sunColor[i] != old._sunColor[i] || p._planetCentre[i] != old._planetCentre[i];
        dirty = dirty || position.x != _cachedSkyPosition[0] || position.y != _cachedSkyPosition[1] || position.z != _cachedSkyPosition[2];
        if (dirty == false)
			return;

        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        rm.beginGpuTiming(1);
        startRenderPass(rm, *_skyLightingPass);
		{
			startPipeline("AtmosphereSkyRadiancePipeline");
			{
				rm.resourceBarrierTransition(_skyRadiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
				setRootConstantParameter("_skyOutputUav", _skyRadiance->getUAV());
				rm.dispatch(64, 32, 1);
				rm.resourceBarrierTransition(_skyRadiance, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			}
			endPipeline();
			startPipeline("AtmosphereSkyFilterPipeline");
			{
				rm.resourceBarrierTransition(_skyFiltered, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
				setRootConstantParameter("_skyOutputUav", _skyFiltered->getUAV());
				rm.dispatch(64, 192, 1);
				rm.resourceBarrierTransition(_skyFiltered, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
			endPipeline();
		}
        endRenderPass();
        rm.endGpuTiming(1);

        _cachedSkyParameters = p;
        _cachedSkyPosition[0] = position.x; _cachedSkyPosition[1] = position.y; _cachedSkyPosition[2] = position.z;
        _skyLightingValid = true;
    }

	void SceneRenderer::updateRender() noexcept
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		renderModule.preRender();

		updateAtmosphereTransmittanceLut();
        updateAtmosphereMultipleScattering();
        updateSurfaceTransport();
        updateAtmosphereSkyLighting();

        //RaytracingRenderer& raytracingRenderer = DuckingEngine::getInstance().GetRaytracingRendererWritable();
        //const bool raytraced = raytracingRenderer.updateRaytracingRenderer(renderModule);
        //if (raytraced)
        //    raytracingRenderer.dispatchRay(renderModule, _sceneConstantBuffer);
        //if (!raytraced)
		const RenderPassTargets& gbuffer = _mainGeometryPass->_binding._target->_targets;
		const ITextureRef& lighting = _gbufferResolvePass->_binding._target->_targets._colors[0]._texture;
		const ITextureRef& atmosphere = _atmospherePass->_binding._target->_targets._colors[0]._texture;
		const ITextureRef& horizontal = _bloomHorizontalPass->_binding._target->_targets._colors[0]._texture;
		const ITextureRef& vertical = _bloomVerticalPass->_binding._target->_targets._colors[0]._texture;
        {
		if (_renderLocalScene)
		{
		startRenderPass(renderModule, *_oceanPass);
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

		// MainRender
		if (_renderLocalScene)
			renderModule.beginRenderPass(*_mainGeometryPass);
		else
			renderModule.beginRenderPass(*_mainGeometryClearPass);
		drawSceneGeometry(renderModule, _sceneConstantBuffer, true);
		if (_renderLocalScene)
			drawTerrainGrass(renderModule);
		renderModule.finishRenderPass();

		// Resolve material attributes to HDR radiance before atmosphere and bloom.
		renderModule.beginGpuTiming(2);
        startRenderPass(renderModule, *_gbufferResolvePass);
		{
			startPipeline("GBufferPipeline");
			{
				for (uint32 i = 0; i < gbuffer._colorCount; ++i)
					renderModule.resourceBarrierTransition(gbuffer._colors[i]._texture->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				renderModule.resourceBarrierTransition(gbuffer._depth._texture->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				setRootConstantParameter("_baseColorIndex", gbuffer._colors[0]._texture->getSRV());
				setRootConstantParameter("_normalRoughnessIndex", gbuffer._colors[1]._texture->getSRV());
				setRootConstantParameter("_materialIndex", gbuffer._colors[2]._texture->getSRV());
				setRootConstantParameter("_depthIndex", gbuffer._depth._texture->getSRV());
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
				SceneManager::PostProcess& postProcess = DuckingEngine::getInstance().getSceneManagerWritable().getPostProcessWritable();
				renderModule.setVertexBuffers(0, 1, postProcess._mesh._vertexBufferView.get());
				renderModule.setIndexBuffer(postProcess._mesh._indexBufferView.get());
				renderModule.drawIndexedInstanced(static_cast<UINT>(postProcess._mesh._indexCount), 1, 0, 0, 0);
			}
			endPipeline();
		}
		endRenderPass();
		renderModule.endGpuTiming(2);

        renderModule.beginGpuTiming(3);
        startRenderPass(renderModule, *_atmospherePass);
		{
			startPipeline("AtmospherePipeline");
			{
				setRootConstantParameter("_materialIndex", gbuffer._colors[2]._texture->getSRV());
				renderModule.resourceBarrierTransition(lighting->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				setRootConstantParameter("_lightingIndex", lighting->getSRV());
				setRootConstantParameter("_sceneDepthIndex", gbuffer._depth._texture->getSRV());
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

		renderModule.endGpuTiming(3);

        // The horizontal pass can reuse the lighting output after atmosphere has read it.
		if (_atmosphereConstantBufferData._bloomStrength > 0.0f)
		{
			SceneManager::PostProcess& postProcess = DuckingEngine::getInstance().getSceneManagerWritable().getPostProcessWritable();
			startRenderPass(renderModule, *_bloomHorizontalPass);
			startPipeline("BloomHorizontalPipeline");
			renderModule.resourceBarrierTransition(atmosphere->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			setRootConstantParameter("_bloomSourceIndex", atmosphere->getSRV());
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
			setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
			renderModule.setVertexBuffers(0, 1, postProcess._mesh._vertexBufferView.get());
			renderModule.setIndexBuffer(postProcess._mesh._indexBufferView.get());
			renderModule.drawIndexedInstanced(static_cast<UINT>(postProcess._mesh._indexCount), 1, 0, 0, 0);
			endPipeline();
			endRenderPass();

			// The second bloom pass blurs vertically and composes with the original HDR output.
			startRenderPass(renderModule, *_bloomVerticalPass);
			startPipeline("BloomVerticalPipeline");
			renderModule.resourceBarrierTransition(horizontal->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			setRootConstantParameter("_bloomSourceIndex", horizontal->getSRV());
			setRootConstantParameter("_originalHdrIndex", atmosphere->getSRV());
			setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
			setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
			renderModule.setVertexBuffers(0, 1, postProcess._mesh._vertexBufferView.get());
			renderModule.setIndexBuffer(postProcess._mesh._indexBufferView.get());
			renderModule.drawIndexedInstanced(static_cast<UINT>(postProcess._mesh._indexCount), 1, 0, 0, 0);
			endPipeline();
			endRenderPass();
		}

		// Final output only applies exposure and tone mapping to the selected HDR source.
		startRenderPass(renderModule, *_finalOutputPass);
		{
			startPipeline("FinalOutputPipeline");
			{
				const ITextureRef& source = _atmosphereConstantBufferData._bloomStrength > 0.0f ? vertical : atmosphere;
				renderModule.resourceBarrierTransition(source->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				const uint32 outputSourceSRV = source->getSRV();
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
