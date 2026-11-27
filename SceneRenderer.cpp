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
#if defined(_DK_DEBUG_)
#include "EditorDebugDrawManager.h"
#endif
#include "SceneManager.h"
#include "ShaderCompiler.h"
#include "Resource/Material/GrassTiles.h"
#include "Resource/Material/Atmosphere/CloudLimits.h"

#include <DirectXCollision.h>
#include <random>

namespace DK
{
    static bool atmosphereUsesSurfaceBounce(const AtmosphereConstantBuffer& atmosphere)
    {
        return atmosphere._surfaceBounceStrength > 0.0f && (atmosphere._atmosphereSamplingMode != ATMOSPHERE_PREVIEW || atmosphere._previewSurfaceBounce != 0u);
    }

#if defined(_DK_DEBUG_)
    void AtmosphereAbsorptionLayer::validate([[maybe_unused]] uint32 layerIndex) const
    {
        for (uint32 channel = 0; channel < DK_COUNT_OF(_extinction); ++channel)
            DK_ASSERT_LOG(std::isfinite(_extinction[channel]) && _extinction[channel] >= 0.0f, "Absorption layer %u: invalid _extinction[%u]=%g", layerIndex, channel, _extinction[channel]);
        DK_ASSERT_LOG(std::isfinite(_strength) && _strength >= 0.0f, "Absorption layer %u: invalid _strength=%g", layerIndex, _strength);
        DK_ASSERT_LOG(std::isfinite(_peakRadius) && _peakRadius >= 0.0f, "Absorption layer %u: invalid _peakRadius=%g", layerIndex, _peakRadius);
        DK_ASSERT_LOG(std::isfinite(_lowerWidth) && _lowerWidth > 0.0f, "Absorption layer %u: invalid _lowerWidth=%g", layerIndex, _lowerWidth);
        DK_ASSERT_LOG(std::isfinite(_upperWidth) && _upperWidth > 0.0f, "Absorption layer %u: invalid _upperWidth=%g", layerIndex, _upperWidth);
        if (_strength > 0.0f && (_extinction[0] > 0.0f || _extinction[1] > 0.0f || _extinction[2] > 0.0f))
            DK_ASSERT_LOG(_peakRadius >= _lowerWidth, "Absorption layer %u: inner radius is negative, _peakRadius=%g, _lowerWidth=%g", layerIndex, _peakRadius, _lowerWidth);
    }

    void CloudConstantBuffer::validate() const
    {
        DK_ASSERT_LOG(std::isfinite(_cloudBaseHeight) && _cloudBaseHeight >= 0.0f && std::isfinite(_cloudTopHeight) && _cloudTopHeight > _cloudBaseHeight, "Invalid cloud base or top height");
        DK_ASSERT_LOG(_cloudCoverage >= 0.0f && _cloudCoverage <= 1.0f, "Cloud coverage must be in [0, 1]");
        DK_ASSERT_LOG(std::isfinite(_cloudExtinction) && _cloudExtinction > 0.0f, "Cloud extinction must be positive");
        DK_ASSERT_LOG(std::isfinite(_cloudWeatherPeriod) && _cloudWeatherPeriod > 0.0f && std::isfinite(_cloudShapePeriod) && _cloudShapePeriod > 0.0f && std::isfinite(_cloudDetailPeriod) && _cloudDetailPeriod > 0.0f, "Invalid cloud noise scale");
        DK_ASSERT_LOG(std::isfinite(_cloudWindSpeedXZ[0]) && std::isfinite(_cloudWindSpeedXZ[1]), "Invalid cloud wind speed");
        DK_ASSERT_LOG(_cloudScatteringAlbedo >= 0.0f && _cloudScatteringAlbedo <= 1.0f, "Cloud scattering albedo must be in [0, 1]");
    }

    void AtmosphereConstantBuffer::validate() const
    {
        DK_ASSERT_LOG(_atmosphereSamplingMode <= ATMOSPHERE_PREVIEW, "Invalid atmosphere sampling mode: %u", _atmosphereSamplingMode);
        DK_ASSERT_LOG(_previewViewIntervalCount >= ATMOSPHERE_PREVIEW_MIN_VIEW_INTERVALS && _previewViewIntervalCount <= ATMOSPHERE_PREVIEW_MAX_VIEW_INTERVALS, "Invalid preview view interval count: %u", _previewViewIntervalCount);
        DK_ASSERT_LOG(std::isfinite(_densityReferenceRadius) && _densityReferenceRadius > 0.0f, "Invalid _densityReferenceRadius=%g", _densityReferenceRadius);
        DK_ASSERT_LOG(std::isfinite(_densityTransitionRadius) && _densityTransitionRadius > 0.0f && _densityTransitionRadius < _densityReferenceRadius, "Invalid _densityTransitionRadius=%g, _densityReferenceRadius=%g", _densityTransitionRadius, _densityReferenceRadius);
        DK_ASSERT_LOG(std::isfinite(_atmosphereRadius) && _atmosphereRadius > _densityTransitionRadius, "Invalid _atmosphereRadius=%g, _densityTransitionRadius=%g", _atmosphereRadius, _densityTransitionRadius);
        DK_ASSERT_LOG(_atmosphereRadius - _densityTransitionRadius <= ATMOSPHERE_LUT_MAX_RADIAL_DEPTH, "Transmittance LUT diagnostic range exceeded: _atmosphereRadius=%g, _densityTransitionRadius=%g, maximum span=%g", _atmosphereRadius, _densityTransitionRadius, ATMOSPHERE_LUT_MAX_RADIAL_DEPTH);
        DK_ASSERT_LOG(std::isfinite(_rayleighHeight) && _rayleighHeight > 0.0f, "Invalid _rayleighHeight=%g", _rayleighHeight);
        DK_ASSERT_LOG(std::isfinite(_mieHeight) && _mieHeight > 0.0f, "Invalid _mieHeight=%g", _mieHeight);
        DK_ASSERT_LOG(std::isfinite(_scatteringScale) && _scatteringScale >= 0.0f, "Invalid _scatteringScale=%g", _scatteringScale);
        DK_ASSERT_LOG(std::isfinite(_rayleighScale) && _rayleighScale >= 0.0f, "Invalid _rayleighScale=%g", _rayleighScale);
        DK_ASSERT_LOG(std::isfinite(_mieScale) && _mieScale >= 0.0f, "Invalid _mieScale=%g", _mieScale);
        DK_ASSERT_LOG(std::isfinite(_mieAbsorptionScale) && _mieAbsorptionScale >= 0.0f, "Invalid _mieAbsorptionScale=%g", _mieAbsorptionScale);
        DK_ASSERT_LOG(std::isfinite(_mieAnisotropy), "Invalid _mieAnisotropy=%g", _mieAnisotropy);
        DK_ASSERT_LOG(std::isfinite(_sunDegree), "Invalid _sunDegree=%g", _sunDegree);
        DK_ASSERT_LOG(std::isfinite(_sunIrradiance) && _sunIrradiance >= 0.0f, "Invalid _sunIrradiance=%g", _sunIrradiance);
        for (uint32 channel = 0; channel < DK_COUNT_OF(_sunColor); ++channel)
        {
            DK_ASSERT_LOG(std::isfinite(_sunColor[channel]) && _sunColor[channel] >= 0.0f, "Invalid _sunColor[%u]=%g", channel, _sunColor[channel]);
            DK_ASSERT_LOG(std::isfinite(_planetCentre[channel]), "Invalid _planetCentre[%u]=%g", channel, _planetCentre[channel]);
        }
        DK_ASSERT_LOG(_surfaceBounceSamples <= SURFACE_BOUNCE_MAX_SAMPLES, "Invalid _surfaceBounceSamples=%u, maximum=%u", _surfaceBounceSamples, SURFACE_BOUNCE_MAX_SAMPLES);
        DK_ASSERT_LOG(_shadowCascadeCount <= DK_ATMOSPHERE_SHADOW_CASCADE_COUNT, "Invalid _shadowCascadeCount=%u", _shadowCascadeCount);
        for (uint32 index = 0; index < DK_COUNT_OF(_absorptionLayers); ++index)
            _absorptionLayers[index].validate(index);
    }
#endif

