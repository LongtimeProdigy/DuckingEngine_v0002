#ifndef DK_TERRAIN_PAGE_TABLE_H
#define DK_TERRAIN_PAGE_TABLE_H

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

        // LOD마다 행 수가 절반이므로 앞선 LOD의 행 수 합은 2N - N / 2^(lod - 1)이다.
        return sectionLod0TileCountPerSide * 2 - (sectionLod0TileCountPerSide >> (lod - 1));
    }
#ifdef __cplusplus
}
#endif

#endif
