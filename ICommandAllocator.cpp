#include "stdafx.h"
#include "ICommandAllocator.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
//#pragma comment(lib, "d3dcompiler.lib")
//#include <d3dcompiler.h>
#include "d3dx12.h"

ICommandAllocator::~ICommandAllocator()
{
	_commandAllocator->Release();
}

bool ICommandAllocator::Reset() const noexcept
{
	return SUCCEEDED(_commandAllocator->Reset());
}