#include "CommonLog.h"
#include "TerrainWorld.h"
#include "FileUtil.h"
#include "StringUtil.h"
#include "lib/tinyXML/tinyxml.h"

#include <algorithm>
#include <cmath>

namespace DK
{
    bool TerrainWorld::open(const DKString& path)
    {
        *this = {};
        FileReader reader(path);
        DKString text;
        if (reader.isOpen() == false || reader.readText(text) == false)
            return false;

        TiXmlDocument document;
        document.Parse(text.c_str());
        const TiXmlElement* root = document.FirstChildElement("TerrainWorld");
        uint32 version = 0;
        if (document.Error() || root == nullptr || root->QueryUnsignedAttribute("Version", &version) != TIXML_SUCCESS || version != 1 ||
            root->QueryUnsignedAttribute("Width", &_header._width) != TIXML_SUCCESS || root->QueryUnsignedAttribute("Height", &_header._height) != TIXML_SUCCESS ||
            root->QueryUnsignedAttribute("ChunkCells", &_header._chunkCellCountPerSide) != TIXML_SUCCESS || root->QueryUnsignedAttribute("SectionCells", &_sectionCellCountPerSide) != TIXML_SUCCESS ||
            root->QueryUnsignedAttribute("OverviewLod", &_overviewLod) != TIXML_SUCCESS || root->QueryFloatAttribute("SampleSpacing", &_header._sampleWorldSpacing) != TIXML_SUCCESS ||
            root->QueryFloatAttribute("HeightOffset", &_header._heightOffset) != TIXML_SUCCESS || root->QueryFloatAttribute("HeightScale", &_header._heightScale) != TIXML_SUCCESS)
        {
            DK_ASSERT_LOG(false, "Invalid terrain world manifest: %s", path.c_str());
            return false;
        }
#if defined(_DK_DEBUG_)
        if (_header._width < 2 || _header._height < 2 || _header._width > 0x7fffffffu || _header._height > 0x7fffffffu ||
            _header._chunkCellCountPerSide < 2 || _header._chunkCellCountPerSide > 512 || (_header._chunkCellCountPerSide & (_header._chunkCellCountPerSide - 1u)) != 0 ||
            _sectionCellCountPerSide < _header._chunkCellCountPerSide || _sectionCellCountPerSide > 524288 || (_sectionCellCountPerSide & (_sectionCellCountPerSide - 1u)) != 0 ||
            _overviewLod == 0 || _overviewLod >= mipCount(_header._chunkCellCountPerSide) || (uint64(_header._chunkCellCountPerSide) << (_overviewLod - 1)) > _sectionCellCountPerSide ||
            std::isfinite(_header._sampleWorldSpacing) == false || _header._sampleWorldSpacing <= 0.0f || std::isfinite(_header._heightOffset) == false ||
            std::isfinite(_header._heightScale) == false || _header._heightScale <= 0.0f || std::isfinite(_header._heightOffset + _header._heightScale) == false)
        {
            DK_ASSERT_LOG(false, "Invalid terrain world layout: %s", path.c_str());
            return false;
        }

        if ((_header._width - 1) % _sectionCellCountPerSide != 0 || (_header._height - 1) % _sectionCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Terrain world cell counts must be divisible by the section cell count");
            return false;
        }
#endif

        _sectionCountX = (_header._width - 1) / _sectionCellCountPerSide;
        _sectionCountZ = (_header._height - 1) / _sectionCellCountPerSide;
#if defined(_DK_DEBUG_)
        if (uint64(_sectionCountX) * _sectionCountZ > 1048576)
        {
            DK_ASSERT_LOG(false, "Terrain world section count exceeds the supported range");
            return false;
        }
#endif

        _sections.resize(size_t(_sectionCountX) * _sectionCountZ);
        for (const TiXmlElement* element = root->FirstChildElement("Section"); element != nullptr; element = element->NextSiblingElement("Section"))
        {
            uint32 x = 0;
            uint32 z = 0;
            const char* archive = element->Attribute("Archive");
            if (element->QueryUnsignedAttribute("X", &x) != TIXML_SUCCESS || element->QueryUnsignedAttribute("Z", &z) != TIXML_SUCCESS ||
                x >= _sectionCountX || z >= _sectionCountZ || archive == nullptr || archive[0] == '\0')
            {
                DK_ASSERT_LOG(false, "Invalid terrain section entry");
                return false;
            }

            TerrainSection& section = _sections[size_t(z) * _sectionCountX + x];
#if defined(_DK_DEBUG_)
            if (section._archivePath.empty() == false)
            {
                DK_ASSERT_LOG(false, "Duplicate terrain section entry");
                return false;
            }
#endif

            section._id = z * _sectionCountX + x;
            section._x = x;
            section._z = z;
            section._archivePath = FileUtil::jointPath(FileUtil::parentPath(path), archive);
        }

#if defined(_DK_DEBUG_)
        for (const TerrainSection& section : _sections)
        {
            if (section._archivePath.empty())
            {
                DK_ASSERT_LOG(false, "Terrain world has a missing section entry");
                return false;
            }
        }

#endif
        return true;
    }

#if defined(_DK_DEBUG_)
    bool TerrainWorld::matchesSectionArchive(const ArchiveHeader& header) const
    {
        return header._width == _sectionCellCountPerSide + 1 && header._height == _sectionCellCountPerSide + 1 && header._chunkCellCountPerSide == _header._chunkCellCountPerSide &&
            header._sampleWorldSpacing == _header._sampleWorldSpacing && header._heightOffset == _header._heightOffset && header._heightScale == _header._heightScale;
    }
#endif

