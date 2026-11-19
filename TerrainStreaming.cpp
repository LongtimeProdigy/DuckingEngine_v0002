#include "CommonLog.h"
#include "TerrainStreaming.h"
#include <algorithm>
#include <cmath>

namespace DK
{
    struct RankedTerrainTile
    {
        TileRequest _request;
        double _distanceSquared = 0;
    };

    DKVector<uint32> selectSections(const TerrainWorld& world, double cameraX, double cameraZ)
    {
        static_assert(TERRAIN_SECTION_SLOT_COUNT_PER_SIDE > 0 && TERRAIN_SECTION_SLOT_COUNT_PER_SIDE % 2 == 1, "Terrain section residency must have a center slot");
        DK_ASSERT_LOG(std::isfinite(cameraX) && std::isfinite(cameraZ), "Terrain section selection requires finite camera coordinates");
        DK_ASSERT_LOG(world._sectionCellCountPerSide > 0 && (world._header._width - 1) % world._sectionCellCountPerSide == 0 && (world._header._height - 1) % world._sectionCellCountPerSide == 0 && world._sectionCountX == (world._header._width - 1) / world._sectionCellCountPerSide && world._sectionCountZ == (world._header._height - 1) / world._sectionCellCountPerSide && world._sections.size() == size_t(world._sectionCountX) * world._sectionCountZ, "Terrain section selection requires the loaded uniform world grid");
        const double sectionWorldSize = double(world._sectionCellCountPerSide) * world._header._sampleWorldSpacing;
        const double cameraTerrainWorldLocalX = cameraX - double(world._minimumSampleX) * world._header._sampleWorldSpacing;
        const double cameraTerrainWorldLocalZ = cameraZ - double(world._minimumSampleZ) * world._header._sampleWorldSpacing;
        const double cameraSectionLocalX = std::floor(cameraTerrainWorldLocalX / sectionWorldSize);
        const double cameraSectionLocalZ = std::floor(cameraTerrainWorldLocalZ / sectionWorldSize);
        DKVector<uint32> result;
        if (cameraSectionLocalX < -1.0 || cameraSectionLocalZ < -1.0 || cameraSectionLocalX > double(world._sectionCountX) || cameraSectionLocalZ > double(world._sectionCountZ))
            return result;

        constexpr int sectionRadius = (TERRAIN_SECTION_SLOT_COUNT_PER_SIDE - 1) / 2;
        for (int z = -sectionRadius; z <= sectionRadius; ++z)
        {
            for (int x = -sectionRadius; x <= sectionRadius; ++x)
            {
                const int64 targetX = int64(cameraSectionLocalX) + x;
                const int64 targetZ = int64(cameraSectionLocalZ) + z;
                if (targetX >= 0 && targetZ >= 0 && targetX < world._sectionCountX && targetZ < world._sectionCountZ)
                    result.push_back(uint32(targetZ) * world._sectionCountX + uint32(targetX));
            }
        }

        std::sort(result.begin(), result.end(), [&](uint32 lhs, uint32 rhs)
        {
            const TerrainSection& a = world._sections[lhs];
            const TerrainSection& b = world._sections[rhs];
            const double ax = a._x + 0.5 - cameraTerrainWorldLocalX / sectionWorldSize;
            const double az = a._z + 0.5 - cameraTerrainWorldLocalZ / sectionWorldSize;
            const double bx = b._x + 0.5 - cameraTerrainWorldLocalX / sectionWorldSize;
            const double bz = b._z + 0.5 - cameraTerrainWorldLocalZ / sectionWorldSize;
            return ax * ax + az * az < bx * bx + bz * bz;
        });

        return result;
    }


