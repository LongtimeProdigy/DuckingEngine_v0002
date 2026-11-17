#ifndef DK_ATMOSPHERE_LIMITS_H
#define DK_ATMOSPHERE_LIMITS_H

#define DK_ATMOSPHERE_ABSORPTION_LAYER_COUNT 2u
#define DK_ATMOSPHERE_SHADOW_CASCADE_COUNT 4u
// The bounce volume packs its X/Z grid into the texture's horizontal dimension.
#define SURFACE_BOUNCE_GRID 16u
// Match the bounce shader's sample budget and minimum cell size used by its integration.
#define SURFACE_BOUNCE_MAX_SAMPLES 64u
#define SURFACE_BOUNCE_MIN_CELL_WORLD_SIZE 0.01f
// Preserve the transmittance shader's existing diagnostic range on the CPU as well.
#define ATMOSPHERE_LUT_MAX_RADIAL_DEPTH 32000.0f

// Shared texture resolutions keep CPU allocation and shader sampling in agreement.
#define ATMOSPHERE_LUT_WIDTH 512u
#define ATMOSPHERE_LUT_HEIGHT 128u
// 다중 산란 LUT의 할당·dispatch·샘플 좌표가 같은 해상도를 사용해야 한다.
#define ATMOSPHERE_MULTIPLESCATTERING_WIDTH 32u
#define ATMOSPHERE_MULTIPLESCATTERING_HEIGHT 32u
#define SKY_WIDTH 64u
#define SKY_HEIGHT 32u
// GGX 거칠기 단계 뒤에 확산 irradiance 한 단계를 배치하는 atlas 형식이다.
#define SKY_SPECULAR_BAND_COUNT 5u
#define SKY_DIFFUSE_BAND SKY_SPECULAR_BAND_COUNT
#define SKY_BANDS (SKY_SPECULAR_BAND_COUNT + 1u)

#endif
