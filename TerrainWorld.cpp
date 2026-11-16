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
        {
            return false;
        }
        TiXmlDocument document;
        document.Parse(text.c_str());
        const TiXmlElement* root = document.FirstChildElement("TerrainWorld");
        uint32 version = 0;
        if (document.Error() || root == nullptr || root->QueryUnsignedAttribute("Version", &version) != TIXML_SUCCESS || version != 1 ||
            root->QueryUnsignedAttribute("Width", &_header._width) != TIXML_SUCCESS || root->QueryUnsignedAttribute("Height", &_header._height) != TIXML_SUCCESS ||
            root->QueryUnsignedAttribute("ChunkCells", &_header._chunkCells) != TIXML_SUCCESS || root->QueryUnsignedAttribute("SectionCells", &_sectionCellCountPerSide) != TIXML_SUCCESS ||
            root->QueryUnsignedAttribute("OverviewLod", &_overviewLod) != TIXML_SUCCESS || root->QueryFloatAttribute("SampleSpacing", &_header._sampleSpacing) != TIXML_SUCCESS ||
            root->QueryFloatAttribute("HeightOffset", &_header._heightOffset) != TIXML_SUCCESS || root->QueryFloatAttribute("HeightScale", &_header._heightScale) != TIXML_SUCCESS)
        {
            DK_ASSERT_LOG(false, "Invalid terrain world manifest: %s", path.c_str());
            return false;
        }
        if (_header._width < 2 || _header._height < 2 || _header._width > 0x7fffffffu || _header._height > 0x7fffffffu ||
            _header._chunkCells < 2 || _header._chunkCells > 512 || (_header._chunkCells & (_header._chunkCells - 1u)) != 0 ||
            _sectionCellCountPerSide < _header._chunkCells || _sectionCellCountPerSide > 524288 || (_sectionCellCountPerSide & (_sectionCellCountPerSide - 1u)) != 0 ||
            _overviewLod == 0 || _overviewLod >= mipCount(_header._chunkCells) || (uint64(_header._chunkCells) << (_overviewLod - 1)) > _sectionCellCountPerSide ||
            std::isfinite(_header._sampleSpacing) == false || _header._sampleSpacing <= 0.0f || std::isfinite(_header._heightOffset) == false ||
            std::isfinite(_header._heightScale) == false || _header._heightScale <= 0.0f || std::isfinite(_header._heightOffset + _header._heightScale) == false)
        {
            DK_ASSERT_LOG(false, "Invalid terrain world layout: %s", path.c_str());
            return false;
        }
        _sectionCountX = (_header._width - 2) / _sectionCellCountPerSide + 1;
        _sectionCountZ = (_header._height - 2) / _sectionCellCountPerSide + 1;
        if (uint64(_sectionCountX) * _sectionCountZ > 1048576)
        {
            DK_ASSERT_LOG(false, "Terrain world section count exceeds the supported range");
            return false;
        }
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
            if (section._archivePath.empty() == false)
            {
                DK_ASSERT_LOG(false, "Duplicate terrain section entry");
                return false;
            }
            section._id = z * _sectionCountX + x;
            section._x = x;
            section._z = z;
            section._archivePath = FileUtil::jointPath(FileUtil::parentPath(path), archive);
        }
        for (const TerrainSection& section : _sections)
        {
            if (section._archivePath.empty())
            {
                DK_ASSERT_LOG(false, "Terrain world has a missing section entry");
                return false;
            }
        }
        return true;
    }

    bool TerrainWorld::matchesSectionArchive(uint32 sectionId, const ArchiveHeader& header) const
    {
        DK_ASSERT_LOG(sectionId < _sections.size(), "Invalid terrain section ID");
        const TerrainSection& section = _sections[sectionId];
        const uint32 cellsX = std::min(_sectionCellCountPerSide, _header._width - 1 - section._x * _sectionCellCountPerSide);
        const uint32 cellsZ = std::min(_sectionCellCountPerSide, _header._height - 1 - section._z * _sectionCellCountPerSide);
        return header._width == cellsX + 1 && header._height == cellsZ + 1 && header._chunkCells == _header._chunkCells &&
            header._sampleSpacing == _header._sampleSpacing && header._heightOffset == _header._heightOffset && header._heightScale == _header._heightScale;
    }

    DKVector<uint32> TerrainWorld::selectSections(double cameraX, double cameraZ) const
    {
        const double sectionWorldSize = double(_sectionCellCountPerSide) * _header._sampleSpacing;
        const double sectionX = std::floor(cameraX / sectionWorldSize);
        const double sectionZ = std::floor(cameraZ / sectionWorldSize);
        DKVector<uint32> result;
        if (sectionX < -1.0 || sectionZ < -1.0 || sectionX > double(_sectionCountX) || sectionZ > double(_sectionCountZ))
        {
            return result;
        }
        for (int z = -1; z <= 1; ++z)
        {
            for (int x = -1; x <= 1; ++x)
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
        if (sectionCellCountPerSide < header._chunkCells || sectionCellCountPerSide > 524288 || (sectionCellCountPerSide & (sectionCellCountPerSide - 1u)) != 0)
        {
            DK_ASSERT_LOG(false, "Section cells must be a power of two and at least the chunk cell count");
            return false;
        }
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
        overviewLod = std::min(overviewLod, mipCount(header._chunkCells) - 1);
        while ((uint64(header._chunkCells) << (overviewLod - 1)) > sectionCellCountPerSide)
        {
            --overviewLod;
        }
        TiXmlDocument document;
        TiXmlElement* root = new TiXmlElement("TerrainWorld");
        document.LinkEndChild(root);
        root->SetAttribute("Version", 1);
        root->SetAttribute("Width", StringUtil::toString(header._width).c_str());
        root->SetAttribute("Height", StringUtil::toString(header._height).c_str());
        root->SetAttribute("ChunkCells", StringUtil::toString(header._chunkCells).c_str());
        root->SetAttribute("SectionCells", StringUtil::toString(sectionCellCountPerSide).c_str());
        root->SetAttribute("OverviewLod", StringUtil::toString(overviewLod).c_str());
        root->SetAttribute("SampleSpacing", StringUtil::toString(double(header._sampleSpacing)).c_str());
        root->SetAttribute("HeightOffset", StringUtil::toString(double(header._heightOffset)).c_str());
        root->SetAttribute("HeightScale", StringUtil::toString(double(header._heightScale)).c_str());
        const uint32 sectionCountX = (header._width - 2) / sectionCellCountPerSide + 1;
        const uint32 sectionCountZ = (header._height - 2) / sectionCellCountPerSide + 1;
        for (uint32 z = 0; z < sectionCountZ; ++z)
        {
            for (uint32 x = 0; x < sectionCountX; ++x)
            {
                ArchiveHeader sectionHeader = header;
                sectionHeader._width = std::min(sectionCellCountPerSide, header._width - 1 - x * sectionCellCountPerSide) + 1;
                sectionHeader._height = std::min(sectionCellCountPerSide, header._height - 1 - z * sectionCellCountPerSide) + 1;
                const uint32 columns = (sectionHeader._width - 2) / header._chunkCells + 1;
                const uint32 rows = (sectionHeader._height - 2) / header._chunkCells + 1;
                DKVector<Chunk> chunks;
                chunks.reserve(size_t(columns) * rows);
                for (uint32 chunkZ = 0; chunkZ < rows; ++chunkZ)
                {
                    for (uint32 chunkX = 0; chunkX < columns; ++chunkX)
                    {
                        Chunk chunk;
                        if (source.loadChunk(x * (sectionCellCountPerSide / header._chunkCells) + chunkX, z * (sectionCellCountPerSide / header._chunkCells) + chunkZ, chunk) == false)
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
        TerrainWorld check;
        return check.open(manifestPath);
    }
}
