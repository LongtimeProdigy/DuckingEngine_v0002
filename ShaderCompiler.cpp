#include "stdafx.h"
#include "ShaderCompiler.h"
#include "RenderModule.h"

#include <d3d12shader.h>

namespace DK
{
	ShaderCompiler::ShaderCompiler()
	{
		IDxcUtils* utils;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "");
			return;
		}
		_utils = utils;

		IDxcCompiler3* compiler;
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "");
			return;
		}
		_compiler3 = compiler;

#ifdef _DK_DEBUG_
		IDxcVersionInfo* versionInfo = nullptr;
		hr = _compiler3->QueryInterface(IID_PPV_ARGS(&versionInfo));
		if (SUCCEEDED(hr))
		{
			UINT major = 0;
			UINT minor = 0;
			versionInfo->GetVersion(&major, &minor);
			DK_LOG("DXC Version: %u.%u", major, minor);
		}
		versionInfo->Release();
#endif

		_initialized = true;
	}
	const bool ShaderCompiler::compileShader(const char* shaderPath, const char* entry, const ShaderType shaderType, const DKVector<DKString>& defines, RenderResourcePtr<IDxcBlob>& shader, D3D12_SHADER_BYTECODE& outShader, DKVector<ShaderResourceReflection>& outResources, uint32* outThreadGroupSize)
	{
		if (outThreadGroupSize != nullptr)
		{
			outThreadGroupSize[0] = 0;
			outThreadGroupSize[1] = 0;
			outThreadGroupSize[2] = 0;
		}

		const ScopeString<DK_MAX_PATH> shaderFullPath = GlobalPath::makeResourceFullPath(shaderPath);
		const DKStringW shaderPathW = StringUtil::convertCtoWC(shaderFullPath.c_str());
		const DKStringW shaderEntryW = StringUtil::convertCtoWC(entry);

		IDxcBlobEncoding* blob;
		HRESULT hr = const_cast<RenderResourcePtr<IDxcUtils>&>(_utils)->LoadFile(shaderPathW.c_str(), nullptr, &blob);
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}
		RenderResourcePtr<IDxcBlobEncoding> sourceBlob = blob;

		// 참고: https://simoncoenen.com/blog/programming/graphics/DxcCompiling
		DKVector<LPCWSTR> arguments;
		//-E for the entry point (eg. PSMain)
		// Raytracing의 경우 entry를 쓰지 않음
		if (shaderType != ShaderType::Raytracing)
		{
			arguments.push_back(L"-E");
			arguments.push_back(shaderEntryW.c_str());
		}
		else
		{
			DKString temp(entry);
			DK_ASSERT_LOG(temp.empty(), "Raytracing의 경우 entry를 CompileShader에 지정하지 않습니다.");
		}

		//-T for the target profile (eg. ps_6_2)
		arguments.push_back(L"-T");
		switch (shaderType)
		{
		case ShaderType::VertexShader:
			arguments.push_back(L"vs_6_2");
			break;
		case ShaderType::PixelShader:
			arguments.push_back(L"ps_6_2");
			break;
		case ShaderType::ComputeShader:
			arguments.push_back(L"cs_6_2");
			break;
		case ShaderType::Raytracing:
			arguments.push_back(L"lib_6_5");
			//arguments.push_back(L"-HV");
			//arguments.push_back(L"2021");	// HLSL 2021로 컴파일 하란 뜻
			break;
		default:
			DK_ASSERT_LOG(false, "지원하지 않는 ShaderType입니다.");
			return false;
		}

#ifndef _DK_DEBUG_
		// HLSL Object파일에 Reflect, PBD파일을 제거하는 옵션
		// 하지만 IDxcResult에는 여전히 포함하기 때문에 getOutput으로 결과를 가져올 수 있습니다. (DXC_OUT_REFLECTION, DXC_OUT_PDB)
		//Strip reflection data and pdbs (see later)
		arguments.push_back(L"-Qstrip_debug");
		//arguments.push_back(L"-Qstrip_reflect");
#endif

#ifdef _DK_DEBUG_
		arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
		arguments.push_back(DXC_ARG_DEBUG); //-Zi
