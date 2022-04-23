#pragma once

struct ID3D12RootSignature;

class IRootSignature
{
public:
	IRootSignature(ID3D12RootSignature* rootSignature)
		: _rootSignature(rootSignature)
	{}
	IRootSignature(const ID3D12RootSignature& lvalue) = delete;

	dk_inline const ID3D12RootSignature* GetRootSignature() const noexcept
	{
		return _rootSignature;
	}
	dk_inline ID3D12RootSignature* GetRootSignatureWritable() const noexcept
	{
		return _rootSignature;
	}

private:
	ID3D12RootSignature* _rootSignature;
};