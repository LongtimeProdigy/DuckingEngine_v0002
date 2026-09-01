#include "stdafx.h"
#include "RaytracingRenderer.h"
#include "RenderModule.h"
#include "DuckingEngine.h"
#include "SceneObjectManager.h"
#include "SceneObject.h"
#include "StaticMeshComponent.h"
#include "Model.h"
#include "Material.h"

namespace DK
{
	const bool RaytracingRenderer::initialize(RenderModule* renderModule, const uint32 width, const uint32 height)
	{
		HRESULT hr;

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
		hr = renderModule->_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "DXR is not supported by this device.");
			return false;
		}

		if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			DK_ASSERT_LOG(false, "DXR is not supported by this device.");
			return false;
		}

        // Raytracing Output Texture & UAV
        {
            _outputTexture = renderModule->createTexture(
                "Raytracing Output Texture", width, height, nullptr,
                1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false, true
            );

            if (_outputTexture.get() == nullptr)
                return false;
        }

        _renderPassName = "PathTracing";
        _pipelineName = "BruteForce";

        // SBT
        {
            Pipeline* pipeline = renderModule->getRenderPass(_renderPassName)->getPipeline(_pipelineName);

            //UINT64 rayGenSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
            //UINT64 missSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
            //UINT64 hitGroupSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
            UINT64 sbtSize = 192;
            CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
            D3D12_RESOURCE_DESC sbtDesc = CD3DX12_RESOURCE_DESC::Buffer(sbtSize);
            hr = renderModule->_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &sbtDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_SBT.getAddress()));
            if (FAILED(hr))
                return false;

            void* rayGenIdentifier = pipeline->_rtStateObjectProperties->GetShaderIdentifier(L"RayGen");
            void* missIdentifier = pipeline->_rtStateObjectProperties->GetShaderIdentifier(L"Miss");
            void* hitGroupIdentifier = pipeline->_rtStateObjectProperties->GetShaderIdentifier(L"HitGroup");

            void* mapped = nullptr;
            D3D12_RANGE range = {};
            _SBT->Map(0, &range, &mapped);
            memset(mapped, 0, sbtSize);
            memcpy(static_cast<uint8_t*>(mapped) + rayGenOffset, rayGenIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            memcpy(static_cast<uint8_t*>(mapped) + missOffset, missIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            memcpy(static_cast<uint8_t*>(mapped) + hitGroupOffset, hitGroupIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            _SBT->Unmap(0, nullptr);
        }

        _width = width;
        _height = height;

		return true;
	}

    BLAS createBlas(ID3D12Device8* device, ID3D12GraphicsCommandList4* commandList, const StaticMeshModel::SubMeshType& subMesh)
	{
        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        // Vertex
        geometryDesc.Triangles.VertexBuffer.StartAddress = subMesh._vertexBufferView->BufferLocation;// vertexBuffer->GetGPUVirtualAddress();
        geometryDesc.Triangles.VertexBuffer.StrideInBytes = subMesh._vertexBufferView->StrideInBytes;// vertexStride;
        geometryDesc.Triangles.VertexCount = subMesh._vertexBufferView->SizeInBytes / subMesh._vertexBufferView->StrideInBytes;
        geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        // Index
        geometryDesc.Triangles.IndexBuffer = subMesh._indexBufferView->BufferLocation;
        geometryDesc.Triangles.IndexCount = subMesh._indexBufferView->SizeInBytes / (subMesh._indexBufferView->Format == DXGI_FORMAT_R16_UINT ? 2 : 4);
        geometryDesc.Triangles.IndexFormat = subMesh._indexBufferView->Format;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.NumDescs = 1;
        inputs.pGeometryDescs = &geometryDesc;
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

        if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
        {
            DK_ASSERT_LOG(false, "failed craeteblas prebuild");
            return BLAS();
        }

        ID3D12Resource* blas = nullptr;
        ID3D12Resource* scratch = nullptr;

        D3D12_RESOURCE_DESC blasDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );
        CD3DX12_HEAP_PROPERTIES blasProperty(D3D12_HEAP_TYPE_DEFAULT);
        HRESULT hr = device->CreateCommittedResource(&blasProperty, D3D12_HEAP_FLAG_NONE, &blasDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&blas));
        if (FAILED(hr))
        {
            DK_ASSERT_LOG(false, "failed craeteblas blas");
            blas = nullptr;
            return BLAS();
        }

        D3D12_RESOURCE_DESC scratchDesc =CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        CD3DX12_HEAP_PROPERTIES scratchProperty(D3D12_HEAP_TYPE_DEFAULT);
        hr = device->CreateCommittedResource(&scratchProperty, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&scratch));
        if (FAILED(hr))
        {
            DK_ASSERT_LOG(false, "failed craeteblas scratch");
            blas->Release();
            blas = nullptr;
            scratch = nullptr;
            return BLAS();
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.Inputs = inputs;
        buildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
        buildDesc.DestAccelerationStructureData = blas->GetGPUVirtualAddress();
        commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = blas;
        commandList->ResourceBarrier(1, &barrier);

        return BLAS(blas, scratch);
	}
    TLAS createTLAS(ID3D12Device8* device, ID3D12GraphicsCommandList4* commandList, DKVector<BLAS>&& blases)
    {
        const uint32 blasCount = blases.size();

        UINT64 instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * blasCount;
        CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);

        ID3D12Resource* instanceBuffer = nullptr;
        HRESULT hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &instanceBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&instanceBuffer));
        if (FAILED(hr))
        {
            instanceBuffer = nullptr;
            return TLAS();
        }

        void* mappedData = nullptr;
        D3D12_RANGE readRange = {};
        readRange.Begin = 0;
        readRange.End = 0;
        hr = instanceBuffer->Map(0, &readRange, &mappedData );
        if (FAILED(hr))
        {
            instanceBuffer->Release();
            instanceBuffer = nullptr;
            return TLAS();
        }

        DKVector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesces;
        instanceDesces.resize(blasCount);
        for (uint32 i = 0; i < blasCount; ++i)
        {
            BLAS& blas = blases[i];

            D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDesces[i];
            instanceDesc.InstanceID = i;
            instanceDesc.InstanceContributionToHitGroupIndex = 0;       // TODO: Raytracing PipelineObject의 HitGroup내에 있는 HitShader 이름 Index이다. Material이 연결되면 작업해야할듯?
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            instanceDesc.AccelerationStructure = blas._blas->GetGPUVirtualAddress();

            instanceDesc.Transform[0][0] = 1.0f;
            instanceDesc.Transform[0][1] = 0.0f;
            instanceDesc.Transform[0][2] = 0.0f;
            instanceDesc.Transform[0][3] = 0.0f;

            instanceDesc.Transform[1][0] = 0.0f;
            instanceDesc.Transform[1][1] = 1.0f;
            instanceDesc.Transform[1][2] = 0.0f;
            instanceDesc.Transform[1][3] = 0.0f;

            instanceDesc.Transform[2][0] = 0.0f;
            instanceDesc.Transform[2][1] = 0.0f;
            instanceDesc.Transform[2][2] = 1.0f;
            instanceDesc.Transform[2][3] = 0.0f;
        }

        memcpy(mappedData, instanceDesces.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        instanceBuffer->Unmap(0, nullptr);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.NumDescs = instanceDesces.size();
        inputs.InstanceDescs = instanceBuffer->GetGPUVirtualAddress();
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
        if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
        {
            instanceBuffer->Release();
            instanceBuffer = nullptr;
            return TLAS();
        }

        // TLAS
        ID3D12Resource* tlas = nullptr;
        ID3D12Resource* scratch = nullptr;

        D3D12_RESOURCE_DESC tlasDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &tlasDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&tlas));
        if (FAILED(hr))
        {
            instanceBuffer->Release();
            instanceBuffer = nullptr;
            return TLAS();
        }

        D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        hr = device->CreateCommittedResource( &defaultHeap, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&scratch));
        if (FAILED(hr))
        {
            tlas->Release();
            tlas = nullptr;
            instanceBuffer->Release();
            instanceBuffer = nullptr;
            return TLAS();
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.Inputs = inputs;
        buildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
        buildDesc.DestAccelerationStructureData = tlas->GetGPUVirtualAddress();
        commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = tlas;
        commandList->ResourceBarrier(1, &barrier);

        return TLAS(DK::move(blases), tlas, scratch);
    }
	void RaytracingRenderer::updateRaytracingRenderer(RenderModule& renderModule)
	{
        if (_refresh == false)
            return;

        _refresh = false;
        _tlas.release();

        ID3D12Device8* device = renderModule._device.get();
        ID3D12GraphicsCommandList4* commandList = renderModule._commandList->_commandList.get();

        DKVector<BLAS> blases;

		const DKHashMap<uint32, SceneObject>& sceneObjects = DuckingEngine::getInstance().GetSceneObjectManager().getSceneObjects();
		for (DKHashMap<const uint32, SceneObject>::const_iterator iter = sceneObjects.begin(); iter != sceneObjects.end(); ++iter)
		{
            const SceneObject& sceneObject = iter->second;
            uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
            for (uint32 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
            {
                // #todo- component 완전 개편 필요해보임.
                // for문이 아니라 unity, unreal에서는 GetComponent<T>가 어떻게 작동하는지 보고 개편할 것
                // 참고링크: https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c
                const StaticMeshComponent* staticMeshComponent = static_cast<const StaticMeshComponent*>(sceneObject._components[componentIndex].get());
                const DKVector<StaticMeshModel::SubMeshType>& subMeshes = staticMeshComponent->get_model()->get_subMeshArr();
                blases.reserve(blases.size() + subMeshes.size());
                for (uint32 subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
                {
                    const StaticMeshModel::SubMeshType& subMesh = subMeshes[subMeshIndex];
                    BLAS blas = createBlas(device, commandList, subMesh);
                    if (blas.isValid() == false)
                        continue;

                    blases.push_back(DK::move(blas));
                }
            }
		}

        TLAS tlas = createTLAS(device, commandList, DK::move(blases));
        if (tlas.isValid() == false)
            return;

        _tlas = DK::move(tlas);
	}
    void RaytracingRenderer::dispatchRay(RenderModule& renderModule)
    {
        startRenderPass(renderModule, "PathTracing", 0xFFFFFFFF, 0, false, false, true);
        {
            startPipeline("BruteForce");
            {
                setRootConstantParameter("_targetUAV", _outputTexture->getUAV());
                setShaderResourceView("gTLAS", _tlas._tlas->GetGPUVirtualAddress());

                D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
                D3D12_GPU_VIRTUAL_ADDRESS sbtAddress = _SBT->GetGPUVirtualAddress();

                dispatchDesc.RayGenerationShaderRecord.StartAddress = sbtAddress + rayGenOffset;
                dispatchDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                dispatchDesc.MissShaderTable.StartAddress = sbtAddress + missOffset;
                dispatchDesc.MissShaderTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                dispatchDesc.MissShaderTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                dispatchDesc.HitGroupTable.StartAddress = sbtAddress + hitGroupOffset;
                dispatchDesc.HitGroupTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                dispatchDesc.HitGroupTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                dispatchDesc.Width = _width;
                dispatchDesc.Height = _height;
                dispatchDesc.Depth = 1;

                renderModule._commandList->_commandList->DispatchRays(&dispatchDesc);
            }
            endPipeline();
        }
        endRenderPass();

        // ============================================================
        // Output UAV → COPY_SOURCE
        // ============================================================
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = _outputTexture->getTextureBuffer();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        renderModule._commandList->_commandList->ResourceBarrier(1, &barrier);

        // ============================================================
        // BackBuffer → COPY_DEST
        // ============================================================
        barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = renderModule._backBufferResourceArr[renderModule.kCurrentFrameIndex].get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        renderModule._commandList->_commandList->ResourceBarrier(1, &barrier);

        // ============================================================
        // Copy
        // ============================================================
        renderModule._commandList->_commandList->CopyResource(renderModule._backBufferResourceArr[renderModule.kCurrentFrameIndex].get(), _outputTexture->getTextureBuffer());

        // ============================================================
        // BackBuffer → PRESENT
        // ============================================================
        barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = renderModule._backBufferResourceArr[renderModule.kCurrentFrameIndex].get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        renderModule._commandList->_commandList->ResourceBarrier(1, &barrier);

        // ============================================================
        // Output UAV → UAV
        // ============================================================
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = _outputTexture->getTextureBuffer();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        renderModule._commandList->_commandList->ResourceBarrier(1, &barrier);
    }
}
