#pragma once

struct IDXGISwapChain4;

class ISwapChain
{
public:
	ISwapChain(IDXGISwapChain4* swapChain)
		: _swapChain(swapChain)
	{}
	~ISwapChain();

	uint GetCurrentBackBufferIndex() const;

	bool Present(uint syncInterval, uint flag) const noexcept;

private:
	IDXGISwapChain4* _swapChain;
};