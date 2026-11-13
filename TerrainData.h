#pragma once

#include "CommonTypes.h"

#include <cstdint>

namespace DK
{
    enum class SectionType : uint32_t
    {
        Height = 1,             // 샘플당 uint16 하나 (가장 상세한 LOD 0 높이)
        Normal = 2,             // 샘플당 int16 두 개 (LOD 0 법선의 X, Z)
        MaterialIds = 3,        // 혼합할 재질 ID 네 개
        MaterialWeights = 4,    // 각 재질의 혼합 가중치
        Biome = 5,              // 숲·초원 등 지역 분류
        Holes = 6,              // 셀 단위 지형 구멍
        MeshReferences = 7,     // 동굴·절벽 등 외부 메시의 자산 ID와 변환
        HeightPyramid = 8,      // LOD 1 이후의 uint16 높이 배열 (먼 거리에서 사용할 저해상도 높이)
        NormalPyramid = 9       // 같은 LOD들의 법선 X, Z (저해상도 표면의 조명 계산)
    };
    enum class Format : uint32_t
    {
        R16UInt = 1,
        RG16SNorm = 2,
        RGBA16UInt = 3,
        RGBA8UNorm = 4,
        R8UInt = 5,
        MeshReference = 6
    };

#pragma pack(push, 1)
    struct ArchiveHeader
    {
        char _magic[8] = { 'D', 'K', 'T', 'R', 'N', '0', '0', '3' };
        uint32 _version = 3;
        uint32 _headerBytes = 64;
        uint32 _width = 0;              // 원본 높이 샘플 수
        uint32 _height = 0;             // 원본 높이 샘플 수
        uint32 _chunkCells = 128;       // 청크당 셀 수
        uint32 _chunkCount = 0;         // 전체 청크 개수
        float _sampleSpacing = 1;       // 샘플 사이의 월드 거리
        float _heightOffset = -16;      // 저장된 높이 정수를 실제 Y 좌표로 복원하는 기준
        float _heightScale = 128;       // 저장된 높이 정수를 실제 Y 좌표로 복원하는 기준
        uint32 _reserved = 0;
        uint64 _directoryOffset = 64;   // 청크 목록 시작 위치
        uint64 _fileBytes = 0;          // 전체 파일 크기
    };
    struct ChunkInfo
    {
        // 청크의 월드 시작 XZ = chunkXZ × chunkCells × sampleSpacing

        uint32 _x = 0;                  // 청크 인덱스
        uint32 _z = 0;                  // 청크 인덱스
        uint32 _cellsX = 0;             // 실제 셀 수
        uint32 _cellsZ = 0;             // 실제 셀 수
        uint32 _sectionCount = 0;       // 섹션 개수
        uint32 _reserved = 0;
        float _minY = 0;                // 높이 범위
        float _maxY = 0;                // 높이 범위
        uint64 _sectionsOffset = 0;     // 파일 위치
        uint64 _endOffset = 0;          // 파일 위치
    };
    struct SectionInfo
    {
        SectionType _type = SectionType::Height;    // 어떤 데이터인가
        Format _format = Format::R16UInt;           // 숫자를 어떤 형식으로 저장했는가
        uint32 _count = 0;                          // 원소가 몇 개인가
        uint32 _stride = 0;                         // 원소 하나가 몇 바이트인가
        uint64 _offset = 0;                         // 파일 시작부터 몇 바이트 떨어져 있는가
        uint64 _bytes = 0;                          // 전체 데이터가 몇 바이트인가
    };
    struct MeshReference
    {
        uint64 _assetId = 0;
        float _transform[12] = {};
    };
#pragma pack(pop)
    static_assert(sizeof(ArchiveHeader) == 64 && sizeof(ChunkInfo) == 48);
    static_assert(sizeof(SectionInfo) == 32 && sizeof(MeshReference) == 56);

    struct Section
    {
        SectionInfo _info;
        DKVector<uint8_t> _data;
    };
    struct Chunk
    {
        ChunkInfo _info;
        DKVector<Section> _sections;
        const Section* find(SectionType type) const;
    };

    struct MipTile
    {
        uint32_t _x = 0, _z = 0, _lod = 0, _side = 0;
        DKVector<uint16_t> _heights;
        DKVector<DKArray<int16_t, 2>> _normals;
    };

    struct TerrainOverview
    {
        uint32 _width = 0;
        uint32 _height = 0;
        uint32 _lod = 0;
        DKVector<uint16_t> _heights;
        DKVector<DKArray<int16_t, 2>> _normals;
    };

    class Archive
    {
    public:
        bool open(const DKString& path);
        bool loadChunk(uint32_t x, uint32_t z, Chunk& out) const;
        bool loadMip(uint32_t x, uint32_t z, uint32_t lod, MipTile& out) const;
        bool loadVirtualTile(uint32 x, uint32 z, uint32 lod, MipTile& out) const;
        bool loadOverview(uint32 lod, TerrainOverview& out) const;
        dk_inline bool isOpen() const
        {
            return _chunks.empty() == false;
        }
        dk_inline const ArchiveHeader& header() const
        {
            return _header;
        }
        dk_inline const DKVector<ChunkInfo>& chunks() const
        {
            return _chunks;
        }

    private:
        bool loadGrid(uint32 startX, uint32 startZ, uint32 lod, uint32 width, uint32 height, TerrainOverview& out) const;

    private:
        DKString _path;
        ArchiveHeader _header;
        DKVector<ChunkInfo> _chunks;
        DKVector<DKVector<SectionInfo>> _sections;
    };

    bool writeArchive(const DKString& path, ArchiveHeader header, const DKVector<Chunk>& chunks);

    bool buildHeightfield(const ArchiveHeader& settings, const DKVector<uint16_t>& heights, DKVector<Chunk>& out);
    uint32_t mipCount(uint32_t chunkCells);
}
