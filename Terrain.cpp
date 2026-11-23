#include "CommonLog.h"
#include "Terrain.h"
#include "FileUtil.h"
#include "Resource/Material/TerrainShared.h"
#include "lib/tinyXML/tinyxml.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace DK
{
    uint32 mipCount(uint32 chunkCellCountPerSide)
    {
        uint32 count = 1;
        while (chunkCellCountPerSide > 1)
        {
            chunkCellCountPerSide >>= 1;
            ++count;
        }
        return count;
    }

    bool buildArchiveLayout(ArchiveHeader& header, DKVector<ArchiveBlockInfo>& blocks, DKVector<uint32>& lodBlockOffsets)
    {
        if (std::memcmp(header._magic, "DKTRN004", sizeof(header._magic)) != 0 || header._version != 4 || header._headerBytes != sizeof(ArchiveHeader) || header._directoryOffset != sizeof(ArchiveHeader) ||
            header._sampleCountX < 2 || header._sampleCountZ < 2 || header._sampleCountX > 1048576 || header._sampleCountZ > 1048576 ||
            header._chunkCellCountPerSide < 2 || header._chunkCellCountPerSide > 512 || (header._chunkCellCountPerSide & (header._chunkCellCountPerSide - 1)) != 0 ||
            header._overviewLod == 0 || header._overviewLod >= mipCount(header._chunkCellCountPerSide) ||
            std::isfinite(header._sampleWorldSpacing) == false || header._sampleWorldSpacing <= 0 || std::isfinite(header._baseWorldHeight) == false ||
            std::isfinite(header._worldHeightRange) == false || header._worldHeightRange <= 0 || std::isfinite(header._baseWorldHeight + header._worldHeightRange) == false)
        {
            DK_ASSERT_LOG(false, "Terrain archive requires version 4 and a valid heightfield, tile and overview layout");
            return false;
        }

        const uint32 coarsestTileLod0CellCountPerSide = header._chunkCellCountPerSide << (header._overviewLod - 1);
        if ((header._sampleCountX - 1) % coarsestTileLod0CellCountPerSide != 0 || (header._sampleCountZ - 1) % coarsestTileLod0CellCountPerSide != 0 ||
            (header._sampleCountX - 1) % (1u << header._overviewLod) != 0 || (header._sampleCountZ - 1) % (1u << header._overviewLod) != 0)
        {
            DK_ASSERT_LOG(false, "Terrain archive cell counts must divide exactly into streaming tiles and overview samples");
            return false;
        }

        uint64 blockCount = 1;
        lodBlockOffsets.resize(header._overviewLod);
        for (uint32 lod = 0; lod < header._overviewLod; ++lod)
        {
            lodBlockOffsets[lod] = uint32(blockCount);
            const uint32 tileLod0CellCountPerSide = header._chunkCellCountPerSide << lod;
            blockCount += uint64((header._sampleCountX - 1) / tileLod0CellCountPerSide) * ((header._sampleCountZ - 1) / tileLod0CellCountPerSide);
            if (blockCount > 1048576)
            {
                DK_ASSERT_LOG(false, "Terrain archive block count exceeds the supported range");
                return false;
            }
        }

        header._blockCount = uint32(blockCount);
        blocks.clear();
        blocks.reserve(header._blockCount);
        ArchiveBlockInfo overview;
        overview._lod = header._overviewLod;
        overview._sampleCountX = ((header._sampleCountX - 1) >> header._overviewLod) + 1;
        overview._sampleCountZ = ((header._sampleCountZ - 1) >> header._overviewLod) + 1;
        blocks.push_back(overview);
        for (uint32 lod = 0; lod < header._overviewLod; ++lod)
        {
            const uint32 tileLod0CellCountPerSide = header._chunkCellCountPerSide << lod;
            const uint32 tileCountX = (header._sampleCountX - 1) / tileLod0CellCountPerSide;
            const uint32 tileCountZ = (header._sampleCountZ - 1) / tileLod0CellCountPerSide;
            for (uint32 z = 0; z < tileCountZ; ++z)
            {
                for (uint32 x = 0; x < tileCountX; ++x)
                {
                    ArchiveBlockInfo tile;
                    tile._tileLocalIndexX = x;
                    tile._tileLocalIndexZ = z;
                    tile._lod = lod;
                    tile._sampleCountX = header._chunkCellCountPerSide + 1;
                    tile._sampleCountZ = tile._sampleCountX;
                    blocks.push_back(tile);
                }
            }
        }

        uint64 offset = header._directoryOffset + uint64(header._blockCount) * sizeof(ArchiveBlockInfo);
        for (ArchiveBlockInfo& block : blocks)
        {
            const uint64 sampleCount = uint64(block._sampleCountX) * block._sampleCountZ;
            block._heightDataOffset = offset;
            block._normalDataOffset = block._heightDataOffset + sampleCount * sizeof(uint16);
            offset = block._normalDataOffset + sampleCount * sizeof(DKArray<int16, 2>);
        }
        header._fileBytes = offset;
        return true;
    }

    bool Archive::open(const DKString& path)
    {
        _path.clear();
        _header = {};
        _blocks.clear();
        _lodBlockOffsets.clear();
        FileReader reader(path);
        ArchiveHeader header;
        if (reader.isOpen() == false || reader.read(&header, sizeof(header)) == false)
            return false;

        ArchiveHeader expectedHeader = header;
        DKVector<ArchiveBlockInfo> blocks;
        DKVector<uint32> lodBlockOffsets;
        if (buildArchiveLayout(expectedHeader, blocks, lodBlockOffsets) == false)
            return false;
        if (header._blockCount != expectedHeader._blockCount || header._fileBytes != expectedHeader._fileBytes || reader.size() != header._fileBytes)
        {
            DK_ASSERT_LOG(false, "Terrain archive size and block count must match its complete streaming layout");
            return false;
        }

        DKVector<ArchiveBlockInfo> storedBlocks(header._blockCount);
        if (reader.read(storedBlocks.data(), storedBlocks.size() * sizeof(ArchiveBlockInfo)) == false)
            return false;
        for (uint32 index = 0; index < header._blockCount; ++index)
        {
            if (std::memcmp(&storedBlocks[index], &blocks[index], sizeof(ArchiveBlockInfo)) != 0)
            {
                DK_ASSERT_LOG(false, "Terrain block coordinates, LOD, dimensions and offsets must match the cooked streaming layout: %u", index);
                return false;
            }
        }

        _path = path;
        _header = header;
        _blocks = DK::move(blocks);
        _lodBlockOffsets = DK::move(lodBlockOffsets);
        return true;
    }

    bool Archive::loadBlock(const ArchiveBlockInfo& block, TerrainSampleData& out) const
    {
        DK_ASSERT_LOG(isOpen() && block._sampleCountX > 0 && block._sampleCountZ > 0, "Terrain block requires an open archive and a nonempty sample grid");
        const size_t sampleCount = size_t(block._sampleCountX) * block._sampleCountZ;
        DK_ASSERT_LOG(block._normalDataOffset == block._heightDataOffset + sampleCount * sizeof(uint16) && block._normalDataOffset + sampleCount * sizeof(DKArray<int16, 2>) <= _header._fileBytes, "Terrain height and normal arrays must be contiguous and fit inside the archive");
        TerrainSampleData result;
        result._heights.resize(sampleCount);
        result._normals.resize(sampleCount);
        FileReader reader(_path);
        if (reader.isOpen() == false || reader.seek(block._heightDataOffset) == false ||
            reader.read(result._heights.data(), sampleCount * sizeof(uint16)) == false || reader.read(result._normals.data(), sampleCount * sizeof(DKArray<int16, 2>)) == false)
        {
            return false;
        }
        out = DK::move(result);
        return true;
    }

    bool Archive::loadVirtualTile(uint32 tileLocalIndexX, uint32 tileLocalIndexZ, uint32 lod, MipTile& out) const
    {
        DK_ASSERT_LOG(isOpen() && lod < _header._overviewLod, "Terrain virtual tile requires an open archive and a detail LOD");
        const uint32 tileLod0CellCountPerSide = _header._chunkCellCountPerSide << lod;
        const uint32 tileCountX = (_header._sampleCountX - 1) / tileLod0CellCountPerSide;
        const uint32 tileCountZ = (_header._sampleCountZ - 1) / tileLod0CellCountPerSide;
        DK_ASSERT_LOG(tileLocalIndexX < tileCountX && tileLocalIndexZ < tileCountZ, "Terrain virtual tile must fit inside its section");
        const uint32 blockIndex = _lodBlockOffsets[lod] + tileLocalIndexZ * tileCountX + tileLocalIndexX;
        DK_ASSERT_LOG(blockIndex < _blocks.size(), "Terrain virtual tile block index must fit inside the directory");
        const ArchiveBlockInfo& block = _blocks[blockIndex];
        DK_ASSERT_LOG(block._tileLocalIndexX == tileLocalIndexX && block._tileLocalIndexZ == tileLocalIndexZ && block._lod == lod && block._sampleCountX == _header._chunkCellCountPerSide + 1 && block._sampleCountZ == block._sampleCountX, "Terrain virtual tile metadata must match its request");
        TerrainSampleData grid;
        if (loadBlock(block, grid) == false)
            return false;
        MipTile tile;
        tile._tileLocalIndexX = tileLocalIndexX;
        tile._tileLocalIndexZ = tileLocalIndexZ;
        tile._lod = lod;
        tile._sampleCountPerSide = block._sampleCountX;
        tile._heights = DK::move(grid._heights);
        tile._normals = DK::move(grid._normals);
        out = DK::move(tile);
        return true;
    }

    bool Archive::loadOverview(TerrainSampleData& out) const
    {
        DK_ASSERT_LOG(isOpen(), "Terrain overview requires an open archive");
        const ArchiveBlockInfo& block = _blocks.front();
        DK_ASSERT_LOG(block._lod == _header._overviewLod && block._sampleCountX == ((_header._sampleCountX - 1) >> _header._overviewLod) + 1 && block._sampleCountZ == ((_header._sampleCountZ - 1) >> _header._overviewLod) + 1, "Terrain overview block must cover the entire section");
        return loadBlock(block, out);
    }

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
            root->QueryUnsignedAttribute("Width", &_settings._sampleCountX) != TIXML_SUCCESS || root->QueryUnsignedAttribute("Height", &_settings._sampleCountZ) != TIXML_SUCCESS ||
            root->QueryUnsignedAttribute("ChunkCells", &_settings._chunkCellCountPerSide) != TIXML_SUCCESS || root->QueryUnsignedAttribute("SectionCells", &_sectionCellCountPerSide) != TIXML_SUCCESS ||
            root->QueryUnsignedAttribute("OverviewLod", &_overviewLod) != TIXML_SUCCESS || root->QueryFloatAttribute("SampleSpacing", &_settings._sampleWorldSpacing) != TIXML_SUCCESS ||
            root->QueryFloatAttribute("HeightOffset", &_settings._baseWorldHeight) != TIXML_SUCCESS || root->QueryFloatAttribute("HeightScale", &_settings._worldHeightRange) != TIXML_SUCCESS)
        {
            DK_ASSERT_LOG(false, "Invalid terrain world manifest: %s", path.c_str());
            return false;
        }
        if ((root->Attribute("MinimumSampleX") != nullptr && root->QueryIntAttribute("MinimumSampleX", &_minimumSampleX) != TIXML_SUCCESS) || (root->Attribute("MinimumSampleZ") != nullptr && root->QueryIntAttribute("MinimumSampleZ", &_minimumSampleZ) != TIXML_SUCCESS))
        {
            DK_ASSERT_LOG(false, "Terrain minimum sample coordinates must be signed integers");
            return false;
        }
