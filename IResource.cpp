#include "stdafx.h"
#include "IResource.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

IResource::~IResource()
{
	_resource->Release();
}

uint8* IResource::Map() const noexcept
{
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = 0;
	uint8* address = nullptr;
	HRESULT hr = _resource->Map(0, nullptr, reinterpret_cast<void**>(&address));
	DK_ASSERT_LOG(SUCCEEDED(hr), "Map½ÇÆÐ!");

	return address;
}

void IResource::UnMap() const noexcept
{
	_resource->Unmap(0, nullptr);
}