#ifndef DK_CLOUD_LIMITS_H
#define DK_CLOUD_LIMITS_H

// Cache sizes balance initialization cost against interpolation of the procedural fields.
#define DK_CLOUD_NOISE_SIZE 64
#define DK_CLOUD_WEATHER_SIZE 256

#define DK_CLOUD_LOW_FREQUENCY_CELL_COUNT 4u
#define DK_CLOUD_MID_FREQUENCY_CELL_COUNT 8u
#define DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT 16u

// Means of the periodic fields when a sample footprint cannot resolve individual cells.
#define DK_CLOUD_PERLIN_MEAN 0.5f
#define DK_CLOUD_WORLEY_MEAN 0.48f

#endif
