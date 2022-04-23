#include "stdafx.h"
#include "IFence.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
//#pragma comment(lib, "d3dcompiler.lib")
//#include <d3dcompiler.h>
#include "d3dx12.h"

IFence::~IFence()
{
	_fence->Release();
}

uint64 IFence::GetCompletedValue() const noexcept
{
	return _fence->GetCompletedValue();
}

bool IFence::SetEventOnCompletion(uint64 value, HANDLE handle) const noexcept
{
	return SUCCEEDED(_fence->SetEventOnCompletion(value, handle));
}