    DKVector<TileRequest> selectTiles(const TerrainWorld& world, const DKVector<uint32>& sections, double cameraX, double cameraZ, float detailRadius, uint32 budgetPerLod)
    {
        // LOD가 커질수록 Tile 한 장이 담당하는 영역과 검색 반경이 커집니다.
        // 카메라를 중심으로 한 검색 원과 겹치는 Tile를 후보로 모읍니다.
        // 카메라에서 Tile 사각형까지의 최단 거리로 정렬합니다.
        // 모든 선택 Section의 후보를 합쳐 LOD마다 최대 72개를 반환합니다.

        DKVector<TileRequest> result;
        const ArchiveHeader& header = world._header;
        DK_ASSERT_LOG(world._sectionCellCountPerSide > 0 && (header._width - 1) % world._sectionCellCountPerSide == 0 && (header._height - 1) % world._sectionCellCountPerSide == 0, "Terrain world cell counts must be divisible by the section cell count");
        DK_ASSERT_LOG(std::isfinite(cameraX) && std::isfinite(cameraZ) && std::isfinite(detailRadius) && detailRadius > 0.0f && budgetPerLod > 0, "Terrain tile selection requires finite camera coordinates and a positive radius and budget");
        const double sectionWorldSize = double(world._sectionCellCountPerSide) * header._sampleWorldSpacing;
        for (uint32 lod = 0; lod < world._overviewLod; ++lod)
        {
            DKVector<RankedTerrainTile> ranked;
            const uint32 tileCellCount = header._chunkCellCountPerSide << lod;
            DK_ASSERT_LOG(tileCellCount > 0 && world._sectionCellCountPerSide % tileCellCount == 0, "Terrain section cell count must be divisible by each streaming LOD tile cell count");
            const uint32 sectionTileCount = world._sectionCellCountPerSide / tileCellCount;
            const double tileWorldSize = double(tileCellCount) * header._sampleWorldSpacing;
            const double radius = double(detailRadius) * (1u << (lod + 1));
            for (const uint32 sectionId : sections)
            {
                DK_ASSERT_LOG(sectionId < world._sections.size(), "Terrain tile selection section ID is outside the world");
                const TerrainSection& section = world._sections[sectionId];
                DK_ASSERT_LOG(section._x < world._sectionCountX && section._z < world._sectionCountZ, "Terrain section coordinates must be inside the world grid");
                const double cameraTerrainWorldLocalX = cameraX - double(world._minimumSampleX) * header._sampleWorldSpacing;
                const double cameraTerrainWorldLocalZ = cameraZ - double(world._minimumSampleZ) * header._sampleWorldSpacing;
                const double cameraSectionLocalX = cameraTerrainWorldLocalX - section._x * sectionWorldSize;
                const double cameraSectionLocalZ = cameraTerrainWorldLocalZ - section._z * sectionWorldSize;
                const int minSectionLocalTileIndexX = int(std::clamp(std::floor((cameraSectionLocalX - radius) / tileWorldSize), 0.0, double(sectionTileCount)));
                const int minSectionLocalTileIndexZ = int(std::clamp(std::floor((cameraSectionLocalZ - radius) / tileWorldSize), 0.0, double(sectionTileCount)));
                const int maxSectionLocalTileIndexX = int(std::clamp(std::floor((cameraSectionLocalX + radius) / tileWorldSize), -1.0, double(sectionTileCount - 1)));
                const int maxSectionLocalTileIndexZ = int(std::clamp(std::floor((cameraSectionLocalZ + radius) / tileWorldSize), -1.0, double(sectionTileCount - 1)));
                for (int sectionLocalTileIndexZ = minSectionLocalTileIndexZ; sectionLocalTileIndexZ <= maxSectionLocalTileIndexZ; ++sectionLocalTileIndexZ)
                {
                    for (int sectionLocalTileIndexX = minSectionLocalTileIndexX; sectionLocalTileIndexX <= maxSectionLocalTileIndexX; ++sectionLocalTileIndexX)
                    {
                        const double left = sectionLocalTileIndexX * tileWorldSize;
                        const double top = sectionLocalTileIndexZ * tileWorldSize;
                        const double right = left + tileWorldSize;
                        const double bottom = top + tileWorldSize;
                        const double dx = cameraSectionLocalX - std::clamp(cameraSectionLocalX, left, right);
                        const double dz = cameraSectionLocalZ - std::clamp(cameraSectionLocalZ, top, bottom);
                        if (dx * dx + dz * dz <= radius * radius)
                            ranked.push_back({ { uint32(sectionLocalTileIndexX), uint32(sectionLocalTileIndexZ), lod, sectionId }, dx * dx + dz * dz });
                    }
                }
            }

            std::sort(ranked.begin(), ranked.end(), [](const RankedTerrainTile& lhs, const RankedTerrainTile& rhs)
            {
                return lhs._distanceSquared < rhs._distanceSquared;
            });

            for (size_t index = 0; index < std::min<size_t>(budgetPerLod, ranked.size()); ++index)
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

    bool Streamer::start(const TerrainWorld& world, uint32 capacity)
    {
        stop();
#if defined(_DK_DEBUG_)
        if (world._sections.empty() || world._overviewLod == 0 || capacity == 0)
        {
            DK_ASSERT_LOG(false, "Invalid terrain streamer configuration");
            return false;
        }
#endif
        if (world._sectionCellCountPerSide == 0 || world._header._chunkCellCountPerSide == 0 || world._overviewLod == 0 || world._overviewLod >= mipCount(world._header._chunkCellCountPerSide))
        {
            DK_ASSERT_LOG(false, "Terrain streamer requires a valid section, chunk and LOD layout");
            return false;
        }
        if ((world._header._width - 1) % world._sectionCellCountPerSide != 0 || (world._header._height - 1) % world._sectionCellCountPerSide != 0 || world._sectionCellCountPerSide % (world._header._chunkCellCountPerSide << (world._overviewLod - 1)) != 0 || world._sectionCellCountPerSide % (1u << world._overviewLod) != 0 || world._sectionCountX != (world._header._width - 1) / world._sectionCellCountPerSide || world._sectionCountZ != (world._header._height - 1) / world._sectionCellCountPerSide || world._sections.size() != size_t(world._sectionCountX) * world._sectionCountZ)
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
        _capacity = capacity;
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
        {
            _worker.join();
        }
        _wantedSections.clear();
        _sectionRequests.clear();
        _requests.clear();
        _readySections.clear();
        _ready.clear();
        _pending.clear();
        _residencyChanged = false;
        _loadingSectionId = ~0u;
        _world = {};
    }

    void Streamer::request(const DKVector<uint32>& sections, const DKVector<uint32>& missingSections, const DKVector<TileRequest>& tileRequests)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_stopping)
        {
            return;
        }
        DK_ASSERT_LOG(sections.size() <= TERRAIN_SECTION_SLOT_COUNT && tileRequests.size() <= _capacity, "Terrain streaming requests exceed the section or tile cache capacity");
        for (const uint32 sectionId : sections)
        {
            DK_ASSERT_LOG(sectionId < _world._sections.size(), "Terrain streaming section ID is outside the world");
        }
        _residencyChanged = _residencyChanged || _wantedSections != sections;
        _wantedSections = sections;
        for (DKDeque<SectionLoadResult>::iterator sectionLoadResult = _readySections.begin(); sectionLoadResult != _readySections.end();)
        {
            if (std::find(sections.begin(), sections.end(), sectionLoadResult->_sectionId) == sections.end())
            {
                sectionLoadResult = _readySections.erase(sectionLoadResult);
            }
            else
            {
                ++sectionLoadResult;
            }
        }
        for (DKDeque<TileLoadResult>::iterator tileLoadResult = _ready.begin(); tileLoadResult != _ready.end();)
        {
            if (std::find(tileRequests.begin(), tileRequests.end(), tileLoadResult->_request) == tileRequests.end())
            {
                _pending.erase(tileLoadResult->_request);
                tileLoadResult = _ready.erase(tileLoadResult);
            }
            else
            {
                ++tileLoadResult;
            }
        }
        _sectionRequests.clear();
        for (const uint32 sectionId : missingSections)
        {
            DK_ASSERT_LOG(sectionId < _world._sections.size() && std::find(sections.begin(), sections.end(), sectionId) != sections.end(), "Invalid terrain section request");
            const bool ready = std::find_if(_readySections.begin(), _readySections.end(), [sectionId](const SectionLoadResult& sectionLoadResult)
            {
                return sectionLoadResult._sectionId == sectionId;
            }) != _readySections.end();
            if (ready == false && _loadingSectionId != sectionId)
            {
                _sectionRequests.push_back(sectionId);
            }
        }
        for (const TileRequest& request : _requests)
        {
            _pending.erase(request);
        }
        _requests.clear();
        for (const TileRequest& request : tileRequests)
        {
            DK_ASSERT_LOG(request._sectionId < _world._sections.size() && request._lod < _world._overviewLod, "Terrain tile request section ID or LOD is outside the world layout");
            const uint32 tileCellCountPerSide = _world._header._chunkCellCountPerSide << request._lod;
            DK_ASSERT_LOG(_world._sectionCellCountPerSide % tileCellCountPerSide == 0 && request._sectionLocalTileIndexX < _world._sectionCellCountPerSide / tileCellCountPerSide && request._sectionLocalTileIndexZ < _world._sectionCellCountPerSide / tileCellCountPerSide, "Terrain tile request must fit the uniform section grid");
            DK_ASSERT_LOG(std::find(sections.begin(), sections.end(), request._sectionId) != sections.end(), "Terrain tile request must belong to a selected section");
            if (_pending.insert(request).second)
            {
                _requests.push_back(request);
            }
        }
        _wake.notify_one();
    }

