#pragma once

struct IDxcBlob;

namespace DK
{
	enum class ShaderType : uint8
	{
		VertexShader,
		PixelShader,
		ComputeShader,
		Raytracing, 
		COUNT
	};

	class ShaderCompiler
	{
	public:
		static const bool compileShader(const char* shaderPath, const char* entry, const ShaderType shaderType, const DKVector<DKString>& defines, IDxcBlob* shader, D3D12_SHADER_BYTECODE& outShader);
	};
}
