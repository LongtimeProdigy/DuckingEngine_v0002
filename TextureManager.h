#pragma once

struct ID3D12Resource;
struct ID3D12DescriptorHeap;

class TextureRaw
{
public:
	uint _width;
	uint _height;
	uint _bitsPerPixel;
	byte* _data;
	DXGI_FORMAT _format;
};

// MaterialParameter�� ���̴� Type�� POD�� �����ؾ��մϴ�. (memcpy�� �ϱ⶧��)
class ITexture
{
public:
	ITexture(const DKString& path)
		: _path(path)
	{}

	dk_inline const DKString& getPath() const
	{
		return _path;
	}

private:
	DKString _path;
	ID3D12Resource* _resource;
};

class TextureManager
{
public:
	bool initialize();

	ITextureRef createTexture(const DKString& texturePath, uint32& outindex);

private:
	DKHashMap<DKString, uint32> _pathMap;	// key: path, value: index(_textures)
	DKVector<ITextureRef> _textures;

	ID3D12DescriptorHeap* _textureDescriptorHeap;
};