#pragma once

struct IDxcBlob;
struct IDxcUtils;
struct IDxcCompiler3;

enum _D3D_SHADER_INPUT_TYPE : int;
enum _D3D_SRV_DIMENSION : int;

namespace DK
{
	struct ShaderVariableReflection
	{
		DKString _name;
		uint32 _offset = 0; // byte 단위
		uint32 _size = 0;   // byte 단위
	};

	struct ShaderResourceReflection
	{
		DKString _name;

		D3D_SHADER_INPUT_TYPE _type{};
		D3D_SRV_DIMENSION _dimension{};

		uint32 _register = 0;
		uint32 _space = 0;
		uint32 _bindCount = 0;

		uint32 _constantBufferSize = 0;
		DKVector<ShaderVariableReflection> _variables;
	};

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

		const bool compileShader(const char* shaderPath, const char* entry, const ShaderType shaderType, const DKVector<DKString>& defines, RenderResourcePtr<IDxcBlob>& shader, D3D12_SHADER_BYTECODE& outShader, DKVector<ShaderResourceReflection>& outResources, uint32* outThreadGroupSize = nullptr) const;

	private:
		bool _initialized = false;
		RenderResourcePtr<IDxcUtils> _utils = nullptr;
		RenderResourcePtr<IDxcCompiler3> _compiler3 = nullptr;
	};
}
