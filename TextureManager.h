#pragma once

struct D3D12_RESOURCE_DESC;

class TextureManager
{
public:
	TextureManager() {}
	~TextureManager() {}

public:
	bool LoadTextureSRV(const char* texturePath) const;

private:
	bool LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow, int& imageSize) const;
};