#if defined(_DK_DEBUG_)
        if (_settings._sampleCountX < 2 || _settings._sampleCountZ < 2 || _settings._sampleCountX > 0x7fffffffu || _settings._sampleCountZ > 0x7fffffffu ||
            _settings._chunkCellCountPerSide < 2 || _settings._chunkCellCountPerSide > 512 || (_settings._chunkCellCountPerSide & (_settings._chunkCellCountPerSide - 1u)) != 0 ||
            _sectionCellCountPerSide < _settings._chunkCellCountPerSide || _sectionCellCountPerSide > 524288 || (_sectionCellCountPerSide & (_sectionCellCountPerSide - 1u)) != 0 ||
            _overviewLod == 0 || _overviewLod >= mipCount(_settings._chunkCellCountPerSide) || (uint64(_settings._chunkCellCountPerSide) << (_overviewLod - 1)) > _sectionCellCountPerSide ||
            std::isfinite(_settings._sampleWorldSpacing) == false || _settings._sampleWorldSpacing <= 0.0f || std::isfinite(_settings._baseWorldHeight) == false ||
            std::isfinite(_settings._worldHeightRange) == false || _settings._worldHeightRange <= 0.0f || std::isfinite(_settings._baseWorldHeight + _settings._worldHeightRange) == false)
        {
            DK_ASSERT_LOG(false, "Invalid terrain world layout: %s", path.c_str());
            return false;
        }

        if ((_settings._sampleCountX - 1) % _sectionCellCountPerSide != 0 || (_settings._sampleCountZ - 1) % _sectionCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Terrain world cell counts must be divisible by the section cell count");
            return false;
        }
