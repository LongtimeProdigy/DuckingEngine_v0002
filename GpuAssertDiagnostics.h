#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#include "lib/dxc_2026_08_11/inc/dxcapi.h"

namespace DK
{
    enum class GpuAssertDataType : uint32_t { Uint4 = 0, Float4 = 1 };

    // Matches Assert.hlsl. The first failing invocation owns the whole record.
    struct GpuAssertRecord
    {
        uint32_t failed = 0;
        uint32_t fileId = 0;
        uint32_t line = 0;
        GpuAssertDataType dataType = GpuAssertDataType::Uint4;
        uint32_t data[4] = {};
    };
    static_assert(sizeof(GpuAssertRecord) == 32, "GPU assert record layout mismatch");

    inline std::mutex& gpuAssertFileMutex()
    {
        static std::mutex mutex;
        return mutex;
    }
    inline std::vector<std::string>& gpuAssertFiles()
    {
        static std::vector<std::string> files;
        return files;
    }
    inline uint32_t registerGpuAssertFile(std::string path)
    {
        for (char& c : path)
            if (c == '\\') c = '/';
        const std::lock_guard<std::mutex> lock(gpuAssertFileMutex());
        auto& files = gpuAssertFiles();
        for (size_t i = 0; i < files.size(); ++i)
            if (files[i] == path) return static_cast<uint32_t>(i + 1);
        files.push_back(std::move(path));
        return static_cast<uint32_t>(files.size());
    }
    inline std::string gpuAssertFilePath(uint32_t fileId)
    {
        const std::lock_guard<std::mutex> lock(gpuAssertFileMutex());
        const auto& files = gpuAssertFiles();
        return fileId > 0 && fileId <= files.size() ? files[fileId - 1] : "<unknown GPU assert file>";
    }
    inline std::string gpuAssertUtf8Path(const wchar_t* path)
    {
        const int length = static_cast<int>(wcslen(path));
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, path, length, nullptr, 0, nullptr, nullptr);
        std::string result(bytes, '\0');
        if (bytes) WideCharToMultiByte(CP_UTF8, 0, path, length, &result[0], bytes, nullptr, nullptr);
        return result;
    }
    inline std::string wrapGpuAssertSource(const std::string& path, const char* source, size_t size)
    {
        const uint32_t fileId = registerGpuAssertFile(path);
        std::string result = "#line 1 \"" + gpuAssertFilePath(fileId) + "\"\n";
        result.append(source, size);
        return result;
    }
    inline std::string formatGpuAssert(const GpuAssertRecord& record)
    {
        char details[256];
        if (record.dataType == GpuAssertDataType::Float4)
        {
            float data[4];
            std::memcpy(data, record.data, sizeof(data));
            std::snprintf(details, sizeof(details), "): GPU assertion failed. float4(%.9g, %.9g, %.9g, %.9g)"
                " [0x%08X, 0x%08X, 0x%08X, 0x%08X]\n",
                data[0], data[1], data[2], data[3],
                record.data[0], record.data[1], record.data[2], record.data[3]);
        }
        else
        {
            std::snprintf(details, sizeof(details), "): GPU assertion failed. uint4(%u, %u, %u, %u)"
                " [0x%08X, 0x%08X, 0x%08X, 0x%08X]\n",
                record.data[0], record.data[1], record.data[2], record.data[3],
                record.data[0], record.data[1], record.data[2], record.data[3]);
        }
        return gpuAssertFilePath(record.fileId) + "(" + std::to_string(record.line) + details;
    }

    // Resolve __FILE__ after DXC has expanded includes and macros. HLSL cannot store
    // strings in the record, so replace the generated filename marker with an ID.
    inline HRESULT preprocessGpuAssertSource(IDxcCompiler3* compiler, const DxcBuffer& input,
        LPCWSTR* arguments, UINT32 argumentCount, IDxcIncludeHandler* includes,
        std::string& output, std::string& errors)
    {
        try
        {
            std::vector<LPCWSTR> preprocessArguments(arguments, arguments + argumentCount);
            preprocessArguments.push_back(L"-P");
            Microsoft::WRL::ComPtr<IDxcResult> result;
            HRESULT hr = compiler->Compile(&input, preprocessArguments.data(), static_cast<UINT32>(preprocessArguments.size()),
                includes, IID_PPV_ARGS(&result));
            if (FAILED(hr)) return hr;
            HRESULT status;
            hr = result->GetStatus(&status);
            if (FAILED(hr)) return hr;
            if (FAILED(status))
            {
                Microsoft::WRL::ComPtr<IDxcBlobUtf8> diagnostics;
                if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&diagnostics), nullptr)) && diagnostics)
                    errors.assign(diagnostics->GetStringPointer(), diagnostics->GetStringLength());
                return status;
            }
            Microsoft::WRL::ComPtr<IDxcBlobUtf8> text;
            hr = result->GetOutput(DXC_OUT_HLSL, IID_PPV_ARGS(&text), nullptr);
            if (FAILED(hr) || !text) return FAILED(hr) ? hr : E_FAIL;
            output.assign(text->GetStringPointer(), text->GetStringLength());
            const std::string marker = "GpuAssertFileId(";
            size_t start = 0;
            while ((start = output.find(marker, start)) != std::string::npos)
            {
                size_t cursor = start + marker.size();
                while (cursor < output.size() && std::isspace(static_cast<unsigned char>(output[cursor]))) ++cursor;
                if (cursor == output.size() || output[cursor++] != '"') return E_INVALIDARG;
                std::string filename;
                bool closed = false;
                while (cursor < output.size())
                {
                    char c = output[cursor++];
                    if (c == '"') { closed = true; break; }
                    if (c == '\\' && cursor < output.size()) c = output[cursor++];
                    filename += c;
                }
                while (cursor < output.size() && std::isspace(static_cast<unsigned char>(output[cursor]))) ++cursor;
                if (!closed || cursor == output.size() || output[cursor++] != ')') return E_INVALIDARG;
                const std::string id = std::to_string(registerGpuAssertFile(filename)) + "u";
                output.replace(start, cursor - start, id);
                start += id.size();
            }
            return S_OK;
        }
        catch (...) { return E_OUTOFMEMORY; }
    }
}
