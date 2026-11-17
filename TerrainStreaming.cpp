#include "CommonLog.h"
#include "TerrainStreaming.h"
#include <algorithm>
#include <cmath>

namespace DK
{
    struct RankedTerrainPage
    {
        TileRequest _request;
        double _distanceSquared = 0;
    };

    DKVector<TileRequest> selectPages(const TerrainWorld& world, const DKVector<uint32>& sections, double cameraX, double cameraZ, float detailRadius, uint32 budgetPerLod)
    {
        DKVector<TileRequest> result;
        const ArchiveHeader& header = world._header;
        const double sectionWorldSize = double(world._sectionCellCountPerSide) * header._sampleWorldSpacing;
        for (uint32 lod = 0; lod < world._overviewLod; ++lod)
        {
            DKVector<RankedTerrainPage> ranked;
            const uint32 tileCellCountPerSide = header._chunkCellCountPerSide << lod;
            const double tileWorldSize = double(tileCellCountPerSide) * header._sampleWorldSpacing;
            const double radius = double(detailRadius) * (1u << (lod + 1));
            for (const uint32 sectionId : sections)
            {
                const TerrainSection& section = world._sections[sectionId];
                const uint32 cellsX = std::min(world._sectionCellCountPerSide, header._width - 1 - section._x * world._sectionCellCountPerSide);
                const uint32 cellsZ = std::min(world._sectionCellCountPerSide, header._height - 1 - section._z * world._sectionCellCountPerSide);
                const uint32 columns = (cellsX - 1) / tileCellCountPerSide + 1;
                const uint32 rows = (cellsZ - 1) / tileCellCountPerSide + 1;
                const double localX = cameraX - section._x * sectionWorldSize;
                const double localZ = cameraZ - section._z * sectionWorldSize;
                const int minX = int(std::clamp(std::floor((localX - radius) / tileWorldSize), 0.0, double(columns)));
                const int minZ = int(std::clamp(std::floor((localZ - radius) / tileWorldSize), 0.0, double(rows)));
                const int maxX = int(std::clamp(std::floor((localX + radius) / tileWorldSize), -1.0, double(columns - 1)));
                const int maxZ = int(std::clamp(std::floor((localZ + radius) / tileWorldSize), -1.0, double(rows - 1)));
                for (int z = minZ; z <= maxZ; ++z)
                {
                    for (int x = minX; x <= maxX; ++x)
                    {
                        const double left = x * tileWorldSize;
                        const double top = z * tileWorldSize;
                        const double right = std::min(left + tileWorldSize, cellsX * double(header._sampleWorldSpacing));
                        const double bottom = std::min(top + tileWorldSize, cellsZ * double(header._sampleWorldSpacing));
                        const double dx = localX - std::clamp(localX, left, right);
                        const double dz = localZ - std::clamp(localZ, top, bottom);
                        if (dx * dx + dz * dz <= radius * radius)
                        {
                            ranked.push_back({ { uint32(x), uint32(z), lod, sectionId }, dx * dx + dz * dz });
                        }
                    }
                }
            }
            std::sort(ranked.begin(), ranked.end(), [](const RankedTerrainPage& lhs, const RankedTerrainPage& rhs)
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
        if (world._sections.empty() || world._overviewLod == 0 || capacity == 0)
        {
            DK_ASSERT_LOG(false, "Invalid terrain streamer configuration");
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

    void Streamer::request(const DKVector<uint32>& sections, const DKVector<uint32>& missingSections, const DKVector<TileRequest>& tiles)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_stopping)
        {
            return;
        }
        DK_ASSERT_LOG(sections.size() <= 9, "Terrain section residency exceeds 3 x 3");
        _residencyChanged = _residencyChanged || _wantedSections != sections;
        _wantedSections = sections;
        for (DKDeque<SectionPage>::iterator page = _readySections.begin(); page != _readySections.end();)
        {
            if (std::find(sections.begin(), sections.end(), page->_sectionId) == sections.end())
            {
                page = _readySections.erase(page);
            }
            else
            {
                ++page;
            }
        }
        for (DKDeque<TilePage>::iterator page = _ready.begin(); page != _ready.end();)
        {
            if (std::find(tiles.begin(), tiles.end(), page->_request) == tiles.end())
            {
                _pending.erase(page->_request);
                page = _ready.erase(page);
            }
            else
            {
                ++page;
            }
        }
        _sectionRequests.clear();
        for (const uint32 sectionId : missingSections)
        {
            DK_ASSERT_LOG(sectionId < _world._sections.size() && std::find(sections.begin(), sections.end(), sectionId) != sections.end(), "Invalid terrain section request");
            const bool ready = std::find_if(_readySections.begin(), _readySections.end(), [sectionId](const SectionPage& page)
            {
                return page._sectionId == sectionId;
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
        for (const TileRequest& request : tiles)
        {
            if (_requests.size() >= _capacity)
            {
                break;
            }
            if (request._lod < _world._overviewLod && std::find(sections.begin(), sections.end(), request._sectionId) != sections.end() && _pending.insert(request).second)
            {
                _requests.push_back(request);
            }
        }
        _wake.notify_one();
    }

    bool Streamer::takeReady(TilePage& page)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ready.empty())
        {
            return false;
        }
        page = DK::move(_ready.front());
        _ready.pop_front();
        _pending.erase(page._request);
        _wake.notify_one();
        return true;
    }

    bool Streamer::takeReady(SectionPage& page)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_readySections.empty())
        {
            return false;
        }
        page = DK::move(_readySections.front());
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
                if (_sectionRequests.empty() == false && _readySections.size() < 9)
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
                SectionPage page;
                page._sectionId = sectionId;
                Archive& archive = archives[sectionId];
                page._loaded = (archive.isOpen() || archive.open(_world._sections[sectionId]._archivePath)) && _world.matchesSectionArchive(archive.header()) && archive.loadOverview(_world._overviewLod, page._overview);
                DK_ASSERT_LOG(page._loaded, "Terrain section load failed: %u", sectionId);
                std::lock_guard<std::mutex> lock(_mutex);
                _loadingSectionId = ~0u;
                if (_stopping == false && std::find(_wantedSections.begin(), _wantedSections.end(), sectionId) != _wantedSections.end())
                {
                    _readySections.push_back(DK::move(page));
                }
            }
            else
            {
                TilePage page;
                page._request = request;
                const DKHashMap<uint32, Archive>::const_iterator archive = archives.find(request._sectionId);
                page._loaded = archive != archives.end() && archive->second.loadVirtualTile(request._x, request._z, request._lod, page._tile);
                DK_ASSERT_LOG(page._loaded, "Terrain page load failed: section=%u lod=%u tile=(%u,%u)", request._sectionId, request._lod, request._x, request._z);
                std::lock_guard<std::mutex> lock(_mutex);
                if (_stopping == false && std::find(_wantedSections.begin(), _wantedSections.end(), request._sectionId) != _wantedSections.end())
                {
                    _ready.push_back(DK::move(page));
                }
                else
                {
                    _pending.erase(request);
                }
            }
        }
    }
}
