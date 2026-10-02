#include "stdafx.h"
#include "RaytracingRenderer.h"
#include "DuckingEngine.h"
#include "SceneObjectManager.h"
#include "SceneObject.h"
#include "StaticMeshComponent.h"
#include "Model.h"
#include "Material.h"

namespace DK
{
    const bool RaytracingRenderer::initialize(RenderModule* renderModule, uint32 width, uint32 height)
    {
        _supported = renderModule->supportsRaytracing();
        if (!_supported)
            DK_LOG("DXR unavailable; using raster rendering.");
        return true;
    }

    bool RaytracingRenderer::updateRaytracingRenderer(RenderModule& renderModule)
    {
        if (!_supported || RenderModule::kWidth == 0 || RenderModule::kHeight == 0)
            return false;

        DKVector<RaytracingGeometry> geometries;
        DKVector<ITextureRef> textures;
        for (auto& entry : DuckingEngine::getInstance().GetSceneObjectManagerWritable().getSceneObjectsWritable())
        {
            SceneObject& object = entry.second;
            for (auto& component : object._components)
            {
                auto* mesh = dynamic_cast<StaticMeshComponent*>(component.get());
                if (mesh == nullptr || mesh->get_modelWritable() == nullptr)
                    continue;
                for (auto& submesh : mesh->get_modelWritable()->get_subMeshArrWritable())
                {
                    Material* material = submesh._material.get();
                    if (submesh._indices.empty() || submesh._vertices.empty() || material == nullptr)
                        continue;
                    // The current hit shader uses the StaticMeshStandard material layout.
                    if (material->get_materialName() != "StaticMeshStandard" || material->_parameterBufferForCPU.size() != 8)
                        continue;
                    if (geometries.size() == kRaytracingDescriptorCount)
                        return false;
                    RaytracingGeometry geometry;
                    geometry._vertices = submesh._vertexBuffer;
                    geometry._indices = submesh._indexBuffer;
                    geometry._material = material->get_parameterBufferForGPUWritable();
                    geometry._vertexCount = static_cast<uint32>(submesh._vertices.size());
                    geometry._vertexStride = sizeof(StaticMeshVertex);
                    geometry._indexCount = static_cast<uint32>(submesh._indices.size());
                    geometry._materialStride = static_cast<uint32>(material->_parameterBufferForCPU.size());
                    object.get_worldTransform().tofloat4x4(geometry._world);
                    geometries.push_back(DK::move(geometry));
                    for (auto& parameter : material->_parameterArr)
                    {
                        if (parameter->getType() == MaterialParameter::Type::TEXTURE)
                        {
                            auto texture = static_cast<MaterialParameterTexture*>(parameter.get())->_value;
                            if (texture != nullptr)
                                textures.push_back(DK::move(texture));
                        }
                    }
                }
            }
        }
        if (geometries.empty())
            return false;

        bool changed = geometries.size() != _geometries.size();
        for (size_t i = 0; !changed && i < geometries.size(); ++i)
        {
            const auto& a = geometries[i];
            const auto& b = _geometries[i];
            changed = a._vertices != b._vertices || a._indices != b._indices || a._material != b._material ||
                a._vertexCount != b._vertexCount || a._indexCount != b._indexCount ||
                a._vertexStride != b._vertexStride || a._materialStride != b._materialStride ||
                memcmp(&a._world, &b._world, sizeof(float4x4)) != 0;
        }
        bool texturesChanged = textures != _materialTextures;
        bool resized = _width != RenderModule::kWidth || _height != RenderModule::kHeight;
        if (changed || texturesChanged || resized)
            renderModule.waitAllGPU();

        const bool recreateTextures =
            resized ||
            _outputTexture == nullptr ||
            _outputAccumulateTexture == nullptr;

        if (changed || texturesChanged || recreateTextures)
            renderModule.waitAllGPU();

        if (recreateTextures)
        {
            _width = RenderModule::kWidth;
            _height = RenderModule::kHeight;

            _outputTexture.reset();
            _outputAccumulateTexture.reset();

            _outputTexture = renderModule.createTexture(
                "Raytracing Output",
                _width, _height,
                static_cast<const byte*>(nullptr),
                DXGI_FORMAT_R8G8B8A8_UNORM,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                false, true);

            if (_outputTexture == nullptr)
                return false;

            // 초기 데이터는 필요 없습니다.
            // 첫 DispatchRays에서 모든 픽셀을 덮어씁니다.
            _outputAccumulateTexture = renderModule.createTexture(
                "Raytracing Accumulation",
                _width, _height,
                static_cast<const byte*>(nullptr),
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                false, true);

            if (_outputAccumulateTexture == nullptr)
                return false;
        }

        if (changed || texturesChanged || recreateTextures)
            resetAccumulation();

        if (changed || _scene._result == nullptr)
        {
            // Keep any successfully recorded builds alive even if a later allocation fails.
            _scene = RaytracingAccelerationStructure();
            if (!renderModule.buildRaytracingScene(geometries, _scene))
                return false;
            _geometries = DK::move(geometries);
        }
        _materialTextures = DK::move(textures);
        for (auto& texture : _materialTextures)
            renderModule.resourceBarrierTransition(texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        return true;
    }

    void RaytracingRenderer::dispatchRay(RenderModule& renderModule, const IBufferRef& sceneConstants)
    {
        if (_outputTexture == nullptr || _outputAccumulateTexture == nullptr)
            return;

        renderModule.resourceBarrierTransition(_outputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        renderModule.resourceBarrierTransition(_outputAccumulateTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        startRenderPass(renderModule, "PathTracing", 0xffffffff);
        {
            startPipelinePermutation("BruteForce", static_cast<uint32>(_mode));
            {
                setConstantBuffer("SceneConstantBuffer", sceneConstants);

                setRootConstantParameter("_accumulateTextureUAV", _outputAccumulateTexture->getUAV());
                setRootConstantParameter("_targetUAV", _outputTexture->getUAV());
                setRootConstantParameter("_sampleCount", _sampleCount);
                setRootConstantParameter("_samplingMode", _samplingMode);
                setShaderResourceView("gTLAS", _scene._result);
                renderModule.dispatchRays(_width, _height);
                ++_sampleCount;
            }
            endPipeline();
        }
        endRenderPass();

        renderModule.copyTextureToRenderTarget(_outputTexture, 1);
    }
}
