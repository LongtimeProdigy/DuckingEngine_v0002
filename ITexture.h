#pragma once

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

//struct D3D12_CPU_DESCRIPTOR_HANDLE;

class ITexture
{
public:
	ITexture(const char* texturePath, const TextureRawRef& textureRaw, const D3D12_CPU_DESCRIPTOR_HANDLE& handle)
		: _texturePath(texturePath)
		, _textureRaw(textureRaw)
		, _handle(handle)
	{}

private:
	// ������ �ʿ���µ� ��� ���� ������ container���� ����ִ� textureRaw�� count�� �������� �ʾ� ���⼭ ������ �ֽ��ϴ�.
	const char* _texturePath;
	const TextureRawRef _textureRaw;

	// TextureParameter���� ���� �ÿ� ������ handle
	const D3D12_CPU_DESCRIPTOR_HANDLE _handle;
};