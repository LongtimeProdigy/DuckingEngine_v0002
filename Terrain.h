#pragma once

#include "CommonTypes.h"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace DK
{
#pragma pack(push, 1)
    struct ArchiveHeader
    {
        char _magic[8] = { 'D', 'K', 'T', 'R', 'N', '0', '0', '4' };
        uint32 _version = 4;
        uint32 _headerBytes = 64;
        uint32 _sampleCountX = 0;
        uint32 _sampleCountZ = 0;
        uint32 _chunkCellCountPerSide = 128;
        uint32 _blockCount = 0;
        float _sampleWorldSpacing = 0.5f;
        float _baseWorldHeight = -16;
        float _worldHeightRange = 128;
        uint32 _overviewLod = 3;
        uint64 _directoryOffset = 64;
        uint64 _fileBytes = 0;
    };

    struct ArchiveBlockInfo
    {
        uint32 _tileLocalIndexX = 0;
        uint32 _tileLocalIndexZ = 0;
        uint32 _lod = 0;
        uint32 _sampleCountX = 0;
        uint32 _sampleCountZ = 0;
        uint32 _reserved = 0;
        uint64 _heightDataOffset = 0;
        uint64 _normalDataOffset = 0;
    };
#pragma pack(pop)

    static_assert(sizeof(ArchiveHeader) == 64 && sizeof(ArchiveBlockInfo) == 40);
    static_assert(sizeof(uint16) == 2 && sizeof(DKArray<int16, 2>) == 4);
    static_assert(std::is_trivially_copyable_v<ArchiveHeader> && std::is_trivially_copyable_v<ArchiveBlockInfo>);
    static_assert(offsetof(ArchiveHeader, _chunkCellCountPerSide) == 24 && offsetof(ArchiveHeader, _blockCount) == 28);
    static_assert(offsetof(ArchiveHeader, _sampleWorldSpacing) == 32 && offsetof(ArchiveHeader, _overviewLod) == 44);
    static_assert(offsetof(ArchiveHeader, _directoryOffset) == 48 && offsetof(ArchiveHeader, _fileBytes) == 56);
    static_assert(offsetof(ArchiveBlockInfo, _lod) == 8 && offsetof(ArchiveBlockInfo, _sampleCountX) == 12 && offsetof(ArchiveBlockInfo, _sampleCountZ) == 16);
    static_assert(offsetof(ArchiveBlockInfo, _heightDataOffset) == 24 && offsetof(ArchiveBlockInfo, _normalDataOffset) == 32);

    struct MipTile
    {
        uint32 _tileLocalIndexX = 0;
        uint32 _tileLocalIndexZ = 0;
        uint32 _lod = 0;
        uint32 _sampleCountPerSide = 0;
        DKVector<uint16> _heights;
        DKVector<DKArray<int16, 2>> _normals;
    };

    struct TerrainSampleData
    {
        DKVector<uint16> _heights;
        DKVector<DKArray<int16, 2>> _normals;
    };

    class Archive
    {
    public:
        bool open(const DKString& path);
        bool loadVirtualTile(uint32 tileLocalIndexX, uint32 tileLocalIndexZ, uint32 lod, MipTile& out) const;
        bool loadOverview(TerrainSampleData& out) const;
        dk_inline bool isOpen() const
        {
            return _blocks.empty() == false;
        }
        dk_inline const ArchiveHeader& header() const
        {
            return _header;
        }

    private:
        bool loadBlock(const ArchiveBlockInfo& block, TerrainSampleData& out) const;
        DKString _path;
        ArchiveHeader _header;
        DKVector<ArchiveBlockInfo> _blocks;
        DKVector<uint32> _lodBlockOffsets;
    };

    struct TerrainSection
    {
        uint32 _sectionId = 0;        // Section 고유 인덱스: _sectionIndexZ * TerrainWorld::_sectionCountX + _sectionIndexX
        uint32 _sectionIndexX = 0;    // 전체 Section 격자에서 X축 인덱스
        uint32 _sectionIndexZ = 0;    // 전체 Section 격자에서 Z축 인덱스
        DKString _archivePath;       // terrain.xml의 디렉터리를 기준으로 결합한 Section Archive 경로
    };

    struct TerrainWorldSettings
    {
        uint32 _sampleCountX = 0;
        uint32 _sampleCountZ = 0;
        uint32 _chunkCellCountPerSide = 128;
        float _sampleWorldSpacing = 0.5f;
        float _baseWorldHeight = -16;
        float _worldHeightRange = 128;
    };

    struct TerrainWorld
    {
        TerrainWorldSettings _settings;
        uint32 _sectionCellCountPerSide = 0;    // 모든 Section에 공통인 한 변의 셀 수. 샘플 수는 셀 수 + 1이며, 전체 지형의 X/Z 셀 수를 나누어떨어지게 해야 한다.
        uint32 _sectionCountX = 0;             // 전체 지형을 X축으로 나눈 Section 개수
        uint32 _sectionCountZ = 0;             // 전체 지형을 Z축으로 나눈 Section 개수
        int32 _minimumSampleX = 0;            // 전체 지형 X축 최소 샘플 좌표. Section (0, 0)의 X축 시작점
        int32 _minimumSampleZ = 0;            // 전체 지형 Z축 최소 샘플 좌표. Section (0, 0)의 Z축 시작점
        uint32 _overviewLod = 0;               // Section 전체의 저해상도 Height·Normal에 사용하는 LOD. 샘플 간격은 원본의 2^LOD배다.
        DKVector<TerrainSection> _sections;    // 전체 Section의 경로·격자 위치 목록. Z행 안에서 X가 증가하는 순서이며 인덱스는 Section ID와 같다.

        bool open(const DKString& path);
#if defined(_DK_DEBUG_)
        bool matchesSectionArchive(const ArchiveHeader& header) const;
#endif
    };

    struct TileRequest
    {
        uint32 _tileLocalIndexX = 0;          // 해당 Section내부에서 TileIndexX
        uint32 _tileLocalIndexZ = 0;          // 해당 Section내부에서 TileIndexZ
        uint32 _lod = 0;
        uint32 _sectionId = 0;
        dk_inline bool operator==(const TileRequest& rhs) const
        {
            return _tileLocalIndexX == rhs._tileLocalIndexX && _tileLocalIndexZ == rhs._tileLocalIndexZ && _lod == rhs._lod && _sectionId == rhs._sectionId;
        }
    };

    struct TileLoadResult
    {
        TileRequest _request;
        MipTile _tile;
        bool _loaded = false;
    };

    struct SectionLoadResult
    {
        uint32 _sectionId = 0;
        TerrainSampleData _overview;
        bool _loaded = false;
    };

    // 카메라 좌표를 Section 격자 좌표로 변환하고, 중심 Section과 주변 8개를 선택
    DKVector<uint32> selectSections(const TerrainWorld& world, double cameraX, double cameraZ);
    // 선택한 Section 목록을 받아, 그 안에서 각 스트리밍 LOD의 Tile을 선택
    DKVector<TileRequest> selectTiles(const TerrainWorld& world, const DKVector<uint32>& sections, double cameraX, double cameraZ, float lod0WorldRadius, uint32 tileBudgetPerLod);

    class Streamer
    {
    public:
        ~Streamer();
        bool start(const TerrainWorld& world, uint32 tileRequestCapacity = 64);
        void stop();
        void request(const DKVector<uint32>& currentSections, const DKVector<uint32>& missingSections, const DKVector<TileRequest>& tileRequests);
        bool takeReady(TileLoadResult& tileLoadResult);
        bool takeReady(SectionLoadResult& sectionLoadResult);
    private:
        void run();
        TerrainWorld _world;
        uint32 _tileRequestCapacity = 64;
        bool _stopping = true;
        std::thread _worker;
        std::mutex _mutex;
        std::condition_variable _wake;

        DKVector<uint32> _wantedSections;
        DKDeque<uint32> _sectionRequests;
        DKDeque<SectionLoadResult> _readySections;

        DKDeque<TileRequest> _tileRequests;
        DKDeque<TileLoadResult> _readyTiles;
        bool _residencyChanged = false;
        uint32 _loadingSectionId = ~0u;
        DKOptional<TileRequest> _loadingTileRequest;
    };

    uint32 mipCount(uint32 chunkCellCountPerSide);
    bool buildArchiveLayout(ArchiveHeader& header, DKVector<ArchiveBlockInfo>& blocks, DKVector<uint32>& lodBlockOffsets);
}
