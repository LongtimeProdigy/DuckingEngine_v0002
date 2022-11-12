#include "stdafx.h"
#include "EditorDebugDrawManager.h"

#include "float4.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

EditorDebugDrawManager* EditorDebugDrawManager::_this = nullptr;

VertexBufferViewRef EditorDebugDrawManager::SpherePrimitiveInfo::kVertexBufferView = nullptr;
IndexBufferViewRef EditorDebugDrawManager::SpherePrimitiveInfo::kIndexBufferView = nullptr;
uint32 EditorDebugDrawManager::SpherePrimitiveInfo::indexCount = 0;

bool EditorDebugDrawManager::initialize()
{
	initialize_Sphere();

	return false;
}

bool EditorDebugDrawManager::initialize_Sphere()
{
	static float4 vertices[6] =
	{
		float4(1, 0, 0, 1),
		float4(0, 0, 1, 1),
		float4(0, 1, 0, 1),
		float4(-1, 0, 0, 1),
		float4(0, 0, -1, 1),
		float4(0, -1, 0, 1)
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

	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	renderModule.createVertexBuffer(vertices, sizeof(decltype(vertices[0])), ARRAYSIZE(vertices), SpherePrimitiveInfo::kVertexBufferView);
	renderModule.createIndexBuffer(indices, ARRAYSIZE(indices), SpherePrimitiveInfo::kIndexBufferView);
	SpherePrimitiveInfo::indexCount = ARRAYSIZE(indices);

	_spherePrimitiveInfoArr.reserve(MAX_ELEMENT_COUNT);
	DKVector<SpherePrimitiveInfo> worldPositionBuffer;
	worldPositionBuffer.resize(MAX_ELEMENT_COUNT);
	uint32 strideSize = sizeof(decltype(worldPositionBuffer[0]));
	uint32 elementCount = static_cast<uint32>(worldPositionBuffer.size());
	_sphereDebugDrawElementBuffer = renderModule.createUploadBuffer(worldPositionBuffer.data(), strideSize * elementCount);

	return true;
}

void EditorDebugDrawManager::prepareShaderData()
{
	_sphereDebugDrawElementBuffer->upload(_spherePrimitiveInfoArr.data());
}

void EditorDebugDrawManager::endUpdateRender()
{
	_spherePrimitiveInfoArr.clear();
}