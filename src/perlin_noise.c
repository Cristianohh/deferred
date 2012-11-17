/*! @file perlin_noise.c
 *  @author Kyle Weicht
 *  @date 11/16/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "perlin_noise.h"

#include <stdint.h>
#include <math.h>
#include <immintrin.h>

/*
 * Internal
 */
/* Source: http://mrl.nyu.edu/~perlin/noise/ */
static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}
static void fade_sse(const float* t, float* r) {
    __m128 vt = _mm_load_ps(t);
    __m128 v6 = _mm_set1_ps(6.0f);
    __m128 v15 = _mm_set1_ps(15.0f);
    __m128 v10 = _mm_set1_ps(10.0f);
    
    __m128 a = _mm_mul_ps(vt, v6); /* (t * 6.0f) */
    __m128 b = _mm_sub_ps(a, v15); /* (a - 15.0f) */
    __m128 c = _mm_mul_ps(vt, b); /* (t * b) */
    __m128 d = _mm_add_ps(c, v10); /* c + 10.0f */
    __m128 vt3 = _mm_mul_ps(vt, _mm_mul_ps(vt, vt)); /* t*t*t */
    __m128 vr = _mm_mul_ps(d, vt3);

    _mm_store_ps(r, vr);
}
static float lerp(float t, float a, float b) {
    return a + t * (b - a);
}
static void lerp_sse(const float* t, const float* a, const float* b, float* r) {
    __m128 va = _mm_load_ps(a);
    __m128 vb = _mm_load_ps(b);
    __m128 vt = _mm_load_ps(t);

    __m128 b_minus_a = _mm_sub_ps(vb, va); /* (b-a) */
    __m128 t_bma = _mm_mul_ps(vt, b_minus_a); /* (t*(b-a) */
    __m128 vr = _mm_add_ps(va, t_bma); /* a + (t*(b-a)) */
    
    _mm_store_ps(r, vr);
}
static float grad(int hash, float x, float y, float z) {
    int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
    float u = h<8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
    v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}
static void grad_sse(const int* hash, const float* x, const float* y, const float* z, float* r) {
    int ii;
    for(ii=0; ii<4; ++ii) {
        int h = hash[ii] & 15;                      // CONVERT LO 4 BITS OF HASH CODE
        float u = h<8 ? x[ii] : y[ii],                 // INTO 12 GRADIENT DIRECTIONS.
        v = h<4 ? y[ii] : h==12||h==14 ? x[ii] : z[ii];
        r[ii] = ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
    }
}
static uint8_t _p[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
};

/*
 * External
 */
