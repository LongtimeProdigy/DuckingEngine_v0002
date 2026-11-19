#pragma once

#include "TerrainWorld.h"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace DK
{
    struct PageRequest
    {
        uint32 _x = 0;
        uint32 _z = 0;
        uint32 _lod = 0;
        uint32 _sectionId = 0;
        dk_inline bool operator==(const PageRequest& rhs) const
        {
            return _x == rhs._x && _z == rhs._z && _lod == rhs._lod && _sectionId == rhs._sectionId;
        }
    };

    struct PageRequestHash
    {
        dk_inline size_t operator()(const PageRequest& request) const
        {
            uint64 hash = request._sectionId;
            hash = (hash ^ request._lod) * 1099511628211ull;
            hash = (hash ^ request._z) * 1099511628211ull;
            return size_t((hash ^ request._x) * 1099511628211ull);
        }
    };

    struct PageLoadResult
    {
        PageRequest _request;
        MipTile _tile;
        bool _loaded = false;
    };

    struct SectionLoadResult
    {
        uint32 _sectionId = 0;
        TerrainOverview _overview;
        bool _loaded = false;
    };

    // 카메라 좌표를 Section 격자 좌표로 변환하고, 중심 Section과 주변 8개를 선택
    DKVector<uint32> selectSections(const TerrainWorld& world, double cameraX, double cameraZ);
    // 선택한 Section 목록을 받아, 그 안에서 LOD0·1·2 Page를 각각 선택
    DKVector<PageRequest> selectPages(const TerrainWorld& world, const DKVector<uint32>& sections, double cameraX, double cameraZ, float detailRadius, uint32 budgetPerLod);

    class Streamer
    {
    public:
        ~Streamer();
        bool start(const TerrainWorld& world, uint32 capacity = 64);
        void stop();
        void request(const DKVector<uint32>& sections, const DKVector<uint32>& missingSections, const DKVector<PageRequest>& pageRequests);
        bool takeReady(PageLoadResult& page);
        bool takeReady(SectionLoadResult& page);
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
        DKDeque<PageRequest> _requests;
        DKDeque<SectionLoadResult> _readySections;
        DKDeque<PageLoadResult> _ready;
        DKHashSet<PageRequest, PageRequestHash> _pending;
        bool _residencyChanged = false;
        uint32 _loadingSectionId = ~0u;
    };
}
