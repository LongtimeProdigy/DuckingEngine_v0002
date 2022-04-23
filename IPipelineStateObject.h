#pragma once

struct ID3D12PipelineState;

class IPipelineStateObject
{
public:
	IPipelineStateObject(ID3D12PipelineState* pso)
		: _pipelineStateObject(pso)
	{}
	~IPipelineStateObject();

	const ID3D12PipelineState* GetPipelineStateObject() const noexcept;
	ID3D12PipelineState* GetPipelineStateObjectWritable() const noexcept;

private:
	ID3D12PipelineState* _pipelineStateObject;
};