#endif

        if (_sectionCellCountPerSide % (_settings._chunkCellCountPerSide << (_overviewLod - 1)) != 0 || _sectionCellCountPerSide % (1u << _overviewLod) != 0)
        {
            DK_ASSERT_LOG(false, "Terrain section cell count must be divisible by the coarsest streaming tile cell count and overview sample step");
            return false;
        }

        if (_minimumSampleX % int32(_sectionCellCountPerSide) != 0 || _minimumSampleZ % int32(_sectionCellCountPerSide) != 0 || int64(_minimumSampleX) + _settings._sampleCountX - 1 > 0x7fffffffll || int64(_minimumSampleZ) + _settings._sampleCountZ - 1 > 0x7fffffffll)
        {
            DK_ASSERT_LOG(false, "Terrain minimum sample coordinates must align to sections and the maximum coordinates must fit signed shader integers");
            return false;
        }
        _sectionCountX = (_settings._sampleCountX - 1) / _sectionCellCountPerSide;
        _sectionCountZ = (_settings._sampleCountZ - 1) / _sectionCellCountPerSide;
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

            section._sectionId = z * _sectionCountX + x;
            section._sectionIndexX = x;
            section._sectionIndexZ = z;
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
        return header._sampleCountX == _sectionCellCountPerSide + 1 && header._sampleCountZ == _sectionCellCountPerSide + 1 && header._chunkCellCountPerSide == _settings._chunkCellCountPerSide &&
            header._overviewLod == _overviewLod && header._sampleWorldSpacing == _settings._sampleWorldSpacing && header._baseWorldHeight == _settings._baseWorldHeight && header._worldHeightRange == _settings._worldHeightRange;
    }
