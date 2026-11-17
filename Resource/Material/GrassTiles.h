#ifndef GRASS_TILES_H
#define GRASS_TILES_H

#define GRASS_TILE_METERS 10.0f
#define GRASS_TILE_GRID_SIDE_DEFAULT 64u
// 풀잎은 사각형 두 개와 끝 삼각형 하나로 구성하며 CPU 인덱스와 셰이더 정점 해석을 공유한다.
#define GRASS_BLADE_COUNT 6u
#define GRASS_BLADE_VERTEX_COUNT 7u
#define GRASS_BLADE_INDEX_COUNT 15u
#define GRASS_LOD0_END 18.0f
#define GRASS_LOD1_END 42.0f
#define GRASS_LOD2_END 79.0f
#define GRASS_LOD3_END 130.0f
#define GRASS_LOD4_END 195.0f
#define GRASS_LOD5_END 270.0f

#endif
