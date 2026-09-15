#include "stdafx.h"
#include "EditorDebugDrawManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

namespace DK
{
	EditorDebugDrawManager* EditorDebugDrawManager::_this = nullptr;

	RenderResourcePtr<ID3D12Resource> EditorDebugDrawManager::SpherePrimitiveInfo::kVertexBuffer = nullptr;
	RenderResourcePtr<ID3D12Resource> EditorDebugDrawManager::SpherePrimitiveInfo::kIndexBuffer = nullptr;
	VertexBufferViewRef EditorDebugDrawManager::SpherePrimitiveInfo::kVertexBufferView = nullptr;
	IndexBufferViewRef EditorDebugDrawManager::SpherePrimitiveInfo::kIndexBufferView = nullptr;
	uint32 EditorDebugDrawManager::SpherePrimitiveInfo::indexCount = 0;

	RenderResourcePtr<ID3D12Resource> EditorDebugDrawManager::LinePrimitiveInfo::kVertexBuffer = nullptr;
	RenderResourcePtr<ID3D12Resource> EditorDebugDrawManager::LinePrimitiveInfo::kIndexBuffer = nullptr;
	VertexBufferViewRef EditorDebugDrawManager::LinePrimitiveInfo::kVertexBufferView = nullptr;
	IndexBufferViewRef EditorDebugDrawManager::LinePrimitiveInfo::kIndexBufferView = nullptr;
	uint32 EditorDebugDrawManager::LinePrimitiveInfo::indexCount = 0;

	bool EditorDebugDrawManager::initialize()
	{
		initialize_Sphere();
		initialize_Line();

		return false;
	}

	bool initialize_Common(const float3* vertices, const uint32 strideSize, const uint32 vertexCount, const uint32* indices, const uint32 indexCount, uint32& outIndexCount, RenderResourcePtr<ID3D12Resource>& vertexBuffer, VertexBufferViewRef& vertexBufferView, RenderResourcePtr<ID3D12Resource>& indexBuffer, IndexBufferViewRef& indexBufferView, Ptr<IBuffer>& primitiveInfoBuffer, const DKStringW& vertexDebugName, const DKStringW& indexDebugName, const DKStringW& cbufferDebugName)
	{
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		renderModule.createVertexBuffer(vertices, sizeof(decltype(vertices[0])), vertexCount, vertexBuffer, vertexBufferView, vertexDebugName);
		renderModule.createIndexBuffer(indices, indexCount, indexBuffer, indexBufferView, indexDebugName);
		outIndexCount = indexCount;

		uint32 elementCount = MAX_ELEMENT_COUNT;
		primitiveInfoBuffer = renderModule.createUploadBuffer(strideSize * elementCount, cbufferDebugName);

		return true;
	}
	bool EditorDebugDrawManager::initialize_Sphere()
	{
		static float3 vertices[6] =
		{
			float3(1, 0, 0),
			float3(0, 0, 1),
			float3(0, 1, 0),
			float3(-1, 0, 0),
			float3(0, 0, -1),
			float3(0, -1, 0)
		};
		static uint32 indices[24] =
		{
			0, 1, 2,
			1, 3, 2,
			3, 4, 2,
			4, 0, 2,
			0, 5, 1,
			1, 5, 3,
			3, 5, 4,
			4, 5, 0
		};

		_primitiveInfoSphereArr.reserve(MAX_ELEMENT_COUNT);
		initialize_Common(
			vertices, sizeof(decltype(vertices[0])), ARRAYSIZE(vertices),
			indices, ARRAYSIZE(indices),
			SpherePrimitiveInfo::indexCount,
			SpherePrimitiveInfo::kVertexBuffer, SpherePrimitiveInfo::kVertexBufferView,
			SpherePrimitiveInfo::kIndexBuffer, SpherePrimitiveInfo::kIndexBufferView,
			_primitiveInfoSphereBuffer, 
			L"DebugDrawElement_Sphere_VertexBuffer", L"DebugDrawElement_Sphere_IndexBuffer", L"DebugDrawElement_Sphere_PrimitiveBuffer"
		);

		return true;
	}
	bool EditorDebugDrawManager::initialize_Line()
	{
		static float3 vertices[2] =
		{
			float3(0, 0, 0),
			float3(0, 0, 0)
		};
		static uint32 indices[2] =
		{
			0, 1
		};

		_primitiveInfoLineArr.reserve(MAX_ELEMENT_COUNT);
		initialize_Common(
			vertices, sizeof(decltype(vertices[0])), ARRAYSIZE(vertices),
			indices, ARRAYSIZE(indices),
			LinePrimitiveInfo::indexCount,
			LinePrimitiveInfo::kVertexBuffer, LinePrimitiveInfo::kVertexBufferView,
			LinePrimitiveInfo::kIndexBuffer, LinePrimitiveInfo::kIndexBufferView,
			_primitiveInfoLineBuffer,
			L"DebugDrawElement_Line_VertexBuffer", L"DebugDrawElement_Line_IndexBuffer", L"DebugDrawElement_Line_PrimitiveBuffer"
		);

		return true;
	}

	void EditorDebugDrawManager::prepareShaderData()
	{
		_primitiveInfoSphereBuffer->upload(_primitiveInfoSphereArr.data());
		_primitiveInfoLineBuffer->upload(_primitiveInfoLineArr.data());
	}

	void EditorDebugDrawManager::endUpdateRender()
	{
		_primitiveInfoSphereArr.clear();
		_primitiveInfoLineArr.clear();
	}
}