    bool Streamer::takeReady(TileLoadResult& tileLoadResult)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ready.empty())
            return false;

        tileLoadResult = DK::move(_ready.front());
        _ready.pop_front();
        _pending.erase(tileLoadResult._request);
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
                _wake.wait(lock, [&]
                {
                    return _stopping || (_sectionRequests.empty() == false && _readySections.size() < 9) || (_sectionRequests.empty() && _requests.empty() == false && _ready.size() < _capacity) || _residencyChanged;
                });
                if (_stopping)
                {
                    return;
                }
                _residencyChanged = false;
                for (DKHashMap<uint32, Archive>::iterator archive = archives.begin(); archive != archives.end();)
                {
                    if (std::find(_wantedSections.begin(), _wantedSections.end(), archive->first) == _wantedSections.end())
                    {
                        archive = archives.erase(archive);
                    }
                    else
                    {
                        ++archive;
                    }
                }
                if (_sectionRequests.empty() == false && _readySections.size() < TERRAIN_SECTION_SLOT_COUNT)
                {
                    sectionId = _sectionRequests.front();
                    _sectionRequests.pop_front();
                    _loadingSectionId = sectionId;
                }
                else if (_sectionRequests.empty() && _requests.empty() == false && _ready.size() < _capacity)
                {
                    request = _requests.front();
                    _requests.pop_front();
                }
                else
                {
                    continue;
                }
            }
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
                sectionLoadResult._loaded = sectionLoadResult._loaded && archive.loadOverview(_world._overviewLod, sectionLoadResult._overview);
                if (sectionLoadResult._loaded)
                {
                    const uint32 sampleCountPerSide = (_world._sectionCellCountPerSide >> _world._overviewLod) + 1;
                    DK_ASSERT_LOG(sectionLoadResult._overview._lod == _world._overviewLod && sectionLoadResult._overview._width == sampleCountPerSide && sectionLoadResult._overview._height == sampleCountPerSide && sectionLoadResult._overview._heights.size() == size_t(sampleCountPerSide) * sampleCountPerSide && sectionLoadResult._overview._normals.size() == sectionLoadResult._overview._heights.size(), "Loaded terrain section overview does not match the uniform section layout");
                }
                DK_ASSERT_LOG(sectionLoadResult._loaded, "Terrain section load failed: %u", sectionId);
                std::lock_guard<std::mutex> lock(_mutex);
                _loadingSectionId = ~0u;
                if (_stopping == false && std::find(_wantedSections.begin(), _wantedSections.end(), sectionId) != _wantedSections.end())
                {
                    _readySections.push_back(DK::move(sectionLoadResult));
                }
            }
            else
            {
                TileLoadResult tileLoadResult;
                tileLoadResult._request = request;
                const DKHashMap<uint32, Archive>::const_iterator archive = archives.find(request._sectionId);
                tileLoadResult._loaded = archive != archives.end() && archive->second.loadVirtualTile(request._sectionLocalTileIndexX, request._sectionLocalTileIndexZ, request._lod, tileLoadResult._tile);
                if (tileLoadResult._loaded)
                {
                    const uint32 sampleCountPerSide = _world._header._chunkCellCountPerSide + 1;
                    DK_ASSERT_LOG(tileLoadResult._tile._sectionLocalTileIndexX == request._sectionLocalTileIndexX && tileLoadResult._tile._sectionLocalTileIndexZ == request._sectionLocalTileIndexZ && tileLoadResult._tile._lod == request._lod && tileLoadResult._tile._sampleCountPerSide == sampleCountPerSide && tileLoadResult._tile._heights.size() == size_t(sampleCountPerSide) * sampleCountPerSide && tileLoadResult._tile._normals.size() == tileLoadResult._tile._heights.size(), "Loaded terrain tile does not match its request or cache slot dimensions");
                }
                DK_ASSERT_LOG(tileLoadResult._loaded, "Terrain tile load failed: section=%u lod=%u tile=(%u,%u)", request._sectionId, request._lod, request._sectionLocalTileIndexX, request._sectionLocalTileIndexZ);
                std::lock_guard<std::mutex> lock(_mutex);
                if (_stopping == false && std::find(_wantedSections.begin(), _wantedSections.end(), request._sectionId) != _wantedSections.end())
                {
                    _ready.push_back(DK::move(tileLoadResult));
                }
                else
                {
                    _pending.erase(request);
                }
            }
        }
    }
}
