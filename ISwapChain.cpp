#include "stdafx.h"
#include "ISwapChain.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
//#pragma comment(lib, "d3dcompiler.lib")
//#include <d3dcompiler.h>
#include "d3dx12.h"

ISwapChain::~ISwapChain()
{
	_swapChain->Release();
}

bool ISwapChain::Present(uint syncInterval, uint flag) const noexcept
{
	return SUCCEEDED(_swapChain->Present(syncInterval, flag));
}

uint ISwapChain::GetCurrentBackBufferIndex() const
{
	return static_cast<uint>(_swapChain->GetCurrentBackBufferIndex());
}