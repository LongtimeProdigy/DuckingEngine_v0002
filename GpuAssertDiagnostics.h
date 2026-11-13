#pragma once

#include "CommonTypes.h"
#include "StringUtil.h"
#include "FileUtil.h"

#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstring>
#include <mutex>
#include "lib/dxc_2026_08_11/inc/dxcapi.h"

namespace DK
{
    enum class GpuAssertDataType : uint32_t { Uint4 = 0, Float4 = 1 };

    // Matches Assert.hlsl. The first failing invocation owns the whole record.
    struct GpuAssertRecord
    {
        uint32_t _failed = 0;
        uint32_t _fileId = 0;
        uint32_t _sourceLine = 0;
        GpuAssertDataType _dataType = GpuAssertDataType::Uint4;
        uint32_t _data[4] = {};
    };
    static_assert(sizeof(GpuAssertRecord) == 32, "GPU assert record layout mismatch");

    dk_inline std::mutex& gpuAssertFileMutex()
    {
        static std::mutex mutex;
        return mutex;
    }
    dk_inline DKVector<DKString>& gpuAssertFiles()
    {
        static DKVector<DKString> files;
        return files;
    }
    dk_inline uint32_t registerGpuAssertFile(DKString path)
    {
        for (char& c : path)
            if (c == '\\')
                c = '/';
        const std::lock_guard<std::mutex> lock(gpuAssertFileMutex());
        DKVector<DKString>& files = gpuAssertFiles();
        for (size_t i = 0; i < files.size(); ++i)
            if (files[i] == path)
                return static_cast<uint32_t>(i + 1);
        files.push_back(DK::move(path));
        return static_cast<uint32_t>(files.size());
    }
    dk_inline DKString gpuAssertFilePath(uint32_t fileId)
    {
        const std::lock_guard<std::mutex> lock(gpuAssertFileMutex());
        const DKVector<DKString>& files = gpuAssertFiles();
        return fileId > 0 && fileId <= files.size() ? files[fileId - 1] : "<unknown GPU assert file>";
    }
    dk_inline DKString gpuAssertUtf8Path(const wchar_t* path)
    {
        return FileUtil::fromWidePath(path);
    }
    dk_inline DKString wrapGpuAssertSource(const DKString& path, const char* source, size_t size)
    {
        const uint32_t fileId = registerGpuAssertFile(path);
        DKString result = "#line 1 \"" + gpuAssertFilePath(fileId) + "\"\n";
        result.append(source, size);
        return result;
    }
    dk_inline DKString formatGpuAssert(const GpuAssertRecord& record)
    {
        DKString details;
        if (record._dataType == GpuAssertDataType::Float4)
        {
            float data[4];
            DK::memcpy(data, record._data, sizeof(data));
            if (StringUtil::format(details, "): GPU assertion failed. float4(%.9g, %.9g, %.9g, %.9g)"
                " [0x%08X, 0x%08X, 0x%08X, 0x%08X]\n",
                data[0], data[1], data[2], data[3],
                record._data[0], record._data[1], record._data[2], record._data[3]) == false)
            {
                return {};
            }
        }
        else
        {
            if (StringUtil::format(details, "): GPU assertion failed. uint4(%u, %u, %u, %u)"
                " [0x%08X, 0x%08X, 0x%08X, 0x%08X]\n",
                record._data[0], record._data[1], record._data[2], record._data[3],
                record._data[0], record._data[1], record._data[2], record._data[3]) == false)
            {
                return {};
            }
        }
        return gpuAssertFilePath(record._fileId) + "(" + StringUtil::toString(record._sourceLine) + details;
    }

    // Resolve __FILE__ after DXC has expanded includes and macros. HLSL cannot store
    // strings in the record, so replace the generated filename marker with an ID.
    dk_inline HRESULT preprocessGpuAssertSource(IDxcCompiler3* compiler, const DxcBuffer& input,
        LPCWSTR* arguments, UINT32 argumentCount, IDxcIncludeHandler* includes,
        DKString& output, DKString& errors)
    {
        DKVector<LPCWSTR> preprocessArguments(arguments, arguments + argumentCount);
        preprocessArguments.push_back(L"-P");
        Microsoft::WRL::ComPtr<IDxcResult> result;
        HRESULT hr = compiler->Compile(&input, preprocessArguments.data(), static_cast<UINT32>(preprocessArguments.size()),
            includes, IID_PPV_ARGS(&result));
        if (FAILED(hr))
            return hr;
        HRESULT status;
        hr = result->GetStatus(&status);
        if (FAILED(hr))
            return hr;
        if (FAILED(status))
        {
            Microsoft::WRL::ComPtr<IDxcBlobUtf8> diagnostics;
            if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&diagnostics), nullptr)) && diagnostics)
                errors.assign(diagnostics->GetStringPointer(), diagnostics->GetStringLength());
            return status;
        }
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> text;
        hr = result->GetOutput(DXC_OUT_HLSL, IID_PPV_ARGS(&text), nullptr);
        if (FAILED(hr) || text.Get() == nullptr)
            return FAILED(hr) ? hr : E_FAIL;
        output.assign(text->GetStringPointer(), text->GetStringLength());
        const DKString marker = "GpuAssertFileId(";
        size_t start = 0;
        while ((start = output.find(marker, start)) != DKString::npos)
        {
            size_t cursor = start + marker.size();
            while (cursor < output.size() && StringUtil::isWhitespace(output[cursor]))
                ++cursor;
            if (cursor == output.size() || output[cursor++] != '"')
                return E_INVALIDARG;
            DKString filename;
            bool closed = false;
            while (cursor < output.size())
            {
                char c = output[cursor++];
                if (c == '"')
                {
                    closed = true;
                    break;
                }
                if (c == '\\' && cursor < output.size())
                    c = output[cursor++];
                filename += c;
            }
            while (cursor < output.size() && StringUtil::isWhitespace(output[cursor]))
                ++cursor;
            if (closed == false || cursor == output.size() || output[cursor++] != ')')
                return E_INVALIDARG;
            const DKString id = StringUtil::toString(registerGpuAssertFile(filename)) + "u";
            output.replace(start, cursor - start, id);
            start += id.size();
        }
        return S_OK;
    }
}
