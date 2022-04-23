#pragma once

struct ID3D12CommandQueue;

class ICommandList;
class IFence;

class ICommandQueue
{
public:
	ICommandQueue(ID3D12CommandQueue* commandQueue)
		: _commandQueue(commandQueue)
	{}
	~ICommandQueue();
	
	void ExecuteCommandLists(const uint commandListCount, const std::vector<ICommandList*>& commandLists) const noexcept;
	bool Signal(IFence* fence, uint64 value) const noexcept;

private:
	ID3D12CommandQueue* _commandQueue = nullptr;
};