float noise(uint8_t seed, float x, float y, float z) {
    float u, v, w;
    int A, AA, AB, B, BA, BB;
    int X = (int)floorf(x) & 255,                  // FIND UNIT CUBE THAT
        Y = (int)floorf(y) & 255,                  // CONTAINS POINT.
        Z = (int)floorf(z) & 255;
    x -= floorf(x);                                // FIND RELATIVE X,Y,Z
    y -= floorf(y);                                // OF POINT IN CUBE.
    z -= floorf(z);
    u = fade(x),                                // COMPUTE FADE CURVES
    v = fade(y),                                // FOR EACH OF X,Y,Z.
    w = fade(z);
    A = (seed^_p[X  ])+Y, AA = (seed^_p[A])+Z, AB = (seed^_p[A+1])+Z,      // HASH COORDINATES OF
    B = (seed^_p[X+1])+Y, BA = (seed^_p[B])+Z, BB = (seed^_p[B+1])+Z;      // THE 8 CUBE CORNERS,

    return lerp(w, lerp(v, lerp(u, grad(seed ^ _p[AA  ], x  , y  , z   ),  // AND ADD
                                   grad(seed ^ _p[BA  ], x-1, y  , z   )), // BLENDED
                           lerp(u, grad(seed ^ _p[AB  ], x  , y-1, z   ),  // RESULTS
                                   grad(seed ^ _p[BB  ], x-1, y-1, z   ))),// FROM  8
                   lerp(v, lerp(u, grad(seed ^ _p[AA+1], x  , y  , z-1 ),  // CORNERS
                                   grad(seed ^ _p[BA+1], x-1, y  , z-1 )), // OF CUBE
                           lerp(u, grad(seed ^ _p[AB+1], x  , y-1, z-1 ),
                                   grad(seed ^ _p[BB+1], x-1, y-1, z-1 ))));
}
void noisev(uint8_t seed, const float* x, const float* y, const float* z, float* n, int count) {
    int ii;
    //
    // Scalar
    //
#if 0
    for(ii=0; ii<count; ++ii) {
        float u, v, w;
        float x = *xv, y = *yv, z = *zv;
        int A, AA, AB, B, BA, BB;
        int X = (int)floorf(x) & 255,                  // FIND UNIT CUBE THAT
            Y = (int)floorf(y) & 255,                  // CONTAINS POINT.
            Z = (int)floorf(z) & 255;
        x -= floorf(x);                                // FIND RELATIVE X,Y,Z
        y -= floorf(y);                                // OF POINT IN CUBE.
        z -= floorf(z);
        u = fade(x),                                // COMPUTE FADE CURVES
        v = fade(y),                                // FOR EACH OF X,Y,Z.
        w = fade(z);
        A = (seed^_p[X  ])+Y, AA = (seed^_p[A])+Z, AB = (seed^_p[A+1])+Z,      // HASH COORDINATES OF
        B = (seed^_p[X+1])+Y, BA = (seed^_p[B])+Z, BB = (seed^_p[B+1])+Z;      // THE 8 CUBE CORNERS,

        n[ii] = lerp(w, lerp(v, lerp(u, grad(seed ^ _p[AA  ], x  , y  , z   ),  // AND ADD
                                        grad(seed ^ _p[BA  ], x-1, y  , z   )), // BLENDED
                                lerp(u, grad(seed ^ _p[AB  ], x  , y-1, z   ),  // RESULTS
                                        grad(seed ^ _p[BB  ], x-1, y-1, z   ))),// FROM  8
                        lerp(v, lerp(u, grad(seed ^ _p[AA+1], x  , y  , z-1 ),  // CORNERS
                                        grad(seed ^ _p[BA+1], x-1, y  , z-1 )), // OF CUBE
                                lerp(u, grad(seed ^ _p[AB+1], x  , y-1, z-1 ),
                                        grad(seed ^ _p[BB+1], x-1, y-1, z-1 ))));
        ++xv, ++yv, ++zv;
    }
#endif
    for(ii=0; ii<count; ii += 4) {
        __m128 vu, vv, vw; // float u, v, w;
        __m128 vx = _mm_load_ps(x + ii),
               vy = _mm_load_ps(y + ii),
               vz = _mm_load_ps(z + ii); //float x = *xv, y = *yv, z = *zv;
        __m128i vA, vAA, vAB, vB, vBA, vBB; // int A, AA, AB, B, BA, BB;
        __m128i vX = _mm_floor_ps( int X = (int)floorf(x) & 255,                  // FIND UNIT CUBE THAT
            Y = (int)floorf(y) & 255,                  // CONTAINS POINT.
            Z = (int)floorf(z) & 255;
        x -= floorf(x);                                // FIND RELATIVE X,Y,Z
        y -= floorf(y);                                // OF POINT IN CUBE.
        z -= floorf(z);
        u = fade(x),                                // COMPUTE FADE CURVES
        v = fade(y),                                // FOR EACH OF X,Y,Z.
        w = fade(z);
        A = (seed^_p[X  ])+Y, AA = (seed^_p[A])+Z, AB = (seed^_p[A+1])+Z,      // HASH COORDINATES OF
        B = (seed^_p[X+1])+Y, BA = (seed^_p[B])+Z, BB = (seed^_p[B+1])+Z;      // THE 8 CUBE CORNERS,

        n[ii] = lerp(w, lerp(v, lerp(u, grad(seed ^ _p[AA  ], x  , y  , z   ),  // AND ADD
                                        grad(seed ^ _p[BA  ], x-1, y  , z   )), // BLENDED
                                lerp(u, grad(seed ^ _p[AB  ], x  , y-1, z   ),  // RESULTS
                                        grad(seed ^ _p[BB  ], x-1, y-1, z   ))),// FROM  8
                        lerp(v, lerp(u, grad(seed ^ _p[AA+1], x  , y  , z-1 ),  // CORNERS
                                        grad(seed ^ _p[BA+1], x-1, y  , z-1 )), // OF CUBE
                                lerp(u, grad(seed ^ _p[AB+1], x  , y-1, z-1 ),
                                        grad(seed ^ _p[BB+1], x-1, y-1, z-1 ))));
        ++xv, ++yv, ++zv;
    }
}
