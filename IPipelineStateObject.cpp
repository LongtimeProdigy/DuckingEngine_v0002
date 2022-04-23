#include "stdafx.h"
#include "IPipelineStateObject.h"

#pragma region D3D12 Lib
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

IPipelineStateObject::~IPipelineStateObject()
{
	_pipelineStateObject->Release();
}

const ID3D12PipelineState* IPipelineStateObject::GetPipelineStateObject() const noexcept
{
	return _pipelineStateObject;
}
ID3D12PipelineState* IPipelineStateObject::GetPipelineStateObjectWritable() const noexcept
{
	return _pipelineStateObject;
}
