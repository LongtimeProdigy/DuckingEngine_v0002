#include "CommonLog.h"
#include "FileUtil.h"
#include "StringUtil.h"
#include "Terrain.h"
#include "lib/tinyXML/tinyxml.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <wincodec.h>
#include <wrl/client.h>

namespace DK
{
    struct TerrainHeightfield
    {
        uint32 _sampleCountX = 0;
        uint32 _sampleCountZ = 0;
        DKVector<uint16> _heights;
    };

    static float heightAt(const ArchiveHeader& header, const TerrainHeightfield& source, uint32 x, uint32 z)
    {
        return header._baseWorldHeight + header._worldHeightRange * (float(source._heights[size_t(z) * source._sampleCountX + x]) / 65535.0f);
    }

    static DKArray<int16, 2> normalAt(const ArchiveHeader& header, const TerrainHeightfield& source, uint32 x, uint32 z, uint32 step)
    {
        const uint32 left = x >= step ? x - step : 0;
        const uint32 right = std::min(x + step, source._sampleCountX - 1);
        const uint32 top = z >= step ? z - step : 0;
        const uint32 bottom = std::min(z + step, source._sampleCountZ - 1);
        const double nx = -(double(heightAt(header, source, right, z)) - heightAt(header, source, left, z)) / ((right - left) * double(header._sampleWorldSpacing));
        const double nz = -(double(heightAt(header, source, x, bottom)) - heightAt(header, source, x, top)) / ((bottom - top) * double(header._sampleWorldSpacing));
        const double length = std::sqrt(nx * nx + 1 + nz * nz);
        return DKArray<int16, 2>{ int16(std::lround(nx / length * 32767)), int16(std::lround(nz / length * 32767)) };
    }

    static bool writeArchive(const DKString& path, ArchiveHeader header, const TerrainHeightfield& source, uint32 lod0StartOffsetX = 0, uint32 lod0StartOffsetZ = 0)
    {
        DKVector<ArchiveBlockInfo> blocks;
        DKVector<uint32> lodBlockOffsets;
        if (buildArchiveLayout(header, blocks, lodBlockOffsets) == false)
            return false;
        if (source._sampleCountX < 2 || source._sampleCountZ < 2 || source._heights.size() != uint64(source._sampleCountX) * source._sampleCountZ ||
            uint64(lod0StartOffsetX) + header._sampleCountX > source._sampleCountX || uint64(lod0StartOffsetZ) + header._sampleCountZ > source._sampleCountZ ||
            lod0StartOffsetX % (1u << header._overviewLod) != 0 || lod0StartOffsetZ % (1u << header._overviewLod) != 0)
        {
            DK_ASSERT_LOG(false, "Terrain cooking source must contain the aligned archive sample region");
            return false;
        }

        const DKString temporaryPath = path + ".tmp";
        if (FileUtil::exist(path) || FileUtil::exist(temporaryPath))
        {
            DK_ASSERT_LOG(false, "Terrain cooking output must be a new archive");
            return false;
        }
        FileWriter writer(temporaryPath);
        if (writer.isOpen() == false || writer.write(&header, sizeof(header)) == false || writer.write(blocks.data(), blocks.size() * sizeof(ArchiveBlockInfo)) == false)
            return false;

        for (const ArchiveBlockInfo& block : blocks)
        {
            const uint32 step = 1u << block._lod;
            const uint32 blockLod0StartOffsetX = lod0StartOffsetX + block._tileLocalIndexX * (header._chunkCellCountPerSide << block._lod);
            const uint32 blockLod0StartOffsetZ = lod0StartOffsetZ + block._tileLocalIndexZ * (header._chunkCellCountPerSide << block._lod);
            DK_ASSERT_LOG(uint64(blockLod0StartOffsetX) + uint64(block._sampleCountX - 1) * step < source._sampleCountX && uint64(blockLod0StartOffsetZ) + uint64(block._sampleCountZ - 1) * step < source._sampleCountZ, "Cooked terrain block samples must fit inside the source heightfield");
            DKVector<uint16> heights(size_t(block._sampleCountX) * block._sampleCountZ);
            DKVector<DKArray<int16, 2>> normals(heights.size());
            for (uint32 z = 0; z < block._sampleCountZ; ++z)
            {
                for (uint32 x = 0; x < block._sampleCountX; ++x)
                {
                    const uint32 sourceX = blockLod0StartOffsetX + x * step;
                    const uint32 sourceZ = blockLod0StartOffsetZ + z * step;
                    const size_t destination = size_t(z) * block._sampleCountX + x;
                    heights[destination] = source._heights[size_t(sourceZ) * source._sampleCountX + sourceX];
                    normals[destination] = normalAt(header, source, sourceX, sourceZ, step);
                }
            }
            if (writer.write(heights.data(), heights.size() * sizeof(uint16)) == false || writer.write(normals.data(), normals.size() * sizeof(DKArray<int16, 2>)) == false)
                return false;
        }
        if (writer.close() == false || FileUtil::rename(temporaryPath, path) == false)
            return false;
        return true;
    }

