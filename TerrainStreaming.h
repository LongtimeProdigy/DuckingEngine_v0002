#pragma once

#include "TerrainWorld.h"
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
        uint32 _sectionId = 0;
        dk_inline bool operator==(const TileRequest& rhs) const
        {
            return _x == rhs._x && _z == rhs._z && _lod == rhs._lod && _sectionId == rhs._sectionId;
        }
    };

    struct TileRequestHash
    {
        dk_inline size_t operator()(const TileRequest& request) const
        {
            uint64 hash = request._sectionId;
            hash = (hash ^ request._lod) * 1099511628211ull;
            hash = (hash ^ request._z) * 1099511628211ull;
            return size_t((hash ^ request._x) * 1099511628211ull);
        }
    };

    struct TilePage
    {
        TileRequest _request;
        MipTile _tile;
        bool _loaded = false;
    };

    struct SectionPage
    {
        uint32 _sectionId = 0;
        TerrainOverview _overview;
        bool _loaded = false;
    };

    DKVector<TileRequest> selectPages(const TerrainWorld& world, const DKVector<uint32>& sections, double cameraX, double cameraZ, float detailRadius, uint32 budgetPerLod);

    class Streamer
    {
    public:
        ~Streamer();
        bool start(const TerrainWorld& world, uint32 capacity = 64);
        void stop();
        void request(const DKVector<uint32>& sections, const DKVector<uint32>& missingSections, const DKVector<TileRequest>& tiles);
        bool takeReady(TilePage& page);
        bool takeReady(SectionPage& page);
    private:
        void run();
        TerrainWorld _world;
        uint32 _capacity = 64;
        bool _stopping = true;
        std::thread _worker;
        std::mutex _mutex;
        std::condition_variable _wake;
        DKVector<uint32> _wantedSections;
        DKDeque<uint32> _sectionRequests;
        DKDeque<TileRequest> _requests;
        DKDeque<SectionPage> _readySections;
        DKDeque<TilePage> _ready;
        DKHashSet<TileRequest, TileRequestHash> _pending;
        bool _residencyChanged = false;
        uint32 _loadingSectionId = ~0u;
    };
}