	static bool intersectsGeometryFrustum(const StaticMeshInstanceChunk& chunk, const DKArray<float4, 6>& frustumPlanes)
	{
		for (const float4& plane : frustumPlanes)
		{
			const float x = plane.x >= 0.0f ? chunk._maximum.x : chunk._minimum.x;
			const float y = plane.y >= 0.0f ? chunk._maximum.y : chunk._minimum.y;
			const float z = plane.z >= 0.0f ? chunk._maximum.z : chunk._minimum.z;
			if (plane.x * x + plane.y * y + plane.z * z + plane.w < 0.0f)
				return false;
		}
		return true;
	}

	static std::random_device rd;
	static std::mt19937 gen(rd());
	//static std::normal_distribution<float> dist(0, 1);

	bool SceneRenderer::initialize()
	{
#if defined(_DK_DEBUG_)
        _atmosphereConstantBufferData._atmosphereSamplingMode = ATMOSPHERE_PREVIEW;
        _atmosphereConstantBufferData.validate();
        validateSurfaceCaptureParameters();
        _cloudConstantBufferData.validate();
#endif
		if (initialize_createRenderPass() == false)
			return false;
		if (initialize_renderPassHandles() == false || initialize_createMaterialDefinition() == false)
			return false;
		return initialize_createSceneConstantBuffer();
	}

	bool SceneRenderer::initialize_renderPassHandles()
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

		_cloudNoisePass = renderModule.findRenderPass("CloudNoisePass");
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

		if (_cloudNoisePass == nullptr || _transmittancePass == nullptr || _multipleScatteringPass == nullptr ||
			_surfaceBouncePass == nullptr || _skyLightingPass == nullptr || _oceanPass == nullptr ||
			_gbufferResolvePass == nullptr || _atmospherePass == nullptr || _bloomHorizontalPass == nullptr ||
			_bloomVerticalPass == nullptr || _finalOutputPass == nullptr || _mainGeometryPass == nullptr ||
			_mainGeometryClearPass == nullptr || _surfaceCapturePass == nullptr)
			return false;

		using Size = RenderTargetDefinition::Size;
		bool (*const hasGeometryOutputs)(const RenderTarget*) = [](const RenderTarget* target)
		{
			if (target == nullptr)
				return false;

			const RenderTargetDefinition& definition = target->_definition;
			DK_ASSERT_LOG(definition._colorCount != 3 || (definition._colors[0]._nameHash == StringUtil::hash("Albedo") && definition._colors[1]._nameHash == StringUtil::hash("Normal") && definition._colors[2]._nameHash == StringUtil::hash("Material")), "Invalid geometry output semantics");
			return definition._colorCount == 3 && definition._depth._srv &&
				definition._colors[0]._srv && definition._colors[1]._srv && definition._colors[2]._srv;
		};
		const DKFunction<bool(const RenderPassInstance&)> isCapture = [&](const RenderPassInstance& pass)
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
		parameter._bufferNameHash = StringUtil::registerHash(bufferName);
#if defined(_DK_DEBUG_)
		parameter._bufferName = bufferName;
#endif
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

