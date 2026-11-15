#ifndef GRID_HASH_HLSL
#define GRID_HASH_HLSL

uint HashGridCell(int2 cell)
{
    // https://github.com/recastnavigation/recastnavigation/blob/main/DetourTileCache/Source/DetourTileCache.cpp
    // 목적은 서로 가까운 격자 좌표에서도 규칙성이 잘 드러나지 않는 정수값을 얻는 것
    uint hash = asuint(cell.x) * 0x8da6b343u ^ asuint(cell.y) * 0xd8163841u;
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    return hash;
}

#endif