    static bool splitTerrainWorld(const DKString& archivePath, const DKString& directoryPath, uint32 sectionCellCountPerSide)
    {
        Archive source;
        if (source.open(archivePath) == false)
        {
            return false;
        }
        const ArchiveHeader& header = source.header();
#if defined(_DK_DEBUG_)
        if (sectionCellCountPerSide < header._chunkCellCountPerSide || sectionCellCountPerSide > 524288 || (sectionCellCountPerSide & (sectionCellCountPerSide - 1u)) != 0)
        {
            DK_ASSERT_LOG(false, "Section cells must be a power of two and at least the chunk cell count");
            return false;
        }
        if ((header._sampleCountX - 1) % sectionCellCountPerSide != 0 || (header._sampleCountZ - 1) % sectionCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Terrain world cell counts must be divisible by the section cell count");
            return false;
        }
#endif
        if (sectionCellCountPerSide % header._chunkCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Terrain section cell count must be divisible by the chunk cell count");
            return false;
        }
        uint32 overviewLod = 3;
        while ((sectionCellCountPerSide >> overviewLod) > 256)
        {
            ++overviewLod;
        }
        overviewLod = std::min(overviewLod, mipCount(header._chunkCellCountPerSide) - 1);
        while ((uint64(header._chunkCellCountPerSide) << (overviewLod - 1)) > sectionCellCountPerSide)
        {
            --overviewLod;
        }
        if (sectionCellCountPerSide % (header._chunkCellCountPerSide << (overviewLod - 1)) != 0 || sectionCellCountPerSide % (1u << overviewLod) != 0)
        {
            DK_ASSERT_LOG(false, "Terrain section cell count must be divisible by the coarsest streaming tile cell count and overview sample step");
            return false;
        }
        const DKString manifestPath = FileUtil::jointPath(directoryPath, "terrain.xml");
        if (FileUtil::exist(directoryPath) || FileUtil::createDirectory(directoryPath) == false)
        {
            DK_ASSERT_LOG(false, "Terrain world output must be a new directory");
            return false;
        }
        TerrainHeightfield heightfield;
        heightfield._sampleCountX = header._sampleCountX;
        heightfield._sampleCountZ = header._sampleCountZ;
        heightfield._heights.resize(size_t(header._sampleCountX) * header._sampleCountZ);
        const uint32 sourceTileCountX = (header._sampleCountX - 1) / header._chunkCellCountPerSide;
        const uint32 sourceTileCountZ = (header._sampleCountZ - 1) / header._chunkCellCountPerSide;
        for (uint32 z = 0; z < sourceTileCountZ; ++z)
        {
            for (uint32 x = 0; x < sourceTileCountX; ++x)
            {
                MipTile tile;
                if (source.loadVirtualTile(x, z, 0, tile) == false)
                    return false;
                for (uint32 row = 0; row < tile._sampleCountPerSide; ++row)
                {
                    const size_t destination = size_t(z * header._chunkCellCountPerSide + row) * header._sampleCountX + x * header._chunkCellCountPerSide;
                    DK::memcpy(heightfield._heights.data() + destination, tile._heights.data() + size_t(row) * tile._sampleCountPerSide, tile._sampleCountPerSide * sizeof(uint16));
                }
            }
        }
        TiXmlDocument document;
        TiXmlElement* root = new TiXmlElement("TerrainWorld");
        document.LinkEndChild(root);
        root->SetAttribute("Version", 1);
        root->SetAttribute("Width", StringUtil::toString(header._sampleCountX).c_str());
        root->SetAttribute("Height", StringUtil::toString(header._sampleCountZ).c_str());
        root->SetAttribute("ChunkCells", StringUtil::toString(header._chunkCellCountPerSide).c_str());
        root->SetAttribute("SectionCells", StringUtil::toString(sectionCellCountPerSide).c_str());
        root->SetAttribute("OverviewLod", StringUtil::toString(overviewLod).c_str());
        root->SetAttribute("SampleSpacing", StringUtil::toString(double(header._sampleWorldSpacing)).c_str());
        root->SetAttribute("HeightOffset", StringUtil::toString(double(header._baseWorldHeight)).c_str());
        root->SetAttribute("HeightScale", StringUtil::toString(double(header._worldHeightRange)).c_str());
        const uint32 sectionCountX = (header._sampleCountX - 1) / sectionCellCountPerSide;
        const uint32 sectionCountZ = (header._sampleCountZ - 1) / sectionCellCountPerSide;
        for (uint32 z = 0; z < sectionCountZ; ++z)
        {
            for (uint32 x = 0; x < sectionCountX; ++x)
            {
                ArchiveHeader sectionHeader = header;
                sectionHeader._sampleCountX = sectionCellCountPerSide + 1;
                sectionHeader._sampleCountZ = sectionCellCountPerSide + 1;
                sectionHeader._overviewLod = overviewLod;
                DKString filename;
                StringUtil::format(filename, "section_%u_%u.dkterrain", x, z);
                if (writeArchive(FileUtil::jointPath(directoryPath, filename), sectionHeader, heightfield, x * sectionCellCountPerSide, z * sectionCellCountPerSide) == false)
                {
                    return false;
                }
                TiXmlElement* element = new TiXmlElement("Section");
                element->SetAttribute("X", StringUtil::toString(x).c_str());
                element->SetAttribute("Z", StringUtil::toString(z).c_str());
                element->SetAttribute("Archive", filename.c_str());
                root->LinkEndChild(element);
            }
        }
        TiXmlPrinter printer;
        document.Accept(&printer);
        FileWriter writer(manifestPath);
        if (writer.isOpen() == false || writer.write(printer.CStr(), printer.Size()) == false || writer.close() == false)
        {
            return false;
        }
#if defined(_DK_DEBUG_)
        TerrainWorld check;
        return check.open(manifestPath);
#else
        return true;
#endif
    }

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
            FAILED(frame->GetSize(&header._sampleCountX, &header._sampleCountZ)) || header._sampleCountX < 2 || header._sampleCountZ < 2 ||
            header._sampleCountX > 1048576 || header._sampleCountZ > 1048576)
        {
            return false;
        }

        if ((header._sampleCountX - 1) % header._chunkCellCountPerSide != 0 || (header._sampleCountZ - 1) % header._chunkCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Heightmap cell counts must be divisible by the chunk cell count");
            return false;
        }

        const uint64 sampleCount = uint64(header._sampleCountX) * header._sampleCountZ;
        const uint64 byteCount = sampleCount * sizeof(uint16);
        if (byteCount > std::numeric_limits<UINT>::max())
        {
            return false;
        }

        heights.resize(static_cast<size_t>(sampleCount));
        if (IsEqualGUID(format, GUID_WICPixelFormat16bppGray))
        {
            const UINT rowBytes = header._sampleCountX * sizeof(uint16);
            return SUCCEEDED(frame->CopyPixels(nullptr, rowBytes, static_cast<UINT>(byteCount), reinterpret_cast<BYTE*>(heights.data())));
        }

        DKVector<uint8> bytes(static_cast<size_t>(sampleCount));
        if (FAILED(frame->CopyPixels(nullptr, header._sampleCountX, static_cast<UINT>(sampleCount), bytes.data())))
        {
            return false;
        }
        for (size_t sampleIndex = 0; sampleIndex < heights.size(); ++sampleIndex)
        {
            heights[sampleIndex] = uint16(bytes[sampleIndex]) * 257;
        }
        return true;
    }

