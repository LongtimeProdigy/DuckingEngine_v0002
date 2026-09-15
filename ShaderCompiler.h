#pragma once

struct IDxcBlob;
struct IDxcUtils;
struct IDxcCompiler3;

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
		ShaderCompiler();

		const bool compileShader(const char* shaderPath, const char* entry, const ShaderType shaderType, const DKVector<DKString>& defines, IDxcBlob* shader, D3D12_SHADER_BYTECODE& outShader) const;

	private:
		bool _initialized = false;
		RenderResourcePtr<IDxcUtils> _utils = nullptr;
		RenderResourcePtr<IDxcCompiler3> _compiler3 = nullptr;
	};
}
