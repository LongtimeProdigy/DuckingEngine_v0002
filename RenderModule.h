#pragma once

#include "Matrix4x4.h"

struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

enum class RenderModuleType
{
	DIRECTX11, 
	DIRECTX12,
	VULKAN, 
	OPENGL
};

class RenderModuleDX12;
class IResource;
class Camera;
class SkinnedMeshComponent;
enum class MaterialType : uint8;
class Material;
class ITexture;
class SceneObject;

class RenderModule
{
public:
	static uint kFrameCount;

	RenderModule();
	virtual ~RenderModule();

public:
	static RenderModule* CreateRenderModule(RenderModuleType type, uint frameCount);

public:
	virtual bool Initialize(HWND hwnd, uint width, uint height) = 0;

	// Note(220213): Container�� ��Ƽ� cache�صδ� ������Ʈ�� ��� �Լ� �̸��� Load Ű���带 ����մϴ�.
	// ȣ��ø��� ���� �����ؼ� ��ȯ�ϴ� ������Ʈ�� ��� �Լ� �̸��� Create ���λ縦 ����մϴ�.
	virtual Material* CreateMaterial(const MaterialType materialType, const SceneObject* sceneObject) = 0;
	virtual IResource* CreateBuffer(void* data, uint size) const = 0;
	virtual IResource* CreateUploadBuffer(void* data, uint size) const = 0;
	virtual const bool LoadVertexBuffer(const void* data, const uint strideSize, const uint bufferSize, VertexBufferViewRef& outView) = 0;
	virtual const bool LoadIndexBuffer(const void* data, const uint bufferSize, IndexBufferViewRef& outView) = 0;
	virtual ITexture CreateTexture(const char* texturePath, const D3D12_CPU_DESCRIPTOR_HANDLE& handle) = 0;

	virtual void UpdateUploadResource(IResource* resource, void* data, uint size) const noexcept = 0;

	virtual void PreRender() const noexcept = 0;
	virtual bool RenderSkinnedMeshComponent(const SkinnedMeshComponent* skinnedMeshComponent) const = 0;
	virtual void EndRender() const noexcept = 0;

	virtual void RenderUI() const noexcept = 0;

public:
	mutable bool _isRunning = true;
};