#if defined(_DK_DEBUG_)
    static bool verifyTerrain(const DKString& path)
    {
        Archive archive;
        if (archive.open(path) == false)
        {
            return false;
        }
        TerrainSampleData overview;
        if (archive.loadOverview(overview) == false)
            return false;
        uint64 sampleCount = overview._heights.size();
        for (uint32 lod = 0; lod < archive.header()._overviewLod; ++lod)
        {
            const uint32 tileLod0CellCountPerSide = archive.header()._chunkCellCountPerSide << lod;
            const uint32 tileCountX = (archive.header()._sampleCountX - 1) / tileLod0CellCountPerSide;
            const uint32 tileCountZ = (archive.header()._sampleCountZ - 1) / tileLod0CellCountPerSide;
            for (uint32 z = 0; z < tileCountZ; ++z)
            {
                for (uint32 x = 0; x < tileCountX; ++x)
                {
                    MipTile tile;
                    if (archive.loadVirtualTile(x, z, lod, tile) == false)
                        return false;
                    sampleCount += tile._heights.size();
                }
            }
        }
        std::printf("Verified v%u, %u x %u samples, %u blocks, %llu stored samples, %llu bytes.\n", archive.header()._version, archive.header()._sampleCountX, archive.header()._sampleCountZ, archive.header()._blockCount, sampleCount, archive.header()._fileBytes);
        return true;
    }
