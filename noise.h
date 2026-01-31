#ifndef NOISE_H
#define NOISE_H

static double fade(double t);
static double lerp(double a, double b, double t);
static uint32_t hash2D(int x, int y, uint32_t seed);
static double grad(int ix, int iy, double x, double y, uint32_t seed);
double perlinNoise(double x, double y, uint32_t seed);
float fbm2D(
    float x,
    float y,
    uint32_t seed,
    int octaves,
    float lacunarity,  // frequency multiplier (usually 2.0)
    float gain         // amplitude multiplier (usually 0.5)
);

#endif