#endif
		arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp

		arguments.push_back(L"-I");
		ScopeStringW<DK_MAX_PATH> includePath = GlobalPath::makeResourceFullPathW(L"Material");
		arguments.push_back(includePath.c_str());

		//for (const DKString& define : defines)
		//{
		//	const DKStringW defineW = StringUtil::convertCtoWC(define.c_str());
		//	arguments.push_back(L"-D");
		//	arguments.push_back(defineW.c_str());
		//}

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
		sourceBuffer.Size = sourceBlob->GetBufferSize();
		sourceBuffer.Encoding = DXC_CP_ACP;

		IDxcIncludeHandler* handler;
		hr = const_cast<RenderResourcePtr<IDxcUtils>&>(_utils)->CreateDefaultIncludeHandler(&handler);
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "IncludeHandler 생성에 실패했습니다. Shader Compiler을 하지 않습니다.");
			return false;
		}
		RenderResourcePtr<IDxcIncludeHandler> defaultIncludeHandler = handler;

		IDxcResult* dxcResult;
		hr = _compiler3->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()), defaultIncludeHandler.get(), IID_PPV_ARGS(&dxcResult));
		RenderResourcePtr<IDxcResult> result = dxcResult;
		if (FAILED(hr))
		{
			IDxcBlobUtf8* errors;
			hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
			if (SUCCEEDED(hr))
				DK_ASSERT_LOG(false, "Shader Compile Error\nPath: %s\nLog: %s", shaderPath, errors->GetStringPointer());

			errors->Release();

			return false;
		}

		HRESULT status(S_OK);
		hr = result->GetStatus(&status);
		if (FAILED(hr) || FAILED(status))
		{
			IDxcBlobUtf8* errors;
			hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
			const char* test = errors->GetStringPointer();
			DK_ASSERT_LOG(FAILED(hr), "Shader Compile Error\nPath: %s\nLog: %s", shaderPath, test);

			errors->Release();

			return false;
		}

#ifdef _DK_DEBUG_	// PBD
		//{
		//	RenderResourcePtr<IDxcBlob> debugData;
		//	RenderResourcePtr<IDxcBlobUtf16> debugDataPath;
		//	hr = result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(debugData.getAddress()), debugDataPath.getAddress());
		//	if (FAILED(hr))
		//	{
		//		DK_ASSERT_LOG(false, "DebugData 가져오기 실패!");
		//		return false;
		//	}
		//	{
		//		DK_ASSERT_LOG(false, "DebugData(%d): %s", debugData->GetBufferSize(), debugData->GetBufferPointer());
		//		DxcBuffer dataBuffer;
		//		dataBuffer.Ptr = debugData->GetBufferPointer();
		//		dataBuffer.Size = debugData->GetBufferSize();
		//	}
		//	{
		//		DK_ASSERT_LOG(false, "DebugDataPath(%d): %s", debugDataPath->GetBufferSize(), debugDataPath->GetBufferPointer());
		//		DxcBuffer dataBuffer;
		//		dataBuffer.Ptr = debugDataPath->GetBufferPointer();
		//		dataBuffer.Size = debugDataPath->GetBufferSize();
		//	}
		//}
