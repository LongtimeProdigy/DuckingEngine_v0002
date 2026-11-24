#ifndef DK_TERRAIN_SHARED_H
#define DK_TERRAIN_SHARED_H

// 카메라의 중심 Section과 인접 Section을 저장하는 CPU/GPU 공통 atlas 배치다.
#define TERRAIN_SECTION_SLOT_COUNT_PER_SIDE 3u
#define TERRAIN_SECTION_SLOT_COUNT (TERRAIN_SECTION_SLOT_COUNT_PER_SIDE * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE)

#ifdef __cplusplus
#include "../../CommonTypes.h"

namespace DK
{
    dk_inline constexpr uint32 terrainPageTableLodRowOffset(const uint32 sectionLod0TileCountPerSide, const uint32 lod)
#else
    uint terrainPageTableLodRowOffset(const uint sectionLod0TileCountPerSide, const uint lod)
#endif
    {
        if (lod == 0)
            return 0;

        // LOD마다 행 수가 절반이므로 앞선 LOD의 행 수 합은 등비수열의 합 2N - N / 2^(lod - 1)이다.
        return sectionLod0TileCountPerSide * 2 - (sectionLod0TileCountPerSide >> (lod - 1));
    }
#ifdef __cplusplus
}
#endif

#endif
