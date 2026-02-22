#include <math.h>
#include <stdint.h>
#include "noise.h"

/* Fade curve */
static double fade(double t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

/* Linear interpolation */
static double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

/* Seeded integer hash */
static uint32_t hash2D(int x, int y, uint32_t seed) {
    uint32_t h = seed;
    h ^= (uint32_t)x * 0x27d4eb2d;
    h ^= (uint32_t)y * 0x85ebca6b;
    h ^= h >> 15;
    h *= 0x2c1b3c6d;
    h ^= h >> 12;
    return h;
}

/* Gradient selection */
static double grad(int ix, int iy, double x, double y, uint32_t seed) {
    uint32_t h = hash2D(ix, iy, seed) & 7;

    /* 8 unit gradients */
    static const double gradX[8] = { 1, -1,  1, -1,  0,  0,  0.7071, -0.7071 };
    static const double gradY[8] = { 0,  0,  1, -1,  1, -1,  0.7071, -0.7071 };

    return gradX[h] * x + gradY[h] * y;
}

/* 2D Perlin noise */
double perlinNoise(double x, double y, uint32_t seed) {
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    double sx = x - (double)x0;
    double sy = y - (double)y0;

    double n00 = grad(x0, y0, x - x0, y - y0, seed);
    double n10 = grad(x1, y0, x - x1, y - y0, seed);
    double n01 = grad(x0, y1, x - x0, y - y1, seed);
    double n11 = grad(x1, y1, x - x1, y - y1, seed);

    double u = fade(sx);
    double v = fade(sy);

    double nx0 = lerp(n00, n10, u);
    double nx1 = lerp(n01, n11, u);

    return lerp(nx0, nx1, v);
}


float fbm2D(
    float x,
    float y,
    uint32_t seed,
    int octaves,
    float lacunarity,  // frequency multiplier (usually 2.0)
    float gain         // amplitude multiplier (usually 0.5)
) {
    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxSum = 0.0f;

    for (int i = 0; i < octaves; i++) {
        float n = perlinNoise(
            x * frequency,
            y * frequency,
            seed + i * 1013   // decorrelate octaves
        );

        sum += n * amplitude;
        maxSum += amplitude;

        amplitude *= gain;
        frequency *= lacunarity;
    }

    /* normalize to [-1,1] */
    sum /= maxSum;
    
    return sum;
}


float ridgedFbm2D(
    float x,
    float y,
    uint32_t seed,
    int octaves,
    float lacunarity,
    float gain
) {
    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxSum = 0.0f;

    for (int i = 0; i < octaves; i++) {
        float n = perlinNoise(x * frequency, y * frequency, seed + i * 1013);

        n = fabsf(n);        // fold
        n = 1.0f - n;        // ridges
        n *= n;              // sharpen peaks

        sum += n * amplitude;
        maxSum += amplitude;

        amplitude *= gain;
        frequency *= lacunarity;
    }

    return sum / maxSum;     // [0,1]
}

static uint32_t hash3D(int x, int y, int z, uint32_t seed) {
    uint32_t h = seed;
    h ^= (uint32_t)x * 0x27d4eb2d;
    h ^= (uint32_t)y * 0x85ebca6b;
    h ^= (uint32_t)z * 0x165667b1;
    h ^= h >> 15;
    h *= 0x2c1b3c6d;
    h ^= h >> 12;
    return h;
}

static double grad3D(int ix, int iy, int iz, double x, double y, double z, uint32_t seed) {
    uint32_t h = hash3D(ix, iy, iz, seed) & 15;

    static const double grad[16][3] = {
        {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
        {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
        {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1},
        {1,1,0},{-1,1,0},{0,-1,1},{0,-1,-1}
    };

    return grad[h][0]*x + grad[h][1]*y + grad[h][2]*z;
}


double perlinNoise3D(double x, double y, double z, uint32_t seed) {
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int z0 = (int)floor(z);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    double sx = x - x0;
    double sy = y - y0;
    double sz = z - z0;

    double n000 = grad3D(x0, y0, z0, x - x0, y - y0, z - z0, seed);
    double n100 = grad3D(x1, y0, z0, x - x1, y - y0, z - z0, seed);
    double n010 = grad3D(x0, y1, z0, x - x0, y - y1, z - z0, seed);
    double n110 = grad3D(x1, y1, z0, x - x1, y - y1, z - z0, seed);
    double n001 = grad3D(x0, y0, z1, x - x0, y - y0, z - z1, seed);
    double n101 = grad3D(x1, y0, z1, x - x1, y - y0, z - z1, seed);
    double n011 = grad3D(x0, y1, z1, x - x0, y - y1, z - z1, seed);
    double n111 = grad3D(x1, y1, z1, x - x1, y - y1, z - z1, seed);

    double u = fade(sx);
    double v = fade(sy);
    double w = fade(sz);

    double nx00 = lerp(n000, n100, u);
    double nx10 = lerp(n010, n110, u);
    double nx01 = lerp(n001, n101, u);
    double nx11 = lerp(n011, n111, u);

    double nxy0 = lerp(nx00, nx10, v);
    double nxy1 = lerp(nx01, nx11, v);

    return lerp(nxy0, nxy1, w);
}

float fbm3D(
    float x, float y, float z,
    uint32_t seed,
    int octaves,
    float lacunarity,
    float gain
) {
    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxSum = 0.0f;

    for (int i = 0; i < octaves; i++) {
        float n = perlinNoise3D(x * frequency, y * frequency, z * frequency, seed + i*1013);
        sum += n * amplitude;
        maxSum += amplitude;

        amplitude *= gain;
        frequency *= lacunarity;
    }

    return sum / maxSum;  // normalize to [-1,1]
}