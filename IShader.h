#pragma once

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

//struct D3D12_SHADER_BYTECODE;

class IShader
{
public:
	IShader(const D3D12_SHADER_BYTECODE& shaderByteCode)
		: _shaderByteCode(shaderByteCode)
	{}
	~IShader() {}

	dk_inline const D3D12_SHADER_BYTECODE& GetShaderByteCode() const noexcept
	{
		return _shaderByteCode;
	}
	dk_inline D3D12_SHADER_BYTECODE& GetShaderByteCodeWritable() noexcept
	{
		return _shaderByteCode;
	}

private:
	D3D12_SHADER_BYTECODE _shaderByteCode;
};