#endif

    struct RankedTerrainTile
    {
        TileRequest _request;
        double _distanceSquared = 0;
    };

    DKVector<uint32> selectSections(const TerrainWorld& world, double cameraX, double cameraZ)
    {
        static_assert(TERRAIN_SECTION_SLOT_COUNT_PER_SIDE > 0 && TERRAIN_SECTION_SLOT_COUNT_PER_SIDE % 2 == 1, "Terrain section residency must have a center slot");
        DK_ASSERT_LOG(std::isfinite(cameraX) && std::isfinite(cameraZ), "Terrain section selection requires finite camera coordinates");
        DK_ASSERT_LOG(world._sectionCellCountPerSide > 0 && (world._settings._sampleCountX - 1) % world._sectionCellCountPerSide == 0 && (world._settings._sampleCountZ - 1) % world._sectionCellCountPerSide == 0 && world._sectionCountX == (world._settings._sampleCountX - 1) / world._sectionCellCountPerSide && world._sectionCountZ == (world._settings._sampleCountZ - 1) / world._sectionCellCountPerSide && world._sections.size() == size_t(world._sectionCountX) * world._sectionCountZ, "Terrain section selection requires the loaded uniform world grid");
        const double sectionWorldSize = double(world._sectionCellCountPerSide) * world._settings._sampleWorldSpacing;
        const double cameraRelativeToTerrainMinimumWorldX = cameraX - double(world._minimumSampleX) * world._settings._sampleWorldSpacing;
        const double cameraRelativeToTerrainMinimumWorldZ = cameraZ - double(world._minimumSampleZ) * world._settings._sampleWorldSpacing;
        const double cameraSectionIndexX = std::floor(cameraRelativeToTerrainMinimumWorldX / sectionWorldSize);
        const double cameraSectionIndexZ = std::floor(cameraRelativeToTerrainMinimumWorldZ / sectionWorldSize);
        DKVector<uint32> result;
        if (cameraSectionIndexX < -1.0 || cameraSectionIndexZ < -1.0 || cameraSectionIndexX > double(world._sectionCountX) || cameraSectionIndexZ > double(world._sectionCountZ))
            return result;

        constexpr int sectionRadius = (TERRAIN_SECTION_SLOT_COUNT_PER_SIDE - 1) / 2;
        for (int z = -sectionRadius; z <= sectionRadius; ++z)
        {
            for (int x = -sectionRadius; x <= sectionRadius; ++x)
            {
                const int64 targetX = int64(cameraSectionIndexX) + x;
                const int64 targetZ = int64(cameraSectionIndexZ) + z;
                if (targetX >= 0 && targetZ >= 0 && targetX < world._sectionCountX && targetZ < world._sectionCountZ)
                    result.push_back(uint32(targetZ) * world._sectionCountX + uint32(targetX));
            }
        }

        std::sort(result.begin(), result.end(), [&](uint32 lhs, uint32 rhs)
        {
            const TerrainSection& a = world._sections[lhs];
            const TerrainSection& b = world._sections[rhs];
            const double ax = a._sectionIndexX + 0.5 - cameraRelativeToTerrainMinimumWorldX / sectionWorldSize;
            const double az = a._sectionIndexZ + 0.5 - cameraRelativeToTerrainMinimumWorldZ / sectionWorldSize;
            const double bx = b._sectionIndexX + 0.5 - cameraRelativeToTerrainMinimumWorldX / sectionWorldSize;
            const double bz = b._sectionIndexZ + 0.5 - cameraRelativeToTerrainMinimumWorldZ / sectionWorldSize;
            return ax * ax + az * az < bx * bx + bz * bz;
        });

        return result;
    }


    DKVector<TileRequest> selectTiles(const TerrainWorld& world, const DKVector<uint32>& sections, double cameraX, double cameraZ, float lod0WorldRadius, uint32 tileBudgetPerLod)
    {
        // LOD가 커질수록 Tile 한 장이 담당하는 영역과 검색 반경이 커집니다.
        // 카메라를 중심으로 한 검색 원과 겹치는 Tile를 후보로 모읍니다.
        // 카메라에서 Tile 사각형까지의 최단 거리로 정렬합니다.
        // 모든 선택 Section의 후보를 합쳐 LOD마다 최대 72개를 반환합니다.

        DKVector<TileRequest> result;
        const TerrainWorldSettings& settings = world._settings;
        DK_ASSERT_LOG(world._sectionCellCountPerSide > 0 && (settings._sampleCountX - 1) % world._sectionCellCountPerSide == 0 && (settings._sampleCountZ - 1) % world._sectionCellCountPerSide == 0, "Terrain world cell counts must be divisible by the section cell count");
        DK_ASSERT_LOG(std::isfinite(cameraX) && std::isfinite(cameraZ) && std::isfinite(lod0WorldRadius) && lod0WorldRadius > 0.0f && tileBudgetPerLod > 0, "Terrain tile selection requires finite camera coordinates and a positive radius and budget");
        const double sectionWorldSize = double(world._sectionCellCountPerSide) * settings._sampleWorldSpacing;
        for (uint32 lod = 0; lod < world._overviewLod; ++lod)
        {
            DKVector<RankedTerrainTile> ranked;
            const uint32 tileLod0CellCountPerSide = settings._chunkCellCountPerSide << lod;
            DK_ASSERT_LOG(tileLod0CellCountPerSide > 0 && world._sectionCellCountPerSide % tileLod0CellCountPerSide == 0, "Terrain section cell count must be divisible by each streaming LOD tile cell count");
            const uint32 sectionLodTileCountPerSide = world._sectionCellCountPerSide / tileLod0CellCountPerSide;
            const double tileWorldSize = double(tileLod0CellCountPerSide) * settings._sampleWorldSpacing;
            const double radius = double(lod0WorldRadius) * (1u << (lod + 1));
            for (const uint32 sectionId : sections)
            {
                DK_ASSERT_LOG(sectionId < world._sections.size(), "Terrain tile selection section ID is outside the world");
                const TerrainSection& section = world._sections[sectionId];
                DK_ASSERT_LOG(section._sectionIndexX < world._sectionCountX && section._sectionIndexZ < world._sectionCountZ, "Terrain section coordinates must be inside the world grid");
                const double cameraRelativeToTerrainMinimumWorldX = cameraX - double(world._minimumSampleX) * settings._sampleWorldSpacing;
                const double cameraRelativeToTerrainMinimumWorldZ = cameraZ - double(world._minimumSampleZ) * settings._sampleWorldSpacing;
                const double cameraSectionRelativeWorldX = cameraRelativeToTerrainMinimumWorldX - section._sectionIndexX * sectionWorldSize;
                const double cameraSectionRelativeWorldZ = cameraRelativeToTerrainMinimumWorldZ - section._sectionIndexZ * sectionWorldSize;
                const int minSectionLocalTileIndexX = int(std::clamp(std::floor((cameraSectionRelativeWorldX - radius) / tileWorldSize), 0.0, double(sectionLodTileCountPerSide)));
                const int minSectionLocalTileIndexZ = int(std::clamp(std::floor((cameraSectionRelativeWorldZ - radius) / tileWorldSize), 0.0, double(sectionLodTileCountPerSide)));
                const int maxSectionLocalTileIndexX = int(std::clamp(std::floor((cameraSectionRelativeWorldX + radius) / tileWorldSize), -1.0, double(sectionLodTileCountPerSide - 1)));
                const int maxSectionLocalTileIndexZ = int(std::clamp(std::floor((cameraSectionRelativeWorldZ + radius) / tileWorldSize), -1.0, double(sectionLodTileCountPerSide - 1)));
                for (int sectionLocalTileIndexZ = minSectionLocalTileIndexZ; sectionLocalTileIndexZ <= maxSectionLocalTileIndexZ; ++sectionLocalTileIndexZ)
                {
                    for (int sectionLocalTileIndexX = minSectionLocalTileIndexX; sectionLocalTileIndexX <= maxSectionLocalTileIndexX; ++sectionLocalTileIndexX)
                    {
                        const double left = sectionLocalTileIndexX * tileWorldSize;
                        const double top = sectionLocalTileIndexZ * tileWorldSize;
                        const double right = left + tileWorldSize;
                        const double bottom = top + tileWorldSize;
                        const double dx = cameraSectionRelativeWorldX - std::clamp(cameraSectionRelativeWorldX, left, right);
                        const double dz = cameraSectionRelativeWorldZ - std::clamp(cameraSectionRelativeWorldZ, top, bottom);
                        if (dx * dx + dz * dz <= radius * radius)
                            ranked.push_back({ { uint32(sectionLocalTileIndexX), uint32(sectionLocalTileIndexZ), lod, sectionId }, dx * dx + dz * dz });
                    }
                }
            }

            std::sort(ranked.begin(), ranked.end(), [](const RankedTerrainTile& lhs, const RankedTerrainTile& rhs)
            {
                return lhs._distanceSquared < rhs._distanceSquared;
            });

            for (size_t index = 0; index < std::min<size_t>(tileBudgetPerLod, ranked.size()); ++index)
            {
                result.push_back(ranked[index]._request);
            }
        }

        return result;
    }

    Streamer::~Streamer()
    {
        stop();
    }

    bool Streamer::start(const TerrainWorld& world, uint32 tileRequestCapacity)
    {
        stop();

#if defined(_DK_DEBUG_)
        if (world._sections.empty() || world._overviewLod == 0 || tileRequestCapacity == 0)
        {
            DK_ASSERT_LOG(false, "Invalid terrain streamer configuration");
            return false;
        }
#endif
        if (world._sectionCellCountPerSide == 0 || world._settings._chunkCellCountPerSide == 0 || world._overviewLod == 0 || world._overviewLod >= mipCount(world._settings._chunkCellCountPerSide))
        {
            DK_ASSERT_LOG(false, "Terrain streamer requires a valid section, chunk and LOD layout");
            return false;
        }
        if ((world._settings._sampleCountX - 1) % world._sectionCellCountPerSide != 0 || (world._settings._sampleCountZ - 1) % world._sectionCellCountPerSide != 0 || world._sectionCellCountPerSide % (world._settings._chunkCellCountPerSide << (world._overviewLod - 1)) != 0 || world._sectionCellCountPerSide % (1u << world._overviewLod) != 0 || world._sectionCountX != (world._settings._sampleCountX - 1) / world._sectionCellCountPerSide || world._sectionCountZ != (world._settings._sampleCountZ - 1) / world._sectionCellCountPerSide || world._sections.size() != size_t(world._sectionCountX) * world._sectionCountZ)
        {
            DK_ASSERT_LOG(false, "Terrain streamer requires the loaded uniform world grid");
            return false;
        }
        if (world._minimumSampleX % int32(world._sectionCellCountPerSide) != 0 || world._minimumSampleZ % int32(world._sectionCellCountPerSide) != 0)
        {
            DK_ASSERT_LOG(false, "Terrain streamer minimum sample coordinates must align to the section grid");
            return false;
        }

        _world = world;
        _tileRequestCapacity = tileRequestCapacity;
        _stopping = false;
        _worker = std::thread(&Streamer::run, this);
        return true;
    }

    void Streamer::stop()
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _stopping = true;
        }

        _wake.notify_one();
        if (_worker.joinable())
            _worker.join();

        _wantedSections.clear();
        _sectionRequests.clear();
        _tileRequests.clear();
        _readySections.clear();
        _readyTiles.clear();
        _residencyChanged = false;
        _loadingSectionId = ~0u;
        _loadingTileRequest.reset();
        _world = {};
    }

    void Streamer::request(const DKVector<uint32>& currentSections, const DKVector<uint32>& missingSections, const DKVector<TileRequest>& missingTiles)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_stopping)
            return;