#endif

		IDxcBlob* reflectionDataPtr;
		hr = result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionDataPtr), nullptr);
		RenderResourcePtr<IDxcBlob> reflectionData = reflectionDataPtr;
		if (FAILED(hr) || reflectionData.get() == nullptr)
			return false;

		DxcBuffer reflectionBuffer{};
		reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
		reflectionBuffer.Size = reflectionData->GetBufferSize();
		reflectionBuffer.Encoding = 0;

		// ID3D12ShaderReflection과 ID3D12FunctionReflection 공통 처리
		auto collectResources = [&](auto* reflection, uint32 resourceCount) -> bool
		{
			for (uint32 i = 0; i < resourceCount; ++i)
			{
				D3D12_SHADER_INPUT_BIND_DESC desc{};
				if (FAILED(reflection->GetResourceBindingDesc(i, &desc)))
				{
					DK_ASSERT_LOG(false, "Shader Reflection 실패");
					return false;
				}

				ShaderResourceReflection resource;
				resource._name = desc.Name;
				resource._type = desc.Type;
				resource._dimension = desc.Dimension;
				resource._register = desc.BindPoint;
				resource._space = desc.Space;
				resource._bindCount = desc.BindCount;

				bool isDuplicate = false;
				for (const ShaderResourceReflection& prevReflection : outResources)
				{
					if (prevReflection._name == resource._name)
					{
						isDuplicate = true;
						break;
					}
				}
				if (isDuplicate)
					continue;

				if (desc.Type == D3D_SIT_CBUFFER)
				{
					ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByName(desc.Name);
					D3D12_SHADER_BUFFER_DESC cbDesc{};
					if (cb == nullptr || FAILED(cb->GetDesc(&cbDesc)))
					{
						DK_ASSERT_LOG(false, "Shader Reflection 실패");
						return false;
					}

					resource._constantBufferSize = cbDesc.Size;

					for (uint32 j = 0; j < cbDesc.Variables; ++j)
					{
						ID3D12ShaderReflectionVariable* variable = cb->GetVariableByIndex(j);

						D3D12_SHADER_VARIABLE_DESC variableDesc{};
						if (variable == nullptr || FAILED(variable->GetDesc(&variableDesc)))
						{
							DK_ASSERT_LOG(false, "Shader Reflection 실패");
							return false;
						}

						ShaderVariableReflection value;
						value._name = variableDesc.Name;
						value._offset = variableDesc.StartOffset;
						value._size = variableDesc.Size;

						resource._variables.push_back(DK::move(value));
					}
				}

				outResources.push_back(DK::move(resource));
			}

			return true;
		};

		RenderResourcePtr<IDxcUtils>& utils = const_cast<RenderResourcePtr<IDxcUtils>&>(_utils);

		if (shaderType == ShaderType::Raytracing)
		{
			ID3D12LibraryReflection* reflectionPtr;
			hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflectionPtr));
			if (FAILED(hr))
				return false;
			RenderResourcePtr<ID3D12LibraryReflection> reflection = reflectionPtr;

			D3D12_LIBRARY_DESC libraryDesc{};
			if (FAILED(reflection->GetDesc(&libraryDesc)))
				return false;

			for (uint32 i = 0; i < libraryDesc.FunctionCount; ++i)
			{
				ID3D12FunctionReflection* function = reflection->GetFunctionByIndex(i);
				D3D12_FUNCTION_DESC functionDesc{};
				if (function == nullptr || FAILED(function->GetDesc(&functionDesc)))
				{
					DK_ASSERT_LOG(false, "Shader Reflection 실패");
					return false;
				}

				if (collectResources(function, functionDesc.BoundResources) == false)
					return false;
			}
		}
		else
		{
			ID3D12ShaderReflection* reflectionPtr;
			hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflectionPtr));
			if (FAILED(hr))
			{
				DK_ASSERT_LOG(false, "Shader Reflection 실패");
				return false;
			}
			RenderResourcePtr<ID3D12ShaderReflection> reflection = reflectionPtr;

			D3D12_SHADER_DESC shaderDesc{};
			if (FAILED(reflection->GetDesc(&shaderDesc)))
			{
				DK_ASSERT_LOG(false, "Shader Reflection 실패");
				return false;
			}

			if (shaderType == ShaderType::ComputeShader)
			{
				uint32 sizeX = 0;
				uint32 sizeY = 0;
				uint32 sizeZ = 0;

				reflection->GetThreadGroupSize(&sizeX, &sizeY, &sizeZ);

				if (sizeX == 0 || sizeY == 0 || sizeZ == 0)
				{
					DK_ASSERT_LOG(false, "Compute thread group size is invalid: %s", shaderPath);
					return false;
				}

				if (outThreadGroupSize != nullptr)
				{
					outThreadGroupSize[0] = sizeX;
					outThreadGroupSize[1] = sizeY;
					outThreadGroupSize[2] = sizeZ;
				}
			}

			if (collectResources(reflection.get(), shaderDesc.BoundResources) == false)
				return false;
		}

		IDxcBlobUtf16* shaderNamePtr = nullptr;
		IDxcBlob* shaderPtr = nullptr;
		hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderPtr), &shaderNamePtr);
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}
		RenderResourcePtr<IDxcBlobUtf16> shaderName = shaderNamePtr;
		shader = shaderPtr;

		outShader.BytecodeLength = shader->GetBufferSize();
		outShader.pShaderBytecode = shader->GetBufferPointer();

		return true;
	}
}
