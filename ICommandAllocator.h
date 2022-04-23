#pragma once

struct ID3D12CommandAllocator;

class ICommandAllocator
{
public:
	ICommandAllocator(ID3D12CommandAllocator* commandAllocator)
		:_commandAllocator(commandAllocator)
	{}
	~ICommandAllocator();

	dk_inline const ID3D12CommandAllocator* GetCommandAllocator() const noexcept
	{
		return _commandAllocator;
	}
	dk_inline ID3D12CommandAllocator* GetCommnadAllocatorWritable() const noexcept
	{
		return _commandAllocator;
	}

	bool Reset() const noexcept;

private:
	ID3D12CommandAllocator* _commandAllocator;
};