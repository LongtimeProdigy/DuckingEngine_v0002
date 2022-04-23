#pragma once

struct ID3D12Fence;

class IFence
{
public:
	IFence(ID3D12Fence* fence)
		: _fence(fence)
	{}
	~IFence();

	dk_inline const ID3D12Fence* GetFence() const noexcept
	{
		return _fence;
	}
	dk_inline ID3D12Fence* GetFenceWritable() const noexcept
	{
		return _fence;
	}

	uint64 GetCompletedValue() const noexcept;
	bool SetEventOnCompletion(uint64 value, HANDLE handle) const noexcept;

private:
	ID3D12Fence* _fence;
};