		struct FormatEntry
		{
			const char* _name;
			DXGI_FORMAT _format;
			bool _depth;
		};
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
			if (entry._depth == depth && StringUtil::strcmp(entry._name, format) == 0)
			{
				attachment._format = entry._format;
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

#if defined(_DK_DEBUG_)
				for (uint32 i = 0; i < definition._colorCount; ++i)
				{
					DK_ASSERT_LOG(definition._colors[i]._nameHash != StringUtil::hash(name), "Duplicate render target attachment name: %s", name);
				}
#endif

				RenderTargetDefinition::Attachment& color = definition._colors[definition._colorCount++];
#if defined(_DK_DEBUG_)
				color._nameHash = StringUtil::registerHash(name);
				color._name = name;
#endif
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
#if defined(_DK_DEBUG_) == 0
			const char* debugOnly = renderPassNode->Attribute("DebugOnly");
			if (debugOnly != nullptr && StringUtil::strcmp(debugOnly, "True") == 0)
				continue;
#endif
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
#if defined(_DK_DEBUG_) == 0
				const char* debugOnly = renderPassChildNode->Attribute("DebugOnly");
				if (debugOnly != nullptr && StringUtil::strcmp(debugOnly, "True") == 0)
					continue;
#endif
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

					if (appendPipelinePermutations(*renderPassChildNode, pipelineName, pipelineCreateInfo, renderPassCreateInfo) == false)
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

					if (appendPipelinePermutations(*renderPassChildNode, pipelineName, pipelineCreateInfo, renderPassCreateInfo) == false)
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
			materialDefinition.setMaterialName(materialName.c_str());

			using FindResult = DKHashMap<uint64, MaterialDefinition>::iterator;
			FindResult findResult = _materialDefinitionMap.find(StringUtil::hash(materialName));
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
				parameterDefinition.setName(name);
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

			using InsertResult = DKPair<DKHashMap<uint64, MaterialDefinition>::iterator, bool>;
			InsertResult insertResult = _materialDefinitionMap.insert(DKPair<uint64, MaterialDefinition>(StringUtil::registerHash(materialName), DK::move(materialDefinition)));
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

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

		_sceneConstantBuffer = renderModule.createConstantBuffer(sizeof(SceneConstantBuffer), DK_DEBUG_NAME(L"SceneConstantBuffer"));

		_cloudConstantBuffer = renderModule.createConstantBuffer(sizeof(CloudConstantBuffer), DK_DEBUG_NAME(L"CloudConstantBuffer"));
		_cloudNoise = renderModule.createTexture3D(DK_DEBUG_NAME("CloudNoise"), DK_CLOUD_NOISE_SIZE, DK_CLOUD_NOISE_SIZE, DK_CLOUD_NOISE_SIZE, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true, true);
		_cloudWeather = renderModule.createTexture(DK_DEBUG_NAME("CloudWeather"), DK_CLOUD_WEATHER_SIZE, DK_CLOUD_WEATHER_SIZE, nullptr, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true, true);
		if (_cloudConstantBuffer == nullptr || _cloudNoise == nullptr || _cloudWeather == nullptr)
			return false;

		_cloudConstantBufferData._cloudNoiseIndex = _cloudNoise->getSRV();
		_cloudConstantBufferData._cloudWeatherIndex = _cloudWeather->getSRV();

		_atmosphereConstantBuffer = renderModule.createConstantBuffer(sizeof(AtmosphereConstantBuffer), DK_DEBUG_NAME(L"AtmosphereConstantBuffer"));
		_atmosphereTransmittanceLut = renderModule.createTexture(DK_DEBUG_NAME("AtmosphereTransmittanceLut"), kAtmosphereLutWidth, kAtmosphereLutHeight * 2u, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, true);
		if (_atmosphereTransmittanceLut == nullptr)
			return false;

		for (uint32 i = 0; i < DK_COUNT_OF(_multipleScatteringLut); ++i)
		{
			_multipleScatteringLut[i] = renderModule.createTexture(DK_DEBUG_NAME(i == 0 ? "AtmosphereMultipleScattering0" : "AtmosphereMultipleScattering1"), ATMOSPHERE_MULTIPLESCATTERING_WIDTH, ATMOSPHERE_MULTIPLESCATTERING_HEIGHT, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true, true);
			if (_multipleScatteringLut[i] == nullptr)
				return false;
		}

		_skyRadiance = renderModule.createTexture(DK_DEBUG_NAME("AtmosphereSkyRadiance"), SKY_WIDTH, SKY_HEIGHT, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true, true);
		if (_skyRadiance == nullptr)
			return false;
		_skyFiltered = renderModule.createTexture(DK_DEBUG_NAME("AtmosphereSkyFiltered"), SKY_WIDTH, SKY_HEIGHT * SKY_BANDS, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, true);
		if (_skyFiltered == nullptr)
			return false;

        _surfaceSceneConstantBuffer = renderModule.createConstantBuffer(sizeof(SceneConstantBuffer), DK_DEBUG_NAME(L"SurfaceCaptureSceneConstantBuffer"));
		if (_surfaceSceneConstantBuffer == nullptr)
			return false;
        _surfaceBounce = renderModule.createTexture(DK_DEBUG_NAME("Atmosphere surface bounce field"), SURFACE_BOUNCE_GRID * SURFACE_BOUNCE_GRID, SURFACE_BOUNCE_GRID, nullptr, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true, true);
        if (_surfaceBounce == nullptr)
			return false;

		for (uint32 i = 0; i < DK_COUNT_OF(_nearShadowSceneConstants); ++i)
		{
			_nearShadowSceneConstants[i] = renderModule.createConstantBuffer(sizeof(SceneConstantBuffer), DK_DEBUG_NAME(L"ShadowCascadeSceneConstants"));
			if (_nearShadowSceneConstants[i] == nullptr)
				return false;
		}

		AtmosphereConstantBuffer& surface = _atmosphereConstantBufferData;
		const RenderPassTargets& capture = _surfaceCapturePass->_binding._target->_targets;
        DK_ASSERT_LOG(capture._width > 0 && capture._width == capture._height && capture._width % (2u * SURFACE_BOUNCE_GRID) == 0u, "Surface capture must be square and its side must be an even multiple of the bounce grid: %u x %u", capture._width, capture._height);
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

	const MaterialDefinition* SceneRenderer::getMaterialDefinition(uint64 materialNameHash) const
	{
		using FindResult = DKHashMap<uint64, MaterialDefinition>::const_iterator;
		FindResult findResult = _materialDefinitionMap.find(materialNameHash);
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
#if defined(_DK_DEBUG_)
        _atmosphereConstantBufferData.validate();
#endif
		const float3 cameraPosition = Camera::gMainCamera->get_worldTransform().get_translation();
		const DKArray<double, 3>& preciseCamera = Camera::gMainCamera->getWorldPosition();
		DuckingEngine::getInstance().getSceneManagerWritable().prepareTerrainStreaming(preciseCamera[0], preciseCamera[2], deltaTime);

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
        const TerrainStreamingConstants& terrainBounds = DuckingEngine::getInstance().getSceneManagerWritable().getClipMapTerrainWritable()._streamingConstants;
        const double terrainLocalX = preciseCamera[0] - terrainBounds._terrainOriginWorldX;
        const double terrainLocalZ = preciseCamera[2] - terrainBounds._terrainOriginWorldZ;
        const double terrainX = terrainLocalX - std::clamp(terrainLocalX, double(terrainBounds._terrainMinimumSampleX) * terrainBounds._terrainSampleWorldSpacing, (double(terrainBounds._terrainMinimumSampleX) + terrainBounds._terrainCellCountX) * terrainBounds._terrainSampleWorldSpacing);
        const double terrainZ = terrainLocalZ - std::clamp(terrainLocalZ, double(terrainBounds._terrainMinimumSampleZ) * terrainBounds._terrainSampleWorldSpacing, (double(terrainBounds._terrainMinimumSampleZ) + terrainBounds._terrainCellCountZ) * terrainBounds._terrainSampleWorldSpacing);
        const double terrainY = preciseCamera[1] - std::clamp(preciseCamera[1], double(terrainBounds._terrainBaseWorldHeight), double(terrainBounds._terrainBaseWorldHeight + terrainBounds._terrainWorldHeightRange));
        const double terrainRadius = double(SceneManager::ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE * 2u) * SceneManager::ClipMapTerrain::BASE_VERTEX_SPACING * (1u << (SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS - 1u));
        const bool nearTerrain = terrainBounds._terrainCellCountX > 0 && terrainX * terrainX + terrainY * terrainY + terrainZ * terrainZ < terrainRadius * terrainRadius;
        _renderLocalScene = nearTerrain || gx * gx + gy * gy + gz * gz < double(_localGeometryRadius) * _localGeometryRadius;
#if defined(USE_IMGUI)
        _renderLocalScene = _localGeometryEnabled && _renderLocalScene;
#endif

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
            if (atmosphereUsesSurfaceBounce(atmosphere) == false)
                relativeAtmosphere._surfaceBounceStrength = 0.0f;
			_atmosphereConstantBuffer->upload(&relativeAtmosphere);
#if defined(_DK_DEBUG_)
			_cloudConstantBufferData.validate();
#endif
			_cloudConstantBuffer->upload(&_cloudConstantBufferData);
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

			OceanParams params(
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

			const TerrainStreamingConstants& streaming = terrain._streamingConstants;
			const float2 cameraLocalXZ(streaming._terrainCameraLocalX, streaming._terrainCameraLocalZ);

			const float vertexScale = SceneManager::ClipMapTerrain::BASE_VERTEX_SPACING;
			constexpr const uint32 morphCells = 4; // 현재 레벨 정점 간격을 기준으로 한 전환 영역의 폭.
			static_assert(morphCells > 0);
			static_assert(morphCells <= SceneManager::ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE - 2);	// 부모의 안쪽 경계가 부모 자신의 모핑 영역에 들어가지 않게 제한한다.

            const double cameraAnchorWorldX = streaming._terrainOriginWorldX + double(streaming._terrainCameraAnchorSampleX) * streaming._terrainSampleWorldSpacing;
            const double cameraAnchorWorldZ = streaming._terrainOriginWorldZ + double(streaming._terrainCameraAnchorSampleZ) * streaming._terrainSampleWorldSpacing;

			// 카메리 위치를 월드 원점 기준의 각 Level Scale 격자에 맞춘 뒤, Anchor 상대 좌표로 돌려주는 함수
			const DKFunction<float2(float)> snapTerrainGrid = [&](float spacing)->float2
				{
					const double x = std::floor((cameraAnchorWorldX + double(cameraLocalXZ.x)) / spacing) * spacing - cameraAnchorWorldX;
					const double z = std::floor((cameraAnchorWorldZ + double(cameraLocalXZ.y)) / spacing) * spacing - cameraAnchorWorldZ;
					return float2(static_cast<float>(x), static_cast<float>(z));
				};

			uint32 bufferIndex = 0;
			const DKFunction<void(uint32, uint32, float2, uint32)> uploadPatch = [&](const uint32 level, const uint32 type, const float2 patchOrigin, const uint32 rotation)
				{
					const float spacing = static_cast<float>(1u << level) * vertexScale;
					const float parentSpacing = spacing * 2.0f;
					const float2 parentOriginRelativeWorldXZ = snapTerrainGrid(parentSpacing);

					// 현재 Clipmap 배치에서 실제 외곽 경계는 카메라로부터 각 축 방향으로 최소 2*T*spacing만큼 떨어져 있다.
					const float morphEnd = 2.0f * static_cast<float>(SceneManager::ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE) * spacing;
					const float morphStart = morphEnd - static_cast<float>(morphCells) * spacing;

					TerrainMeshConstantBuffer cb{};
					cb._baseXY_scale_rotate = float4( patchOrigin.x, patchOrigin.y, spacing, static_cast<float>(rotation));
					cb._morphCenter_start_end = float4( cameraLocalXZ.x, cameraLocalXZ.y, morphStart, morphEnd);
					cb._parentOriginRelativeWorldXZ = parentOriginRelativeWorldXZ;
					cb._type = type;
					cb._level = level;
					cb._hasParent = (level + 1u < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS) ? 1u : 0u;
					DK_ASSERT_LOG(bufferIndex < terrain._terrainConstantBuffer.size(), "Clipmap constant buffer count mismatch.");
					terrain._terrainConstantBuffer[bufferIndex++]->upload(&cb);
				};

			// Draw Cross
			uploadPatch(0u, 0u, snapTerrainGrid(vertexScale), 0u);

			for (uint32 level = 0; level < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS; ++level)
			{
				const float spacing = static_cast<float>(1u << level) * vertexScale;
				const float gridScale = static_cast<float>(SceneManager::ClipMapTerrain::PATCH_CELL_COUNT_PER_SIDE) * spacing;
				const float2 snappedCameraPos = snapTerrainGrid(spacing);
				const float2 base = snappedCameraPos - gridScale * 2.0f;

				// Patch
				for (uint32 x = 0; x < 4; ++x)
				{
					for (uint32 y = 0; y < 4; ++y)
					{
						if (level != 0 && (x == 1 || x == 2) && (y == 1 || y == 2))
							continue;

						const float2 fill(x >= 2 ? spacing : 0.0f, y >= 2 ? spacing : 0.0f);
						const float2 patchOrigin = base + float2(x, y) * gridScale + fill;
						uploadPatch(level, 1u, patchOrigin, 0u);
					}
				}

				// Filler: 현재 코드에서는 Filter라는 이름을 사용한다.
				uploadPatch(level, 2u, snappedCameraPos, 0u);

				const float2 parentOriginRelativeWorldXZ = snapTerrainGrid(spacing * 2.0f);

				// Seam: 부모가 있는 레벨만 생성한다.
				if (level + 1u < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS)
				{
					const float2 seamOrigin = parentOriginRelativeWorldXZ - gridScale * 2.0f;
					uploadPatch(level, 3u, seamOrigin, 0u);
				}

				// Trim
				if ((level + 1) < SceneManager::ClipMapTerrain::NUM_CLIPMAP_LEVELS)
				{
					const float2 d = cameraLocalXZ - parentOriginRelativeWorldXZ;
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

			SceneObjectConstantBuffer sceneObjectConstantBufferData;
			sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
			sceneObject._sceneObjectConstantBuffer->upload(&sceneObjectConstantBufferData);
		}

		// Render SkinnedMesh SceneObject
		for (const SceneObject& sceneObject : DuckingEngine::getInstance().GetSceneObjectManager().getCharacterSceneObjects())
		{
			if (sceneObject._sceneObjectConstantBuffer == nullptr)
				continue;

			SceneObjectConstantBuffer sceneObjectConstantBufferData;
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

				skinnedMeshComponent->_currentAnimationBufferCS->upload(&skinnedMeshComponent->_skeletonConstantBufferData);
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
	void SceneRenderer::renderUI() noexcept
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

#if defined(_DK_DEBUG_)
		static bool gIsReload = false;
		if (gIsReload)
		{
			renderModule.reloadShader();
			_cloudNoiseValid = false;
			gIsReload = false;
		}
#endif
		ImGui_ImplWin32_NewFrame();
		ImGui_ImplDX12_NewFrame();
		ImGui::NewFrame();

		static bool showUI = true;
		static bool showSettings = false;
		static bool resetSettingsLayout = false;
		if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
			showUI = showUI == false;

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
					showSettings = showSettings == false;

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
								const AtmosphereConstantBuffer defaults;
                                atmosphere._densityReferenceRadius = defaults._densityReferenceRadius;
                                atmosphere._densityTransitionRadius = defaults._densityTransitionRadius;
                                DK::memcpy(atmosphere._planetCentre, defaults._planetCentre, sizeof(atmosphere._planetCentre));
                                atmosphere._atmosphereRadius = defaults._atmosphereRadius;
                                atmosphere._rayleighHeight = defaults._rayleighHeight;
                                atmosphere._mieHeight = defaults._mieHeight;
                                atmosphere._scatteringScale = defaults._scatteringScale;
                                atmosphere._rayleighScale = defaults._rayleighScale;
                                atmosphere._mieScale = defaults._mieScale;
                                atmosphere._mieAbsorptionScale = defaults._mieAbsorptionScale;
                                atmosphere._mieAnisotropy = defaults._mieAnisotropy;
                                for (AtmosphereAbsorptionLayer& layer : atmosphere._absorptionLayers)
									layer = AtmosphereAbsorptionLayer();
							}
							ImGui::InputFloat3("Center (XYZ)", atmosphere._planetCentre, "%.3g");
							ImGui::Text("Density reference radius (m): %.1f km", atmosphere._densityReferenceRadius / 1000.0f);
							ImGui::InputFloat("Density transition radius (m)", &atmosphere._densityTransitionRadius, 100.0f, 1000.0f, "%.1f");
							ImGui::InputFloat("Atmosphere radius (m)", &atmosphere._atmosphereRadius, 100.0f, 1000.0f, "%.1f");
#if defined(_DK_DEBUG_)
                            atmosphere.validate();
#endif

                            if (ImGui::Checkbox("Compare extended atmosphere top", &_compareExtendedAtmosphereTop))
                            {
                                if (_compareExtendedAtmosphereTop)
                                {
                                    float top = atmosphere._densityReferenceRadius + 12.0f * Math::max(atmosphere._rayleighHeight, atmosphere._mieHeight);
									for (const AtmosphereAbsorptionLayer& layer : atmosphere._absorptionLayers)
									{
										if (layer._strength > 0.0f)
											top = Math::max(top, layer._peakRadius + layer._upperWidth);
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
#if defined(_DK_DEBUG_)
                            atmosphere.validate();
#endif
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
#if defined(_DK_DEBUG_)
                            atmosphere.validate();
#endif
						}
                        if (ImGui::CollapsingHeader("Volumetric clouds"))
                        {
                            InspectorProperty("Coverage", [&] { ImGui::SliderFloat("##value", &_cloudConstantBufferData._cloudCoverage, 0.0f, 1.0f); });
                            InspectorProperty("Wind XZ (m/s)", [&] { ImGui::DragFloat2("##value", _cloudConstantBufferData._cloudWindSpeedXZ, 0.1f, 0.0f, 0.0f, "%.1f"); });
                            const float3 cloudBase(atmosphere._planetCentre[0], atmosphere._planetCentre[1] + atmosphere._densityReferenceRadius + _cloudConstantBufferData._cloudBaseHeight, atmosphere._planetCentre[2]);
                            const float cloudThickness = _cloudConstantBufferData._cloudTopHeight - _cloudConstantBufferData._cloudBaseHeight;
                            if (ImGui::Button("Cloud layer"))
                                Camera::gMainCamera->setView(cloudBase + float3(0.0f, cloudThickness * 0.5f, 0.0f), float3(0.0f, 0.0f, 1.0f));
                            ImGui::SameLine();
                            if (ImGui::Button("Above clouds"))
                                Camera::gMainCamera->setView(cloudBase + float3(0.0f, cloudThickness * 1.5f, 0.0f), float3(0.0f, -0.5f, 1.0f));
                        }

                        if (ImGui::CollapsingHeader("Surface to atmosphere"))
                        {
                            InspectorProperty("Sun shadow strength", [&] { ImGui::SliderFloat("##value", &atmosphere._surfaceShadowStrength, 0.0f, 1.0f); });
                            InspectorProperty("Reflected light strength", [&] { ImGui::SliderFloat("##value", &atmosphere._surfaceBounceStrength, 0.0f, 2.0f); });
                            InspectorProperty("Capture half-width (m)", [&] { ImGui::InputFloat("##value", &_surfaceCaptureExtent); });
                            InspectorProperty("Capture depth (m)", [&] { ImGui::InputFloat("##value", &_surfaceCaptureDepth); });
                            InspectorProperty("Shadow bias (m)", [&] { ImGui::InputFloat("##value", &atmosphere._surfaceBias); });
                            InspectorProperty("Bounce samples", [&] { ImGui::SliderInt("##value", reinterpret_cast<int*>(&atmosphere._surfaceBounceSamples), 8, static_cast<int>(SURFACE_BOUNCE_MAX_SAMPLES)); });
#if defined(_DK_DEBUG_)
                            validateSurfaceCaptureParameters();
                            atmosphere.validate();
#endif
                            ImGui::TextWrapped("Four 512x512 shadow cascades (1/8, 1/4, 1/2, full half-width), with blended boundaries. The outer capture feeds the 16x16x16 bounce field.");
                        }
                        if (ImGui::CollapsingHeader("Gas absorption layers"))
                        {
                            ImGui::TextWrapped("Independent RGB absorption shells. Radii are measured from the planet centre; zero amount disables a layer.");
                            if (ImGui::Button("Earth ozone example (layer 0)"))
                            {
                                AtmosphereAbsorptionLayer& layer = atmosphere._absorptionLayers[0];
                                layer._extinction[0] = 0.650e-6f;
								layer._extinction[1] = 1.881e-6f;
								layer._extinction[2] = 0.085e-6f;
                                layer._strength = 1.0f;
								layer._peakRadius = atmosphere._densityReferenceRadius + 25000.0f;
                                layer._lowerWidth = layer._upperWidth = 15000.0f;
                            }
                            for (uint32 i = 0; i < DK_COUNT_OF(atmosphere._absorptionLayers); ++i)
                            {
                                ImGui::PushID(static_cast<int>(i));
                                ImGui::Text("Layer %u", i);
                                AtmosphereAbsorptionLayer& layer = atmosphere._absorptionLayers[i];
                                InspectorProperty("Amount", [&] { ImGui::DragFloat("##value", &layer._strength, 0.01f, 0.0f, 10.0f); });
                                float inverseKm[3] = { layer._extinction[0] * 1000, layer._extinction[1] * 1000, layer._extinction[2] * 1000 };
                                InspectorProperty("Absorption RGB (1/km)", [&] { ImGui::InputFloat3("##value", inverseKm, "%.6f"); });
                                InspectorProperty("Peak radius (m)", [&] { ImGui::InputFloat("##value", &layer._peakRadius, 100, 1000, "%.1f"); });
                                InspectorProperty("Lower width (m)", [&] { ImGui::InputFloat("##value", &layer._lowerWidth); });
                                InspectorProperty("Upper width (m)", [&] { ImGui::InputFloat("##value", &layer._upperWidth); });
								for (uint32 c = 0; c < 3u; ++c)
									layer._extinction[c] = inverseKm[c] / 1000.0f;
#if defined(_DK_DEBUG_)
                                layer.validate(i);
#endif
                                ImGui::PopID();
                            }
                        }
						if (ImGui::CollapsingHeader("Sampling quality"))
						{
							const char* samplingModes[] = { "Production", "Direct reference", "Reference limits", "Production work", "Production transmittance", "Development preview" };
                            static_assert(DK_COUNT_OF(samplingModes) == ATMOSPHERE_PREVIEW + 1u);
							InspectorProperty("Atmosphere integration", [&] { ImGui::Combo("##value", reinterpret_cast<int*>(&atmosphere._atmosphereSamplingMode), samplingModes, DK_COUNT_OF(samplingModes)); });
                            if (atmosphere._atmosphereSamplingMode == ATMOSPHERE_PREVIEW)
                            {
                                InspectorProperty("Preview view intervals", [&] {
                                    int intervalCount = static_cast<int>(atmosphere._previewViewIntervalCount);
                                    if (ImGui::SliderInt("##value", &intervalCount, ATMOSPHERE_PREVIEW_MIN_VIEW_INTERVALS, ATMOSPHERE_PREVIEW_MAX_VIEW_INTERVALS, "%d", ImGuiSliderFlags_AlwaysClamp))
                                        atmosphere._previewViewIntervalCount = static_cast<uint32>(intervalCount);
                                });
                                InspectorProperty("Atmosphere shadows", [&] {
                                    bool enabled = atmosphere._previewAtmosphereShadows != 0u;
                                    if (ImGui::Checkbox("##value", &enabled))
                                        atmosphere._previewAtmosphereShadows = enabled ? 1u : 0u;
                                });
                                InspectorProperty("Surface bounce into atmosphere", [&] {
                                    bool enabled = atmosphere._previewSurfaceBounce != 0u;
                                    if (ImGui::Checkbox("##value", &enabled))
                                        atmosphere._previewSurfaceBounce = enabled ? 1u : 0u;
                                });
                                ImGui::TextWrapped("Preview omits volumetric clouds. Surface shadows still use the scene shadow settings.");
                            }
                            if (atmosphere._atmosphereSamplingMode == ATMOSPHERE_REFERENCE || atmosphere._atmosphereSamplingMode == ATMOSPHERE_LIMIT_DEBUG)
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
						ImGui::TextDisabled("F1: show / hide all UI");
						ImGui::EndTabItem();
					}
#if defined(_DK_DEBUG_)
					if (ImGui::BeginTabItem("GPU"))
					{
						ImGui::Text("Static placements: %u draws, %u mesh instances", _staticMeshDrawCount, _staticMeshInstanceCount);
						ImGui::Text("Visible chunks: %u / %u, triangles: %llu", _staticMeshVisibleChunkCount, _staticMeshTotalChunkCount, _staticMeshTriangleCount);
						if (ImGui::Button("Reset peaks"))
							renderModule.resetGpuTimingPeaks();
						if (ImGui::BeginTable("GPU timings", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
						{
							ImGui::TableSetupColumn("Scope");
							ImGui::TableSetupColumn("GPU (ms)");
							ImGui::TableSetupColumn("Peak (ms)");
							ImGui::TableSetupScrollFreeze(0, 1);
							ImGui::TableHeadersRow();
							const DKVector<RenderModule::GpuTiming>& timings = renderModule.getGpuTimings();
							const auto drawTiming = [&](const auto& drawChildTiming, uint32 timingIndex) -> void
							{
								const RenderModule::GpuTiming& timing = timings[timingIndex];
								const bool hasChildren = timing._childIndices.empty() == false;
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::PushID(static_cast<int>(timingIndex));
								const ImGuiTreeNodeFlags flags = hasChildren ? ImGuiTreeNodeFlags_SpanAvailWidth : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
								const bool expanded = ImGui::TreeNodeEx(timing._name.c_str(), flags);
								ImGui::SetItemTooltip("Depth: %u\nGPU time includes child scopes", timing._depth);
								ImGui::TableSetColumnIndex(1);
								if (timing._executionCount > 0)
								{
									ImGui::Text("%.3f", timing._milliseconds);
									ImGui::SetItemTooltip("Scope executions: %u", timing._executionCount);
								}
								else
								{
									ImGui::TextDisabled(renderModule.hasGpuTimings() ? "Skipped" : "Pending");
								}
								ImGui::TableSetColumnIndex(2);
								ImGui::Text("%.3f", timing._peakMilliseconds);
								if (hasChildren && expanded)
								{
									for (uint32 childIndex : timing._childIndices)
										drawChildTiming(drawChildTiming, childIndex);
									ImGui::TreePop();
								}
								ImGui::PopID();
							};
							for (uint32 timingIndex = 0; timingIndex < timings.size(); ++timingIndex)
							{
								if (timings[timingIndex]._depth == 0)
									drawTiming(drawTiming, timingIndex);
							}
							ImGui::TableNextRow();
							ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableHeaderBg));
							ImGui::TableSetColumnIndex(0);
							ImGui::TextUnformatted("Total");
							ImGui::SetItemTooltip("Sum of top-level scope times for the frame");
							ImGui::TableSetColumnIndex(1);
							if (renderModule.hasGpuTimings())
								ImGui::Text("%.3f", renderModule.getGpuTotalMilliseconds());
							else
								ImGui::TextDisabled("Pending");
							ImGui::TableSetColumnIndex(2);
							ImGui::Text("%.3f", renderModule.getGpuTotalPeakMilliseconds());
							ImGui::EndTable();
						}
						ImGui::EndTabItem();
					}
#endif
					ImGui::EndTabBar();
				}
#if defined(_DK_DEBUG_)
                atmosphere.validate();
#endif
				atmosphere._multipleScatteringMaxOrder = Math::clamp(atmosphere._multipleScatteringMaxOrder, 1u, 8u);
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
	}
#endif

#if defined(_DK_DEBUG_)
    void SceneRenderer::validateSurfaceCaptureParameters() const
    {
        DK_ASSERT_LOG(std::isfinite(_surfaceCaptureExtent) && _surfaceCaptureExtent > 0.0f, "Invalid _surfaceCaptureExtent=%g", _surfaceCaptureExtent);
        DK_ASSERT_LOG(std::isfinite(_surfaceCaptureDepth) && _surfaceCaptureDepth > 0.0f, "Invalid _surfaceCaptureDepth=%g", _surfaceCaptureDepth);
        DK_ASSERT_LOG(2.0f * _surfaceCaptureExtent / SURFACE_BOUNCE_GRID >= SURFACE_BOUNCE_MIN_CELL_WORLD_SIZE, "Surface bounce cell is too small: _surfaceCaptureExtent=%g, grid=%u, minimum cell size=%g", _surfaceCaptureExtent, SURFACE_BOUNCE_GRID, SURFACE_BOUNCE_MIN_CELL_WORLD_SIZE);
        DK_ASSERT_LOG(std::isfinite(_atmosphereConstantBufferData._surfaceBias) && _atmosphereConstantBufferData._surfaceBias >= 0.0f, "Invalid _surfaceBias=%g", _atmosphereConstantBufferData._surfaceBias);
    }
#endif

    void SceneRenderer::prepareSurfaceCapture(const float3& cameraPosition)
    {
		AtmosphereConstantBuffer& atmosphereData = _atmosphereConstantBufferData;

        DK_ASSERT_LOG(atmosphereData._surfaceMapSize > 0u && atmosphereData._surfaceMapSize % (2u * SURFACE_BOUNCE_GRID) == 0u, "Surface capture size must be an even multiple of the bounce grid");
        DK_ASSERT_LOG(atmosphereData._surfaceMapSize == _surfaceCapturePass->_binding._target->_targets._width, "Surface capture size does not match the texture");
#if defined(_DK_DEBUG_)
        validateSurfaceCaptureParameters();
#endif
        DK_ASSERT_LOG(atmosphereData._shadowCascadeCount <= DK_ATMOSPHERE_SHADOW_CASCADE_COUNT, "Invalid shadow cascade count");

        atmosphereData._surfaceMapEnabled = _renderLocalScene && (atmosphereData._surfaceShadowStrength > 0.0f || atmosphereUsesSurfaceBounce(atmosphereData)) ? 1u : 0u;

        const float depth = _surfaceCaptureDepth;
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

        double (*const doublePrecisionDot)(const float3&, const float3&) = [](const float3& a, const float3& b)
        {
            return double(a.x) * b.x + double(a.y) * b.y + double(a.z) * b.z;
        };
        for (uint32 cascadeIndex = 0; cascadeIndex < atmosphereData._shadowCascadeCount; ++cascadeIndex)
        {
            // Nested light-space coverage also serves off-screen atmospheric samples.
            const float extent = _surfaceCaptureExtent / float(1u << (DK_ATMOSPHERE_SHADOW_CASCADE_COUNT - 1u - cascadeIndex));
            const RenderPassInstance* pass = cascadeIndex == DK_ATMOSPHERE_SHADOW_CASCADE_COUNT - 1u ? _surfaceCapturePass : _nearShadowPasses[cascadeIndex];
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

			IBufferRef& surfaceSceneConstantBuffer = (cascadeIndex == DK_ATMOSPHERE_SHADOW_CASCADE_COUNT - 1u ? _surfaceSceneConstantBuffer : _nearShadowSceneConstants[cascadeIndex]);
			_surfaceSceneConstantsData[cascadeIndex] = scene;
			surfaceSceneConstantBuffer->upload(&scene);

			SurfaceShadowCascade& cascade = atmosphereData._shadowCascades[cascadeIndex];
            DK::memcpy(cascade._right, atmosphereData._surfaceRight, sizeof(cascade._right));
            DK::memcpy(cascade._up, atmosphereData._surfaceUp, sizeof(cascade._up));
            DK::memcpy(cascade._forward, atmosphereData._surfaceForward, sizeof(cascade._forward));
            DK::memcpy(cascade._origin, atmosphereData._surfaceOrigin, sizeof(cascade._origin));

            cascade._albedoIndex = map._colors[0]._texture->getSRV();
			cascade._depthIndex = map._depth._texture->getSRV();
			cascade._mapSize = map._width;
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
		if (surface._surfaceShadowStrength > 0.0f)
		{
			for (uint32 i = 0; i < DK_COUNT_OF(_nearShadowPasses); ++i)
			{
				startRenderPass(renderModule, *_nearShadowPasses[i]);
				drawSceneGeometry(renderModule, _nearShadowSceneConstants[i], _surfaceSceneConstantsData[i], false);
				endRenderPass();
			}
		}

		startRenderPass(renderModule, *_surfaceCapturePass);
		drawSceneGeometry(renderModule, _surfaceSceneConstantBuffer, _surfaceSceneConstantsData[DK_ATMOSPHERE_SHADOW_CASCADE_COUNT - 1u], false);
		endRenderPass();

        if (updateBounce && atmosphereUsesSurfaceBounce(surface))
        {
            startRenderPass(renderModule, *_surfaceBouncePass);
			{
				startPipeline("AtmosphereSurfaceBouncePipeline");
				{
					renderModule.resourceBarrierTransition(_surfaceBounce, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
					setRootConstantParameter("_surfaceBounceOutputUav", _surfaceBounce->getUAV());
					renderModule.dispatch(SURFACE_BOUNCE_GRID * SURFACE_BOUNCE_GRID, SURFACE_BOUNCE_GRID, 1u);
					renderModule.resourceBarrierTransition(_surfaceBounce, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				}
				endPipeline();
			}
			endRenderPass();
        }

		if (updateBounce == false)
			return;

        _cachedSurfaceParameters = surface;
        _lastSurfaceTransportUpdate = now;
        _surfaceTransportValid = true;
        _skyLightingValid = false; // Only invalidate when capture/bounce data were refreshed.
    }
    void SceneRenderer::drawSceneGeometry(RenderModule& renderModule, const IBufferRef& sceneConstants, const SceneConstantBuffer& sceneConstantsData, bool includeDebug)
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
				setConstantBuffer(material->get_materialNameHash(), material->get_parameterBufferForGPUWritable());
				setConstantBuffer("TerrainStreamingConstants", terrain._streamingBuffer);

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
					// Draw Patch
					for (uint32 x = 0; x < 4; ++x)
					{
						for (uint32 y = 0; y < 4; ++y)
						{
							if (i != 0 && (x == 1 || x == 2) && (y == 1 || y == 2))
								continue;

							IBufferRef& terrainConstantBuffer = terrain._terrainConstantBuffer[terrainConstantBufferTypeIndex++];
							setConstantBuffer("TerrainMeshConstantBuffer", terrainConstantBuffer);

							renderModule.setVertexBuffers(0, 1, terrain._patch._vertexBufferView.get());
							renderModule.setIndexBuffer(terrain._patch._indexBufferView.get());
							renderModule.drawIndexedInstanced(static_cast<UINT>(terrain._patch._indexCount), 1, 0, 0, 0);
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

			startPipeline("StaticMeshInstancedPipeline");
			{
				setConstantBuffer("SceneConstantBuffer", sceneConstants);
				const float4x4 viewProjection = sceneConstantsData._cameraWorldMatrixInv * sceneConstantsData._cameraProjectionMatrix;
				const DKArray<float4, 6> frustumPlanes = {
					float4(viewProjection._14 + viewProjection._11, viewProjection._24 + viewProjection._21, viewProjection._34 + viewProjection._31, viewProjection._44 + viewProjection._41),
					float4(viewProjection._14 - viewProjection._11, viewProjection._24 - viewProjection._21, viewProjection._34 - viewProjection._31, viewProjection._44 - viewProjection._41),
					float4(viewProjection._14 + viewProjection._12, viewProjection._24 + viewProjection._22, viewProjection._34 + viewProjection._32, viewProjection._44 + viewProjection._42),
					float4(viewProjection._14 - viewProjection._12, viewProjection._24 - viewProjection._22, viewProjection._34 - viewProjection._32, viewProjection._44 - viewProjection._42),
					float4(viewProjection._13, viewProjection._23, viewProjection._33, viewProjection._43),
					float4(viewProjection._14 - viewProjection._13, viewProjection._24 - viewProjection._23, viewProjection._34 - viewProjection._33, viewProjection._44 - viewProjection._43)
				};
				if (includeDebug)
				{
					_staticMeshDrawCount = 0;
					_staticMeshInstanceCount = 0;
					_staticMeshVisibleChunkCount = 0;
					_staticMeshTotalChunkCount = 0;
					_staticMeshTriangleCount = 0;
				}
				struct InstanceDrawRange
				{
					uint32 _firstInstance;
					uint32 _instanceCount;
				};
				DKVector<InstanceDrawRange> visibleRanges;
				for (const StaticMeshInstanceBatch& batch : DuckingEngine::getInstance().GetSceneObjectManager().getStaticMeshInstanceBatches())
				{
					visibleRanges.clear();
					for (const StaticMeshInstanceChunk& chunk : batch._chunks)
					{
						if (includeDebug)
							++_staticMeshTotalChunkCount;
						if (intersectsGeometryFrustum(chunk, frustumPlanes) == false)
							continue;
						if (visibleRanges.empty() == false && visibleRanges.back()._firstInstance + visibleRanges.back()._instanceCount == chunk._firstInstance)
							visibleRanges.back()._instanceCount += chunk._instanceCount;
						else
							visibleRanges.push_back({ chunk._firstInstance, chunk._instanceCount });
						if (includeDebug)
						{
							++_staticMeshVisibleChunkCount;
							_staticMeshInstanceCount += chunk._instanceCount;
						}
					}
					if (visibleRanges.empty())
						continue;

					setShaderResourceView("_staticMeshInstances", batch._staticMeshInstances);
					for (const StaticMeshModel::SubMeshType& subMesh : batch._model->get_subMeshArr())
					{
						const Material* material = subMesh._material.get();
						setConstantBuffer(material->get_materialNameHash(), material->get_parameterBufferForGPU());
						renderModule.setVertexBuffers(0, 1, subMesh._vertexBufferView.get());
						renderModule.setIndexBuffer(subMesh._indexBufferView.get());
						for (const InstanceDrawRange& range : visibleRanges)
						{
							setRootConstantParameter("_firstInstance", range._firstInstance);
							renderModule.drawIndexedInstanced(static_cast<uint32>(subMesh._indices.size()), range._instanceCount, 0, 0, 0);
							if (includeDebug)
							{
								++_staticMeshDrawCount;
								_staticMeshTriangleCount += uint64(subMesh._indices.size() / 3) * range._instanceCount;
							}
						}
					}
				}
			}
			endPipeline();

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
							setConstantBuffer(material->get_materialNameHash(), material->get_parameterBufferForGPU());

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
							setConstantBuffer(material->get_materialNameHash(), material->get_parameterBufferForGPU());

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
			const DKVector<SpherePrimitiveInfo>& spherePrimitiveInfo = debugDrawManager.get_primitiveInfoSphereArr();
			const uint32 sphereInstanceCount = static_cast<uint32>(spherePrimitiveInfo.size());
			if (sphereInstanceCount)
			{
				startPipeline("SpherePipeline");
				{
					setConstantBuffer("SceneConstantBuffer", sceneConstants);
					setShaderResourceView("SpherePrimitiveInfoBuffer", debugDrawManager.get_primitiveInfoSphereBuffer());
					renderModule.setVertexBuffers(0, 1, SpherePrimitiveInfo::kVertexBufferView.get());
					renderModule.setIndexBuffer(SpherePrimitiveInfo::kIndexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(SpherePrimitiveInfo::indexCount), sphereInstanceCount, 0, 0, 0);
				}
				endPipeline();
			}
			const DKVector<LinePrimitiveInfo>& linePrimitiveInfo = debugDrawManager.get_primitiveInfoLineArr();
			const uint32 lineInstanceCount = static_cast<uint32>(linePrimitiveInfo.size());
			if (lineInstanceCount)
			{
				startPipeline("LinePipeline");
				{
					setConstantBuffer("SceneConstantBuffer", sceneConstants);
					setShaderResourceView("LinePrimitiveInfoBuffer", debugDrawManager.get_primitiveInfoLineBuffer());
					renderModule.setVertexBuffers(0, 1, LinePrimitiveInfo::kVertexBufferView.get());
					renderModule.setIndexBuffer(LinePrimitiveInfo::kIndexBufferView.get());
					renderModule.drawIndexedInstanced(static_cast<UINT>(LinePrimitiveInfo::indexCount), lineInstanceCount, 0, 0, 0);
				}
				endPipeline();
			}
			debugDrawManager.endUpdateRender();
            }
#endif
		}
    }

    void SceneRenderer::drawGrass(RenderModule& renderModule)
    {
        SceneManager& sceneManager = DuckingEngine::getInstance().getSceneManagerWritable();
        const SceneManager::Grass& grass = sceneManager.getGrass();
        if (grass._indexBuffer == nullptr)
            return;

        const float3 camera = Camera::gMainCamera->get_worldTransform().get_translation();
        const SceneManager::ClipMapTerrain& terrain = sceneManager.getClipMapTerrainWritable();
        const TerrainStreamingConstants& streaming = terrain._streamingConstants;
        const float mapMinX = Math::max(grass._grassMapOriginX, streaming._terrainOriginWorldX + streaming._terrainMinimumSampleX * streaming._terrainSampleWorldSpacing);
        const float mapMinZ = Math::max(grass._grassMapOriginZ, streaming._terrainOriginWorldZ + streaming._terrainMinimumSampleZ * streaming._terrainSampleWorldSpacing);
        const float mapMaxX = Math::min(grass._grassMapOriginX + grass._grassMapWidth, streaming._terrainOriginWorldX + (streaming._terrainMinimumSampleX + int32(streaming._terrainCellCountX)) * streaming._terrainSampleWorldSpacing);
        const float mapMaxZ = Math::min(grass._grassMapOriginZ + grass._grassMapDepth, streaming._terrainOriginWorldZ + (streaming._terrainMinimumSampleZ + int32(streaming._terrainCellCountZ)) * streaming._terrainSampleWorldSpacing);
        constexpr float tileMeters = GRASS_TILE_METERS;
        constexpr float maxDistance = GRASS_LOD5_END;
        const float maxGrassY = streaming._terrainBaseWorldHeight + streaming._terrainWorldHeightRange + 1.2f;
        int (*const tileAt)(float) = [](float position) { return static_cast<int>(Math::floor(position / GRASS_TILE_METERS)); };
        const float minX = Math::max(mapMinX, camera.x - maxDistance);
        const float minZ = Math::max(mapMinZ, camera.z - maxDistance);
        const float maxX = Math::min(mapMaxX, camera.x + maxDistance);
        const float maxZ = Math::min(mapMaxZ, camera.z + maxDistance);
        if (minX >= maxX || minZ >= maxZ)
            return;

        DirectX::XMMATRIX (*const toDirectXMatrix)(const float4x4&) = [](const float4x4& matrix)
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

        constexpr float lodEnds[] = { GRASS_LOD0_END, GRASS_LOD1_END, GRASS_LOD2_END, GRASS_LOD3_END, GRASS_LOD4_END, GRASS_LOD5_END };
        const int firstTileX = tileAt(minX);
        const int lastTileX = tileAt(maxX);
        const int firstTileZ = tileAt(minZ);
        const int lastTileZ = tileAt(maxZ);
        constexpr float bladeOverhang = 1.2f;
        const double grassCellMeters = double(GRASS_TILE_METERS) / grass._tileGridSide;
        const double cameraAnchorWorldX = streaming._terrainOriginWorldX + double(streaming._terrainCameraAnchorSampleX) * streaming._terrainSampleWorldSpacing;
        const double cameraAnchorWorldZ = streaming._terrainOriginWorldZ + double(streaming._terrainCameraAnchorSampleZ) * streaming._terrainSampleWorldSpacing;
        const int32 grassAnchorX = static_cast<int32>(std::floor(cameraAnchorWorldX / grassCellMeters));
        const int32 grassAnchorZ = static_cast<int32>(std::floor(cameraAnchorWorldZ / grassCellMeters));
        const float grassOffsetX = static_cast<float>(cameraAnchorWorldX - double(grassAnchorX) * grassCellMeters);
        const float grassOffsetZ = static_cast<float>(cameraAnchorWorldZ - double(grassAnchorZ) * grassCellMeters);
        RenderModule& currentRenderModule = renderModule;
        startPipeline("GrassPatchPipeline");
        setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
        setConstantBuffer("TerrainStreamingConstants", terrain._streamingBuffer);
        setRootConstantParameter("_grassPlacementTexture", grass._placementTexture->getSRV());
        setRootConstantParameter("_grassAnchorX", grassAnchorX);
        setRootConstantParameter("_grassAnchorZ", grassAnchorZ);
        setRootConstantParameter("_grassOffsetX", grassOffsetX);
        setRootConstantParameter("_grassOffsetZ", grassOffsetZ);
        setRootConstantParameter("_grassTileGridSide", grass._tileGridSide);
        setRootConstantParameter("_grassMapOriginX", grass._grassMapOriginX);
        setRootConstantParameter("_grassMapOriginZ", grass._grassMapOriginZ);
        setRootConstantParameter("_grassMapWidth", grass._grassMapWidth);
        setRootConstantParameter("_grassMapDepth", grass._grassMapDepth);
        renderModule.setIndexBuffer(grass._indexBufferView.get());
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
                if (worldFrustum.Intersects(bounds) == false)
                    continue;

                uint32 lod = 0;
                while (distanceSquared >= lodEnds[lod] * lodEnds[lod])
                    ++lod;
                const uint32 stride = 1u << lod;
                const uint32 gridSide = grass._tileGridSide / stride;
                setRootConstantParameter("_grassTileX", tileX);
                setRootConstantParameter("_grassTileZ", tileZ);
                setRootConstantParameter("_grassCellStride", stride);
                renderModule.drawIndexedInstanced(SceneManager::Grass::GRASS_INDEX_COUNT, gridSide * gridSide, 0, 0, 0);
            }
        }
        endPipeline();
    }

	void SceneRenderer::updateCloudNoise(RenderModule& renderModule)
	{
		if (_cloudNoiseValid)
			return;

		startRenderPass(renderModule, *_cloudNoisePass);
		startPipeline("CloudVolumeNoisePipeline");
		renderModule.resourceBarrierTransition(_cloudNoise, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		setRootConstantParameter("_cloudNoiseUav", _cloudNoise->getUAV());
		renderModule.dispatch(DK_CLOUD_NOISE_SIZE, DK_CLOUD_NOISE_SIZE, DK_CLOUD_NOISE_SIZE);
		renderModule.resourceBarrierTransition(_cloudNoise, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		endPipeline();

		startPipeline("CloudWeatherPipeline");
		renderModule.resourceBarrierTransition(_cloudWeather, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		setRootConstantParameter("_cloudWeatherUav", _cloudWeather->getUAV());
		renderModule.dispatch(DK_CLOUD_WEATHER_SIZE, DK_CLOUD_WEATHER_SIZE, 1);
		renderModule.resourceBarrierTransition(_cloudWeather, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		endPipeline();
		endRenderPass();

		_cloudNoiseValid = true;
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
				rm.dispatch(ATMOSPHERE_MULTIPLESCATTERING_WIDTH, ATMOSPHERE_MULTIPLESCATTERING_HEIGHT, 1);
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
        bool dirty = _skyLightingValid == false || p._multipleScatteringStrength != old._multipleScatteringStrength
            || p._multipleScatteringMaxOrder != old._multipleScatteringMaxOrder || p._surfaceMapEnabled != old._surfaceMapEnabled
            || p._surfaceShadowStrength != old._surfaceShadowStrength || p._surfaceBounceStrength != old._surfaceBounceStrength
            || p._atmosphereSamplingMode != old._atmosphereSamplingMode || p._previewAtmosphereShadows != old._previewAtmosphereShadows || p._previewSurfaceBounce != old._previewSurfaceBounce
            || p._sunDegree != old._sunDegree
            || p._sunIrradiance != old._sunIrradiance || p._mieAnisotropy != old._mieAnisotropy;
        for (uint32 i = 0; i < 3; ++i)
            dirty = dirty || p._sunColor[i] != old._sunColor[i] || p._planetCentre[i] != old._planetCentre[i];
        dirty = dirty || position.x != _cachedSkyPosition[0] || position.y != _cachedSkyPosition[1] || position.z != _cachedSkyPosition[2];
        if (dirty == false)
			return;

        RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
        startRenderPass(rm, *_skyLightingPass);
		{
			startPipeline("AtmosphereSkyRadiancePipeline");
			{
				rm.resourceBarrierTransition(_skyRadiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
				setRootConstantParameter("_skyOutputUav", _skyRadiance->getUAV());
				rm.dispatch(SKY_WIDTH, SKY_HEIGHT, 1);
				rm.resourceBarrierTransition(_skyRadiance, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			}
			endPipeline();
			startPipeline("AtmosphereSkyFilterPipeline");
			{
				rm.resourceBarrierTransition(_skyFiltered, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
				setRootConstantParameter("_skyOutputUav", _skyFiltered->getUAV());
				rm.dispatch(SKY_WIDTH, SKY_HEIGHT * SKY_BANDS, 1);
				rm.resourceBarrierTransition(_skyFiltered, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
			endPipeline();
		}
        endRenderPass();

        _cachedSkyParameters = p;
        _cachedSkyPosition[0] = position.x; _cachedSkyPosition[1] = position.y; _cachedSkyPosition[2] = position.z;
        _skyLightingValid = true;
    }

	void SceneRenderer::updateRender() noexcept
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		renderModule.preRender();

		updateCloudNoise(renderModule);
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
		drawSceneGeometry(renderModule, _sceneConstantBuffer, _sceneConstantBufferData, true);
		if (_renderLocalScene)
			drawGrass(renderModule);
		renderModule.finishRenderPass();

		// Resolve material attributes to HDR radiance before atmosphere and bloom.
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

        startRenderPass(renderModule, *_atmospherePass);
		{
            const bool preview = _atmosphereConstantBufferData._atmosphereSamplingMode == ATMOSPHERE_PREVIEW;
			startPipelinePermutation("AtmospherePipeline", preview ? 1u : 0u);
			{
				setRootConstantParameter("_materialIndex", gbuffer._colors[2]._texture->getSRV());
				renderModule.resourceBarrierTransition(lighting->getTextureBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				setRootConstantParameter("_lightingIndex", lighting->getSRV());
				setRootConstantParameter("_sceneDepthIndex", gbuffer._depth._texture->getSRV());
				setConstantBuffer("SceneConstantBuffer", _sceneConstantBuffer);
				setConstantBuffer("AtmosphereConstantBuffer", _atmosphereConstantBuffer);
                if (preview == false)
                    setConstantBuffer("CloudConstantBuffer", _cloudConstantBuffer);

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

		// Resolve distant sky clouds, then apply exposure and tone mapping to the selected HDR source.
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