#endif

    static int cookTerrain(int argumentCount, wchar_t* arguments[])
    {
        if (argumentCount == 2 && DKStringW(arguments[1]) == L"--help")
        {
            std::puts("TerrainCooker.exe <8-bit or 16-bit grayscale image> <output.dkterrain> [sampleWorldSpacing baseWorldHeight worldHeightRange chunkCellCountPerSide [overviewLod]]");
            std::puts("Defaults: sampleWorldSpacing=0.5, baseWorldHeight=-16, worldHeightRange=128, chunkCellCountPerSide=128, overviewLod=3");
            std::puts("TerrainCooker.exe --verify <archive.dkterrain> (Debug build only)");
            std::puts("TerrainCooker.exe --verify-world <terrain.xml> (Debug build only)");
            std::puts("TerrainCooker.exe --split <archive.dkterrain> <outputDirectory> <sectionCells>");
            return 0;
        }
        if (argumentCount == 3 && DKStringW(arguments[1]) == L"--verify")
        {
#if defined(_DK_DEBUG_)
            return verifyTerrain(FileUtil::fromWidePath(arguments[2])) ? 0 : 1;
#else
            std::fputs("Terrain validation requires a Debug build.\n", stderr);
            return 1;
#endif
        }
        if (argumentCount == 3 && DKStringW(arguments[1]) == L"--verify-world")
        {
#if defined(_DK_DEBUG_)
            TerrainWorld world;
            if (world.open(FileUtil::fromWidePath(arguments[2])) == false)
                return 1;
            for (const TerrainSection& section : world._sections)
            {
                Archive archive;
                if (archive.open(section._archivePath) == false || world.matchesSectionArchive(archive.header()) == false || verifyTerrain(section._archivePath) == false)
                {
                    DK_ASSERT_LOG(false, "Terrain section must match the world manifest and contain readable streaming blocks");
                    return 1;
                }
            }
            return 0;
#else
            std::fputs("Terrain validation requires a Debug build.\n", stderr);
            return 1;
#endif
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
        if (argumentCount != 3 && argumentCount != 7 && argumentCount != 8)
        {
            std::fputs("Invalid arguments. Run TerrainCooker.exe --help.\n", stderr);
            return 1;
        }

        ArchiveHeader header;
        if (argumentCount >= 7)
        {
            int chunkCellCountPerSide = 0;
            if (StringUtil::parseFloat(StringUtil::convertWCtoC(arguments[3]), header._sampleWorldSpacing) == false ||
                StringUtil::parseFloat(StringUtil::convertWCtoC(arguments[4]), header._baseWorldHeight) == false ||
                StringUtil::parseFloat(StringUtil::convertWCtoC(arguments[5]), header._worldHeightRange) == false ||
                StringUtil::parseInt(StringUtil::convertWCtoC(arguments[6]), chunkCellCountPerSide) == false ||
                header._sampleWorldSpacing <= 0.0f || header._worldHeightRange <= 0.0f ||
                chunkCellCountPerSide < 2 || chunkCellCountPerSide > 512 || (chunkCellCountPerSide & (chunkCellCountPerSide - 1)) != 0)
            {
                std::fputs("Invalid terrain settings. Chunk cells must be a power of two from 2 to 512.\n", stderr);
                return 1;
            }
            header._chunkCellCountPerSide = static_cast<uint32>(chunkCellCountPerSide);
        }
        if (argumentCount == 8)
        {
            int overviewLod = 0;
            if (StringUtil::parseInt(StringUtil::convertWCtoC(arguments[7]), overviewLod) == false || overviewLod <= 0 || uint32(overviewLod) >= mipCount(header._chunkCellCountPerSide))
            {
                DK_ASSERT_LOG(false, "Overview LOD must be positive and fit inside the configured tile LOD range");
                return 1;
            }
            header._overviewLod = uint32(overviewLod);
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
        TerrainHeightfield heightfield;
        const bool heightmapRead = readHeightmap(arguments[1], header, heightfield._heights);
        heightfield._sampleCountX = header._sampleCountX;
        heightfield._sampleCountZ = header._sampleCountZ;
        CoUninitialize();
        if (heightmapRead == false)
        {
            std::fputs("Cannot read an 8-bit or 16-bit grayscale heightmap from the input file.\n", stderr);
            return 1;
        }

        if (FileUtil::createDirectory(FileUtil::parentPath(outputPath)) == false || writeArchive(outputPath, header, heightfield) == false)
        {
            std::fputs("Cannot build or write the terrain archive.\n", stderr);
            return 1;
        }

        std::printf("Wrote v%u, %u x %u samples with overview LOD %u and contiguous detail tiles.\n", header._version, header._sampleCountX, header._sampleCountZ, header._overviewLod);
#if defined(_DK_DEBUG_)
        return verifyTerrain(outputPath) ? 0 : 1;
#else
        return 0;
#endif
    }
}

int wmain(int argumentCount, wchar_t* arguments[])
{
    return DK::cookTerrain(argumentCount, arguments);
}
