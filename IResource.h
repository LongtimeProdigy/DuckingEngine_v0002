#pragma once

struct ID3D12Resource;

class IResource
{
public:
	IResource(ID3D12Resource* resource)
		: _resource(resource)
	{}
	~IResource();

	dk_inline const ID3D12Resource* GetResource() const noexcept
	{
		return _resource;
	}
	dk_inline ID3D12Resource* GetResourceWritable() const noexcept
	{
		return _resource;
	}

	uint8* Map() const noexcept;
	void UnMap() const noexcept;

private:
	ID3D12Resource* _resource;
};