#if defined(_DK_DEBUG_)
        DK_ASSERT_LOG(currentSections.size() <= TERRAIN_SECTION_SLOT_COUNT && missingTiles.size() <= _tileRequestCapacity, "Terrain streaming requests exceed the section or tile cache capacity");
        for (const uint32 sectionId : currentSections)
        {
            DK_ASSERT_LOG(sectionId < _world._sections.size(), "Terrain streaming section ID is outside the world");
        }
#endif

        _residencyChanged = _residencyChanged || _wantedSections != currentSections;
        _wantedSections = currentSections;

        // 이미 로딩완료된 Section들 중 current에 필요 없으면 제거 (사실 이미 상위 콜스텍모든 request를 dequeue해주면 검사할 필요 없음)
        for (DKDeque<SectionLoadResult>::iterator sectionLoadResult = _readySections.begin(); sectionLoadResult != _readySections.end();)
        {
            if (std::find(currentSections.begin(), currentSections.end(), sectionLoadResult->_sectionId) == currentSections.end())
                sectionLoadResult = _readySections.erase(sectionLoadResult);
            else
                ++sectionLoadResult;
        }

        // 이미 로딩완료된 Tile들 중 current에 필요 없으면 제거 (사실 이미 상위 콜스텍모든 request를 dequeue해주면 검사할 필요 없음)
        for (DKDeque<TileLoadResult>::iterator tileLoadResult = _readyTiles.begin(); tileLoadResult != _readyTiles.end();)
        {
            if (std::find(missingTiles.begin(), missingTiles.end(), tileLoadResult->_request) == missingTiles.end())
                tileLoadResult = _readyTiles.erase(tileLoadResult);
            else
                ++tileLoadResult;
        }

        // Section Request 목록을 새롭게 작성
        _sectionRequests.clear();

        // 이미 ready되었거나 로딩중인 section은 제외하고 request에 추가
        for (const uint32 sectionId : missingSections)
        {
            DK_ASSERT_LOG(sectionId < _world._sections.size() && std::find(currentSections.begin(), currentSections.end(), sectionId) != currentSections.end(), "Invalid terrain section request");
            const bool alreadyReady = std::find_if(_readySections.begin(), _readySections.end(), [sectionId](const SectionLoadResult& sectionLoadResult)
            {
                return sectionLoadResult._sectionId == sectionId;
            }) != _readySections.end();
            const bool alreadyLoading = _loadingSectionId == sectionId;
            if (alreadyReady == false && alreadyLoading == false)
                _sectionRequests.push_back(sectionId);
        }

        // Tile Request 목록을 새롭게 작성
        _tileRequests.clear();
        for (const TileRequest& request : missingTiles)
        {
#if defined(_DK_DEBUG_)
            DK_ASSERT_LOG(request._sectionId < _world._sections.size() && request._lod < _world._overviewLod, "Terrain tile request section ID or LOD is outside the world layout");
            const uint32 tileLod0CellCountPerSide = _world._settings._chunkCellCountPerSide << request._lod;
            DK_ASSERT_LOG(_world._sectionCellCountPerSide % tileLod0CellCountPerSide == 0 && request._tileLocalIndexX < _world._sectionCellCountPerSide / tileLod0CellCountPerSide && request._tileLocalIndexZ < _world._sectionCellCountPerSide / tileLod0CellCountPerSide, "Terrain tile request must fit the uniform section grid");
            DK_ASSERT_LOG(std::find(currentSections.begin(), currentSections.end(), request._sectionId) != currentSections.end(), "Terrain tile request must belong to a selected section");
#endif
            const bool alreadyReady = std::find_if(_readyTiles.begin(), _readyTiles.end(), [&request](const TileLoadResult& tileLoadResult)
            {
                return tileLoadResult._request == request;
            }) != _readyTiles.end();
            const bool alreadyLoading = _loadingTileRequest.has_value() && _loadingTileRequest.value() == request;
            if (alreadyReady == false && alreadyLoading == false)
                _tileRequests.push_back(request);
        }

        // task 깨우기
        _wake.notify_one();
    }

    bool Streamer::takeReady(TileLoadResult& tileLoadResult)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_readyTiles.empty())
            return false;

        tileLoadResult = DK::move(_readyTiles.front());
        _readyTiles.pop_front();
        _wake.notify_one();
        return true;
    }

    bool Streamer::takeReady(SectionLoadResult& sectionLoadResult)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_readySections.empty())
            return false;

        sectionLoadResult = DK::move(_readySections.front());
        _readySections.pop_front();
        _wake.notify_one();
        return true;
    }

    void Streamer::run()
    {
        DKHashMap<uint32, Archive> archives;
        for (;;)
        {
            uint32 sectionId = ~0u;
            TileRequest request;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                DK_ASSERT_LOG(_readySections.size() <= TERRAIN_SECTION_SLOT_COUNT, "Section Loading is faster then ready process.");
                DK_ASSERT_LOG(_readyTiles.size() < _tileRequestCapacity, "Tile Loading is faster then ready process.");
                _wake.wait(lock, [&]
                {
                    return _stopping || (_sectionRequests.empty() == false) || (_tileRequests.empty() == false) || _residencyChanged;
                });

                if (_stopping)
                    return;

                _residencyChanged = false;

                for (DKHashMap<uint32, Archive>::iterator archive = archives.begin(); archive != archives.end();)
                {
                    if (std::find(_wantedSections.begin(), _wantedSections.end(), archive->first) == _wantedSections.end())
                        archive = archives.erase(archive);
                    else
                        ++archive;
                }

                // section 우선순위 로딩
                if (_sectionRequests.empty() == false)
                {
                    sectionId = _sectionRequests.front();
                    _sectionRequests.pop_front();
                    _loadingSectionId = sectionId;
                }
                // 로딩할 Section이 없다면 tile로딩
                else if (_sectionRequests.empty() && _tileRequests.empty() == false)
                {
                    request = _tileRequests.front();
                    _tileRequests.pop_front();
                    _loadingTileRequest = request;
                }
                else
                {
                    continue;
                }
            }

            // section 우선순위 로딩
            if (sectionId != ~0u)
            {
                SectionLoadResult sectionLoadResult;
                sectionLoadResult._sectionId = sectionId;
                Archive& archive = archives[sectionId];
                sectionLoadResult._loaded = archive.isOpen() || archive.open(_world._sections[sectionId]._archivePath);
#if defined(_DK_DEBUG_)
                if (sectionLoadResult._loaded)
                {
                    sectionLoadResult._loaded = _world.matchesSectionArchive(archive.header());
                    DK_ASSERT_LOG(sectionLoadResult._loaded, "Terrain section archive does not match the world: %u", sectionId);
                }
#endif
                sectionLoadResult._loaded = sectionLoadResult._loaded && archive.loadOverview(sectionLoadResult._overview);
#if defined(_DK_DEBUG_)
                if (sectionLoadResult._loaded)
                {
                    const uint32 sampleCountPerSide = (_world._sectionCellCountPerSide >> _world._overviewLod) + 1;
                    DK_ASSERT_LOG(sectionLoadResult._overview._heights.size() == size_t(sampleCountPerSide) * sampleCountPerSide && sectionLoadResult._overview._normals.size() == sectionLoadResult._overview._heights.size(), "Loaded terrain section overview does not match the uniform section layout");
                }
                DK_ASSERT_LOG(sectionLoadResult._loaded, "Terrain section load failed: %u", sectionId);
#endif

                std::lock_guard<std::mutex> lock(_mutex);
                _loadingSectionId = ~0u;
                if (_stopping == false && std::find(_wantedSections.begin(), _wantedSections.end(), sectionId) != _wantedSections.end())
                    _readySections.push_back(DK::move(sectionLoadResult));
            }
            // 로딩할 Section이 없다면 tile로딩
            else
            {
                TileLoadResult tileLoadResult;
                tileLoadResult._request = request;
                const DKHashMap<uint32, Archive>::const_iterator archive = archives.find(request._sectionId);
                tileLoadResult._loaded = archive != archives.end() && archive->second.loadVirtualTile(request._tileLocalIndexX, request._tileLocalIndexZ, request._lod, tileLoadResult._tile);
#if defined(_DK_DEBUG_)
                if (tileLoadResult._loaded)
                {
                    const uint32 sampleCountPerSide = _world._settings._chunkCellCountPerSide + 1;
                    DK_ASSERT_LOG(tileLoadResult._tile._tileLocalIndexX == request._tileLocalIndexX && tileLoadResult._tile._tileLocalIndexZ == request._tileLocalIndexZ && tileLoadResult._tile._lod == request._lod && tileLoadResult._tile._sampleCountPerSide == sampleCountPerSide && tileLoadResult._tile._heights.size() == size_t(sampleCountPerSide) * sampleCountPerSide && tileLoadResult._tile._normals.size() == tileLoadResult._tile._heights.size(), "Loaded terrain tile does not match its request or cache slot dimensions");
                }
                DK_ASSERT_LOG(tileLoadResult._loaded, "Terrain tile load failed: section=%u lod=%u tile=(%u,%u)", request._sectionId, request._lod, request._tileLocalIndexX, request._tileLocalIndexZ);
#endif

                std::lock_guard<std::mutex> lock(_mutex);
                _loadingTileRequest.reset();
                if (_stopping == false && std::find(_wantedSections.begin(), _wantedSections.end(), request._sectionId) != _wantedSections.end())
                    _readyTiles.push_back(DK::move(tileLoadResult));
            }
        }
    }
}
