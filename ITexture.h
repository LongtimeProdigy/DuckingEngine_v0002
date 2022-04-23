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
	// 원래는 필요없는데 들고 있지 않으면 container에서 들고있는 textureRaw의 count가 증가되지 않아 여기서 가지고 있습니다.
	const char* _texturePath;
	const TextureRawRef _textureRaw;

	// TextureParameter에서 변경 시에 접근할 handle
	const D3D12_CPU_DESCRIPTOR_HANDLE _handle;
};