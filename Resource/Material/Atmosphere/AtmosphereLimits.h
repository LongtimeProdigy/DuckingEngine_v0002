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
#define SKY_WIDTH 64u
#define SKY_HEIGHT 32u
// Bands 0..4 store GGX roughness levels; band 5 stores diffuse irradiance.
#define SKY_BANDS 6u

#endif
