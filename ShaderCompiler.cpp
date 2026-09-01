#include "stdafx.h"
#include "ShaderCompiler.h"
#include "RenderModule.h"

namespace DK
{
	const bool ShaderCompiler::compileShader(const char* shaderPath, const char* entry, const ShaderType shaderType, const DKVector<DKString>& defines, IDxcBlob* shader, D3D12_SHADER_BYTECODE& outShader)
	{
		RenderResourcePtr<IDxcUtils> utils(nullptr);
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.getAddress()));
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		RenderResourcePtr<IDxcCompiler3> compiler3(nullptr);
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler3.getAddress()));
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

#ifdef _DK_DEBUG_
		IDxcVersionInfo* versionInfo = nullptr;
		hr = compiler3->QueryInterface(IID_PPV_ARGS(&versionInfo));
		if (SUCCEEDED(hr))
		{
			UINT major = 0;
			UINT minor = 0;
			versionInfo->GetVersion(&major, &minor);
			DK_LOG("DXC Version: %u.%u", major, minor);
			versionInfo->Release();
		}
#endif

		const ScopeString<DK_MAX_PATH> shaderFullPath = GlobalPath::makeResourceFullPath(shaderPath);

		const DKStringW shaderPathW = StringUtil::convertCtoWC(shaderFullPath.c_str());
		const DKStringW shaderEntryW = StringUtil::convertCtoWC(entry);

		RenderResourcePtr<IDxcBlobEncoding> sourceBlob(nullptr);
		hr = utils->LoadFile(shaderPathW.c_str(), nullptr, sourceBlob.getAddress());
		if (FAILED(hr) == true)
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

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
		arguments.push_back(L"-Qstrip_reflect");
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

		RenderResourcePtr<IDxcIncludeHandler> defaultIncludeHandler;
		hr = utils->CreateDefaultIncludeHandler(defaultIncludeHandler.getAddress());
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "IncludeHandler 생성에 실패했습니다. Shader Compiler을 하지 않습니다.");
			return false;
		}

		RenderResourcePtr<IDxcResult> result(nullptr);
		hr = compiler3->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()), defaultIncludeHandler.get(), IID_PPV_ARGS(result.getAddress()));
		if (FAILED(hr))
		{
			RenderResourcePtr<IDxcBlobUtf8> errors(nullptr);
			hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.getAddress()), nullptr);
			if (SUCCEEDED(hr))
				DK_ASSERT_LOG(false, "Shader Compile Error\nPath: %s\nLog: %s", shaderPath, errors->GetStringPointer());

			return false;
		}

		HRESULT status(S_OK);
		hr = result->GetStatus(&status);
		if (FAILED(hr) || FAILED(status))
		{
			RenderResourcePtr<IDxcBlobUtf8> errors(nullptr);
			hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.getAddress()), nullptr);
			const char* test = errors->GetStringPointer();
			DK_ASSERT_LOG(FAILED(hr), "Shader Compile Error\nPath: %s\nLog: %s", shaderPath, test);

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

#ifdef _DK_DEBUG_	//Reflection
		//RenderResourcePtr<IDxcBlob> reflectionData;
		//result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionData.getAddress()), nullptr);
		//DxcBuffer reflectionBuffer;
		//reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
		//reflectionBuffer.Size = reflectionData->GetBufferSize();
		//reflectionBuffer.Encoding = 0;
		//RenderResourcePtr<ID3D12ShaderReflection> shaderReflection;
		//utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(shaderReflection.getAddress()));

		//D3D12_SHADER_DESC shaderDesc;
		//shaderReflection->GetDesc(&shaderDesc);
		//const uint32 cBufferCount = shaderDesc.ConstantBuffers;
		//for (uint32 i = 0; i < cBufferCount; ++i)
		//{
		//	ID3D12ShaderReflectionConstantBuffer* cbReflection = nullptr;
		//	cbReflection = shaderReflection->GetConstantBufferByIndex(i);
		//	D3D12_SHADER_BUFFER_DESC shaderBufferDesc;
		//	cbReflection->GetDesc(&shaderBufferDesc);
		//	DKString shaderBufferName = shaderBufferDesc.Name;
		//}
#endif

		RenderResourcePtr<IDxcBlobUtf16> shaderName = nullptr;
		hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), shaderName.getAddress());
		if (FAILED(hr))
		{
			DK_ASSERT_LOG(false, "");
			return false;
		}

		outShader.BytecodeLength = shader->GetBufferSize();
		outShader.pShaderBytecode = shader->GetBufferPointer();

		return true;
	}
}
