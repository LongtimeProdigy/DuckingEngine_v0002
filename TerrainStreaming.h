#pragma once

#include "TerrainData.h"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace DK
{
    struct TileRequest
    {
        uint32 _x = 0;
        uint32 _z = 0;
        uint32 _lod = 0;
        dk_inline uint64 key() const
        {
            return (uint64(_lod) << 40) | (uint64(_z) << 20) | _x;
        }
        dk_inline bool operator==(const TileRequest& rhs) const
        {
            return _x == rhs._x && _z == rhs._z && _lod == rhs._lod;
        }
    };
    struct TilePage
    {
        TileRequest _request;
        MipTile _tile;
        bool _loaded = false;
    };

    DKVector<uint32_t> selectTiles(const ArchiveHeader& header, double cameraX, double cameraZ, float radius, uint32_t budget);

    uint32 overviewLod(const ArchiveHeader& header, uint32 maxCells);
    DKVector<TileRequest> selectPages(const ArchiveHeader& header, double cameraX, double cameraZ, float detailRadius, uint32 tailLod, uint32 budgetPerLod);

    class Streamer
    {
    public:
        ~Streamer();
        bool start(const Archive& archive, uint32_t tailLod, uint32_t capacity = 64);
        void stop();
        void request(const DKVector<TileRequest>& tiles);
        bool takeReady(TilePage& page);
    private:
        void run();
        const Archive* _archive = nullptr;
        uint32_t _tailLod = 0;
        uint32_t _capacity = 64;
        bool _stopping = true;
        std::thread _worker;
        std::mutex _mutex;
        std::condition_variable _wake;
        DKDeque<TileRequest> _requests;
        DKDeque<TilePage> _ready;
        DKHashSet<uint64> _pending;
    };
}
