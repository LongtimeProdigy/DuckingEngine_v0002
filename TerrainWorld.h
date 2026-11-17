#pragma once

#include "TerrainData.h"
#include "Resource/Material/TerrainStreamingLimits.h"

namespace DK
{
    struct TerrainSection
    {
        uint32 _id = 0;            // Section 고유 인덱스: _z * TerrainWorld::_sectionCountX + _x
        uint32 _x = 0;             // 전체 Section 격자에서 X축 인덱스
        uint32 _z = 0;             // 전체 Section 격자에서 Z축 인덱스
        DKString _archivePath;     // terrain.xml의 디렉터리를 기준으로 결합한 Section Archive 경로
    };

    struct TerrainWorld
    {
        ArchiveHeader _header;                 // 전체 지형의 X/Z 샘플 수와 모든 Section이 공유하는 Chunk 크기·샘플 간격·높이 변환 정보. 개별 Archive의 파일 배치 정보는 나타내지 않는다.
        uint32 _sectionCellCountPerSide = 0;    // 모든 Section에 공통인 한 변의 셀 수. 샘플 수는 셀 수 + 1이며, 전체 지형의 X/Z 셀 수를 나누어떨어지게 해야 한다.
        uint32 _sectionCountX = 0;             // 전체 지형을 X축으로 나눈 Section 개수
        uint32 _sectionCountZ = 0;             // 전체 지형을 Z축으로 나눈 Section 개수
        uint32 _overviewLod = 0;               // Section 전체의 저해상도 Height·Normal에 사용하는 LOD. 샘플 간격은 원본의 2^LOD배다.
        DKVector<TerrainSection> _sections;    // 전체 Section의 경로·격자 위치 목록. Z행 안에서 X가 증가하는 순서이며 인덱스는 Section ID와 같다.

        bool open(const DKString& path);
        bool matchesSectionArchive(const ArchiveHeader& header) const;
        DKVector<uint32> selectSections(double cameraX, double cameraZ) const;
    };

    bool splitTerrainWorld(const DKString& archivePath, const DKString& directoryPath, uint32 sectionCellCountPerSide);
}