    DKVector<uint32> TerrainWorld::selectSections(double cameraX, double cameraZ) const
    {
        const double sectionWorldSize = double(_sectionCellCountPerSide) * _header._sampleWorldSpacing;
        const double sectionX = std::floor(cameraX / sectionWorldSize);
        const double sectionZ = std::floor(cameraZ / sectionWorldSize);
        DKVector<uint32> result;
        if (sectionX < -1.0 || sectionZ < -1.0 || sectionX > double(_sectionCountX) || sectionZ > double(_sectionCountZ))
        {
            return result;
        }
        constexpr int sectionRadius = (TERRAIN_SECTION_SLOT_COUNT_PER_SIDE - 1) / 2;
        for (int z = -sectionRadius; z <= sectionRadius; ++z)
        {
            for (int x = -sectionRadius; x <= sectionRadius; ++x)
            {
                const int64 targetX = int64(sectionX) + x;
                const int64 targetZ = int64(sectionZ) + z;
                if (targetX >= 0 && targetZ >= 0 && targetX < _sectionCountX && targetZ < _sectionCountZ)
                {
                    result.push_back(uint32(targetZ) * _sectionCountX + uint32(targetX));
                }
            }
        }
        std::sort(result.begin(), result.end(), [&](uint32 lhs, uint32 rhs)
        {
            const TerrainSection& a = _sections[lhs];
            const TerrainSection& b = _sections[rhs];
            const double ax = a._x + 0.5 - cameraX / sectionWorldSize;
            const double az = a._z + 0.5 - cameraZ / sectionWorldSize;
            const double bx = b._x + 0.5 - cameraX / sectionWorldSize;
            const double bz = b._z + 0.5 - cameraZ / sectionWorldSize;
            return ax * ax + az * az < bx * bx + bz * bz;
        });
        return result;
    }

    bool splitTerrainWorld(const DKString& archivePath, const DKString& directoryPath, uint32 sectionCellCountPerSide)
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
        if ((header._width - 1) % sectionCellCountPerSide != 0 || (header._height - 1) % sectionCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Terrain world cell counts must be divisible by the section cell count");
            return false;
        }
#endif
        const DKString manifestPath = FileUtil::jointPath(directoryPath, "terrain.xml");
        if (FileUtil::exist(directoryPath) || FileUtil::createDirectory(directoryPath) == false)
        {
            DK_ASSERT_LOG(false, "Terrain world output must be a new directory");
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
        TiXmlDocument document;
        TiXmlElement* root = new TiXmlElement("TerrainWorld");
        document.LinkEndChild(root);
        root->SetAttribute("Version", 1);
        root->SetAttribute("Width", StringUtil::toString(header._width).c_str());
        root->SetAttribute("Height", StringUtil::toString(header._height).c_str());
        root->SetAttribute("ChunkCells", StringUtil::toString(header._chunkCellCountPerSide).c_str());
        root->SetAttribute("SectionCells", StringUtil::toString(sectionCellCountPerSide).c_str());
        root->SetAttribute("OverviewLod", StringUtil::toString(overviewLod).c_str());
        root->SetAttribute("SampleSpacing", StringUtil::toString(double(header._sampleWorldSpacing)).c_str());
        root->SetAttribute("HeightOffset", StringUtil::toString(double(header._heightOffset)).c_str());
        root->SetAttribute("HeightScale", StringUtil::toString(double(header._heightScale)).c_str());
        const uint32 sectionCountX = (header._width - 1) / sectionCellCountPerSide;
        const uint32 sectionCountZ = (header._height - 1) / sectionCellCountPerSide;
        const uint32 chunkCountPerSectionSide = sectionCellCountPerSide / header._chunkCellCountPerSide;
        for (uint32 z = 0; z < sectionCountZ; ++z)
        {
            for (uint32 x = 0; x < sectionCountX; ++x)
            {
                ArchiveHeader sectionHeader = header;
                sectionHeader._width = sectionCellCountPerSide + 1;
                sectionHeader._height = sectionCellCountPerSide + 1;
                DKVector<Chunk> chunks;
                chunks.reserve(size_t(chunkCountPerSectionSide) * chunkCountPerSectionSide);
                for (uint32 chunkZ = 0; chunkZ < chunkCountPerSectionSide; ++chunkZ)
                {
                    for (uint32 chunkX = 0; chunkX < chunkCountPerSectionSide; ++chunkX)
                    {
                        Chunk chunk;
                        if (source.loadChunk(x * chunkCountPerSectionSide + chunkX, z * chunkCountPerSectionSide + chunkZ, chunk) == false)
                        {
                            return false;
                        }
                        chunk._info._x = chunkX;
                        chunk._info._z = chunkZ;
                        chunks.push_back(DK::move(chunk));
                    }
                }
                DKString filename;
                StringUtil::format(filename, "section_%u_%u.dkterrain", x, z);
                if (writeArchive(FileUtil::jointPath(directoryPath, filename), sectionHeader, chunks) == false)
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
}
