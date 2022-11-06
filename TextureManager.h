#pragma once

struct ID3D12Resource;
struct ID3D12DescriptorHeap;
enum DXGI_FORMAT;

class TextureRaw
{
public:
	uint _width;
	uint _height;
	uint _bitsPerPixel;
	byte* _data;
	DXGI_FORMAT _format;
};

// MaterialParameter로 쓰이는 Type은 POD를 유지해야합니다. (memcpy를 하기때문)
class ITexture
{
public:
	using TextureSRVType = uint32;
	static constexpr TextureSRVType kErrorTextureSRV = -1;
public:
	ITexture(const DKString& path, const TextureSRVType& textureSRV)
		: _path(path)
		, _textureSRV(textureSRV)
	{}

	dk_inline const DKString& getPath() const
	{
		return _path;
	}

	dk_inline const TextureSRVType& getSRV() const noexcept
	{
		return _textureSRV;
	}

private:
	DKString _path;
	TextureSRVType _textureSRV = kErrorTextureSRV;
};

class TextureManager
{
public:
	bool initialize();

	const ITextureRef& createTexture(const DKString& texturePath);

	dk_inline const RenderResourcePtr<ID3D12DescriptorHeap>& getTextureDescriptorHeap() const
	{
		return _textureDescriptorHeap;
	}
	dk_inline RenderResourcePtr<ID3D12DescriptorHeap>& getTextureDescriptorHeapWritable()
	{
		return _textureDescriptorHeap;
	}

private:
	DKHashMap<DKString, ITextureRef> _textureContainer;
	RenderResourcePtr<ID3D12DescriptorHeap> _textureDescriptorHeap;

	DKVector<ITexture::TextureSRVType> _deletedTextureSRV;
};