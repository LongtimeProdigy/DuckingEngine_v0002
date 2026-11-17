#include "CommonLog.h"
#include "FileUtil.h"
#include "StringUtil.h"
#include "TerrainData.h"
#include "TerrainWorld.h"

#include <cstdio>
#include <limits>
#include <wincodec.h>
#include <wrl/client.h>

namespace DK
{
    static bool readHeightmap(const wchar_t* path, ArchiveHeader& header, DKVector<uint16>& heights)
    {
        Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()))) ||
            FAILED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf())) ||
            FAILED(decoder->GetFrame(0, frame.GetAddressOf())))
        {
            return false;
        }

        WICPixelFormatGUID format{};
        if (FAILED(frame->GetPixelFormat(&format)) || (IsEqualGUID(format, GUID_WICPixelFormat16bppGray) == 0 && IsEqualGUID(format, GUID_WICPixelFormat8bppGray) == 0) ||
            FAILED(frame->GetSize(&header._width, &header._height)) || header._width < 2 || header._height < 2 ||
            header._width > 1048576 || header._height > 1048576)
        {
            return false;
        }

        const uint64 sampleCount = uint64(header._width) * header._height;
        const uint64 byteCount = sampleCount * sizeof(uint16);
        if (byteCount > std::numeric_limits<UINT>::max())
        {
            return false;
        }

        heights.resize(static_cast<size_t>(sampleCount));
        if (IsEqualGUID(format, GUID_WICPixelFormat16bppGray))
        {
            const UINT rowBytes = header._width * sizeof(uint16);
            return SUCCEEDED(frame->CopyPixels(nullptr, rowBytes, static_cast<UINT>(byteCount), reinterpret_cast<BYTE*>(heights.data())));
        }

        DKVector<uint8> bytes(static_cast<size_t>(sampleCount));
        if (FAILED(frame->CopyPixels(nullptr, header._width, static_cast<UINT>(sampleCount), bytes.data())))
        {
            return false;
        }
        for (size_t sampleIndex = 0; sampleIndex < heights.size(); ++sampleIndex)
        {
            heights[sampleIndex] = uint16(bytes[sampleIndex]) * 257;
        }
        return true;
    }

    static bool verifyTerrain(const DKString& path)
    {
        Archive archive;
        if (archive.open(path) == false)
        {
            return false;
        }
        uint64 sampleCount = 0;
        for (const ChunkInfo& chunkInfo : archive.chunks())
        {
            Chunk chunk;
            if (archive.loadChunk(chunkInfo._x, chunkInfo._z, chunk) == false)
            {
                return false;
            }
            sampleCount += chunk.find(ChunkDataType::Height)->_info._count;
        }
        std::printf("Verified %u x %u samples, %zu chunks, %llu stored samples, %llu bytes.\n", archive.header()._width, archive.header()._height, archive.chunks().size(), sampleCount, archive.header()._fileBytes);
        return true;
    }

    static int cookTerrain(int argumentCount, wchar_t* arguments[])
    {
        if (argumentCount == 2 && DKStringW(arguments[1]) == L"--help")
        {
            std::puts("TerrainCooker.exe <8-bit or 16-bit grayscale image> <output.dkterrain> [sampleSpacing heightOffset heightScale chunkCellCountPerSide]");
            std::puts("Defaults: sampleSpacing=1, heightOffset=-16, heightScale=128, chunkCellCountPerSide=128");
            std::puts("TerrainCooker.exe --verify <archive.dkterrain>");
            std::puts("TerrainCooker.exe --split <archive.dkterrain> <outputDirectory> <sectionCells>");
            return 0;
        }
        if (argumentCount == 3 && DKStringW(arguments[1]) == L"--verify")
        {
            return verifyTerrain(FileUtil::fromWidePath(arguments[2])) ? 0 : 1;
        }
        if (argumentCount == 5 && DKStringW(arguments[1]) == L"--split")
        {
            int sectionCells = 0;
            if (StringUtil::parseInt(StringUtil::convertWCtoC(arguments[4]), sectionCells) == false || sectionCells <= 0)
            {
                DK_ASSERT_LOG(false, "Invalid terrain section cell count");
                return 1;
            }
            return splitTerrainWorld(FileUtil::fromWidePath(arguments[2]), FileUtil::fromWidePath(arguments[3]), uint32(sectionCells)) ? 0 : 1;
        }
        if (argumentCount != 3 && argumentCount != 7)
        {
            std::fputs("Invalid arguments. Run TerrainCooker.exe --help.\n", stderr);
            return 1;
        }

        ArchiveHeader header;
        if (argumentCount == 7)
        {
            int chunkCellCountPerSide = 0;
            if (StringUtil::parseFloat(StringUtil::convertWCtoC(arguments[3]), header._sampleWorldSpacing) == false ||
                StringUtil::parseFloat(StringUtil::convertWCtoC(arguments[4]), header._heightOffset) == false ||
                StringUtil::parseFloat(StringUtil::convertWCtoC(arguments[5]), header._heightScale) == false ||
                StringUtil::parseInt(StringUtil::convertWCtoC(arguments[6]), chunkCellCountPerSide) == false ||
                header._sampleWorldSpacing <= 0.0f || header._heightScale <= 0.0f ||
                chunkCellCountPerSide < 2 || chunkCellCountPerSide > 512 || (chunkCellCountPerSide & (chunkCellCountPerSide - 1)) != 0)
            {
                std::fputs("Invalid terrain settings. Chunk cells must be a power of two from 2 to 512.\n", stderr);
                return 1;
            }
            header._chunkCellCountPerSide = static_cast<uint32>(chunkCellCountPerSide);
        }

        const DKString outputPath = FileUtil::fromWidePath(arguments[2]);
        if (FileUtil::exist(outputPath) || FileUtil::exist(outputPath + ".tmp"))
        {
            std::fputs("The output or its .tmp file already exists. Choose a new output path.\n", stderr);
            return 1;
        }

        if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
        {
            std::fputs("Cannot initialize Windows Imaging Component.\n", stderr);
            return 1;
        }
        DKVector<uint16> heights;
        const bool heightmapRead = readHeightmap(arguments[1], header, heights);
        CoUninitialize();
        if (heightmapRead == false)
        {
            std::fputs("Cannot read an 8-bit or 16-bit grayscale heightmap from the input file.\n", stderr);
            return 1;
        }

        DKVector<Chunk> chunks;
        if (buildHeightfield(header, heights, chunks) == false || FileUtil::createDirectory(FileUtil::parentPath(outputPath)) == false ||
            writeArchive(outputPath, header, chunks) == false)
        {
            std::fputs("Cannot build or write the terrain archive.\n", stderr);
            return 1;
        }

        std::printf("Wrote %u x %u samples in %zu chunks.\n", header._width, header._height, chunks.size());
        return verifyTerrain(outputPath) ? 0 : 1;
    }
}

int wmain(int argumentCount, wchar_t* arguments[])
{
    return DK::cookTerrain(argumentCount, arguments);
}
