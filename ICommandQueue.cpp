#include "stdafx.h"
#include "ICommandQueue.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

#include "ICommandList.h"
#include "IFence.h"

ICommandQueue::~ICommandQueue()
{
	_commandQueue->Release();
}

void ICommandQueue::ExecuteCommandLists(const uint commandListCount, const std::vector<ICommandList*>& commandLists) const noexcept
{
	std::vector<ID3D12CommandList*> rawCommandLists;
	rawCommandLists.resize(commandListCount);
	for (uint i = 0; i < commandListCount; ++i)
	{
		rawCommandLists[i] = commandLists[i]->GetCommandListWritable();
	}
	_commandQueue->ExecuteCommandLists(commandListCount, &rawCommandLists[0]);
}

bool ICommandQueue::Signal(IFence* fence, uint64 value) const noexcept
{
	return SUCCEEDED(_commandQueue->Signal(fence->GetFenceWritable(), value));
}