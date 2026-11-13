#include "CommonLog.h"
#include "TerrainStreaming.h"
#include <algorithm>
#include <cmath>

namespace DK
{
    DKVector<uint32> selectTiles(const ArchiveHeader& h, double cameraX, double cameraZ, float radius, uint32 budget)
    {
        const uint32 columns = (h._width - 2) / h._chunkCells + 1;
        const uint32 rows = (h._height - 2) / h._chunkCells + 1;
        const double tileMeters = double(h._chunkCells) * h._sampleSpacing;
        const int minX = static_cast<int>(std::clamp(std::floor((cameraX - radius) / tileMeters), 0.0, double(columns)));
        const int minZ = static_cast<int>(std::clamp(std::floor((cameraZ - radius) / tileMeters), 0.0, double(rows)));
        const int maxX = static_cast<int>(std::clamp(std::floor((cameraX + radius) / tileMeters), -1.0, double(columns - 1)));
        const int maxZ = static_cast<int>(std::clamp(std::floor((cameraZ + radius) / tileMeters), -1.0, double(rows - 1)));
        DKVector<DKPair<double, uint32>> ranked;
        for (int z = minZ; z <= maxZ; ++z)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                const double left = x * tileMeters;
                const double top = z * tileMeters;
                const double right = std::min(left + tileMeters, (h._width - 1) * double(h._sampleSpacing));
                const double bottom = std::min(top + tileMeters, (h._height - 1) * double(h._sampleSpacing));
                const double dx = cameraX - std::clamp(cameraX, left, right);
                const double dz = cameraZ - std::clamp(cameraZ, top, bottom);
                const double distance = dx * dx + dz * dz;
                if (distance <= double(radius) * radius)
                    ranked.emplace_back(distance, uint32(z) * columns + uint32(x));
            }
        }

        std::sort(ranked.begin(), ranked.end());
        DKVector<uint32> selected;
        for (size_t i = 0; i < std::min<size_t>(budget, ranked.size()); ++i)
            selected.push_back(ranked[i].second);

        return selected;
    }

    uint32 overviewLod(const ArchiveHeader& header, uint32 maxCells)
    {
        uint32 lod = 1;
        while (((std::max(header._width, header._height) - 2) >> lod) + 1 > maxCells)
        {
            ++lod;
        }
        return std::max(lod, 3u);
    }

    DKVector<TileRequest> selectPages(const ArchiveHeader& header, double cameraX, double cameraZ, float detailRadius, uint32 tailLod, uint32 budgetPerLod)
    {
        DKVector<TileRequest> result;
        for (uint32 lod = 0; lod < tailLod; ++lod)
        {
            ArchiveHeader level = header;
            level._width = ((header._width - 2) >> lod) + 2;
            level._height = ((header._height - 2) >> lod) + 2;
            level._sampleSpacing *= float(1u << lod);
            const float radius = detailRadius * float(1u << (lod + 1));
            const DKVector<uint32> selected = selectTiles(level, cameraX, cameraZ, radius, budgetPerLod);
            const uint32 columns = (level._width - 2) / level._chunkCells + 1;
            for (const uint32 id : selected)
            {
                result.push_back({ id % columns, id / columns, lod });
            }
        }
        return result;
    }

    Streamer::~Streamer()
    {
        stop();
    }

    bool Streamer::start(const Archive& archive, uint32_t tailLod, uint32_t capacity)
    {
        stop();

        if (archive.isOpen() == false || archive.header()._version != 3 || tailLod == 0 || tailLod > 20 || capacity == 0)
        {
            DK_ASSERT_LOG(false, "Invalid terrain streamer configuration");
            return false;
        }

        _archive = &archive;
        _tailLod = tailLod;
        _capacity = capacity;
        _stopping = false;
        _pending.clear();
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
        _requests.clear();
        _ready.clear();
        _pending.clear();
        _archive = nullptr;
    }

    void Streamer::request(const DKVector<TileRequest>& tiles)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_stopping)
        {
            return;
        }
        for (const TileRequest& request : _requests)
        {
            _pending.erase(request.key());
        }
        _requests.clear();
        for (const TileRequest& request : tiles)
        {
            if (_requests.size() >= _capacity)
            {
                break;
            }
            if (request._lod < _tailLod && _pending.find(request.key()) == _pending.end())
            {
                _pending.insert(request.key());
                _requests.push_back(request);
            }
        }
        _wake.notify_one();
    }

    bool Streamer::takeReady(TilePage& page)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ready.empty())
            return false;

        page = DK::move(_ready.front());
        _ready.pop_front();
        _pending.erase(page._request.key());
        _wake.notify_one();
        return true;
    }

    void Streamer::run()
    {
        for (;;)
        {
            TileRequest request;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _wake.wait(lock, [&] { return _stopping || (_requests.empty() == false && _ready.size() < _capacity); });
                if (_stopping)
                    return;

                request = _requests.front();
                _requests.pop_front();
            }

            TilePage page;
            page._request = request;
            page._loaded = _archive->loadVirtualTile(request._x, request._z, request._lod, page._tile);
            DK_ASSERT_LOG(page._loaded, "Terrain page load failed: lod=%u, tile=(%u, %u)", request._lod, request._x, request._z);

            std::lock_guard<std::mutex> lock(_mutex);
            if (_stopping == false)
                _ready.push_back(DK::move(page));
        }
    }
}
