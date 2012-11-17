/*! @file perlin_noise.c
 *  @author Kyle Weicht
 *  @date 11/16/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "perlin_noise.h"

#include <stdint.h>
#include <math.h>
#include <immintrin.h>
#include <smmintrin.h>

#ifdef __GNUC__
    #define ALIGN(x) __attribute__((aligned(x)))
#endif

/*
 * Internal
 */
/* Source: http://mrl.nyu.edu/~perlin/noise/ */
static float __inline fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}
static __inline __m128 fade_sse(__m128 t) {
    __m128 v6  = _mm_set1_ps(6.0f);
    __m128 v15 = _mm_set1_ps(15.0f);
    __m128 v10 = _mm_set1_ps(10.0f);

    __m128 a = _mm_mul_ps(t, v6); /* (t * 6.0f) */
    __m128 b = _mm_sub_ps(a, v15); /* (a - 15.0f) */
    __m128 c = _mm_mul_ps(t, b); /* (t * b) */
    __m128 d = _mm_add_ps(c, v10); /* c + 10.0f */
    __m128 vt3 = _mm_mul_ps(t, _mm_mul_ps(t, t)); /* t*t*t */
    return _mm_mul_ps(d, vt3);
}
static __inline __m256 fade_avx(__m256 t) {
    __m256 v6  = _mm256_set1_ps(6.0f);
    __m256 v15 = _mm256_set1_ps(15.0f);
    __m256 v10 = _mm256_set1_ps(10.0f);

    __m256 a = _mm256_mul_ps(t, v6); /* (t * 6.0f) */
    __m256 b = _mm256_sub_ps(a, v15); /* (a - 15.0f) */
    __m256 c = _mm256_mul_ps(t, b); /* (t * b) */
    __m256 d = _mm256_add_ps(c, v10); /* c + 10.0f */
    __m256 vt3 = _mm256_mul_ps(t, _mm256_mul_ps(t, t)); /* t*t*t */
    return _mm256_mul_ps(d, vt3);
}
static float __inline lerp(float t, float a, float b) {
    return a + t * (b - a);
}
static __inline __m128 lerp_sse(__m128 t, __m128 a, __m128 b) {
    __m128 b_minus_a = _mm_sub_ps(b, a);     /* (b-a) */
    __m128 t_bma = _mm_mul_ps(t, b_minus_a); /* (t*(b-a) */
    return _mm_add_ps(a, t_bma);               /* a + (t*(b-a)) */
}
static __inline __m256 lerp_avx(__m256 t, __m256 a, __m256 b) {
    __m256 b_minus_a = _mm256_sub_ps(b, a);     /* (b-a) */
    __m256 t_bma = _mm256_mul_ps(t, b_minus_a); /* (t*(b-a) */
    return _mm256_add_ps(a, t_bma);               /* a + (t*(b-a)) */
}
static __inline float grad(int hash, float x, float y, float z) {
    int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
    float u = h<8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
    v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}
static __inline __m128 grad_sse(__m128i vhash, __m128 vx, __m128 vy, __m128 vz) {
    float x[4] ALIGN(16);
    float y[4] ALIGN(16);
    float z[4] ALIGN(16);
    float r[4] ALIGN(16);
    int hash[4] ALIGN(16);
    int ii;

    _mm_store_ps(x, vx);
    _mm_store_ps(y, vy);
    _mm_store_ps(z, vz);
    _mm_store_si128((__m128i*)hash, vhash);

    for(ii=0; ii<4; ++ii) {
        int h = hash[ii] & 15;                      // CONVERT LO 4 BITS OF HASH CODE
        float u = h<8 ? x[ii] : y[ii],                 // INTO 12 GRADIENT DIRECTIONS.
        v = h<4 ? y[ii] : h==12||h==14 ? x[ii] : z[ii];
        r[ii] = ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
    }
    return _mm_load_ps(r);
}
static __inline __m256 grad_avx(__m256i vhash, __m256 vx, __m256 vy, __m256 vz) {
    float x[8] ALIGN(32);
    float y[8] ALIGN(32);
    float z[8] ALIGN(32);
    float r[8] ALIGN(32);
    int hash[8] ALIGN(32);
    int ii;

    _mm256_store_ps(x, vx);
    _mm256_store_ps(y, vy);
    _mm256_store_ps(z, vz);
    _mm256_store_si256((__m256i*)hash, vhash);

    for(ii=0; ii<8; ++ii) {
        int h = hash[ii] & 15;                      // CONVERT LO 4 BITS OF HASH CODE
        float u = h<8 ? x[ii] : y[ii],                 // INTO 12 GRADIENT DIRECTIONS.
        v = h<4 ? y[ii] : h==12||h==14 ? x[ii] : z[ii];
        r[ii] = ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
    }
    return _mm256_load_ps(r);
}
static int32_t _pp[512] = {
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
/*static __inline int32_t _p(int index) {
    return _pp[index];
}*/
#define _p(mask, i) (mask ^ _pp[i])
static __inline __m128i _p_sse(__m128i v) {
    int i[4] ALIGN(16);
    _mm_store_si128((__m128i*)i, v);
    return _mm_set_epi32(_pp[i[0]], _pp[i[1]], _pp[i[2]], _pp[i[3]]);
}
static __inline __m256i _p_avx(__m256i v) {
    int i[8] ALIGN(32);
    _mm256_store_si256((__m256i*)i, v);
    return _mm256_set_epi32(_pp[i[0]], _pp[i[1]], _pp[i[2]], _pp[i[3]],
                            _pp[i[4]], _pp[i[5]], _pp[i[6]], _pp[i[7]]);
}

/*
 * External
 */
float noise(uint32_t seed, float x, float y, float z) {
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
    A  = (_p(seed, X  ))+Y,
    AA = (_p(seed, A  ))+Z,
    AB = (_p(seed, A+1))+Z,      // HASH COORDINATES OF
    B  = (_p(seed, X+1))+Y,
    BA = (_p(seed, B  ))+Z,
    BB = (_p(seed, B+1))+Z;      // THE 8 CUBE CORNERS,

    return lerp(w, lerp(v, lerp(u, grad(_p(seed, AA  ), x  , y  , z   ),  // AND ADD
                                   grad(_p(seed, BA  ), x-1, y  , z   )), // BLENDED
                           lerp(u, grad(_p(seed, AB  ), x  , y-1, z   ),  // RESULTS
                                   grad(_p(seed, BB  ), x-1, y-1, z   ))),// FROM  8
                   lerp(v, lerp(u, grad(_p(seed, AA+1), x  , y  , z-1 ),  // CORNERS
                                   grad(_p(seed, BA+1), x-1, y  , z-1 )), // OF CUBE
                           lerp(u, grad(_p(seed, AB+1), x  , y-1, z-1 ),
                                   grad(_p(seed, BB+1), x-1, y-1, z-1 ))));
}
void noisev(uint32_t seed, const float* x, const float* y, const float* z, float* n, int count) {
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

    //
    // SSE
    //
#if 1
    _mm256_zeroall();
    for(ii=0; ii<count; ii += 4) {
        __m128 vn;
        __m128i vpAA, vpBA, vpAB, vpBB, vpAA1, vpBA1, vpAB1, vpBB1;

        __m128 vx1, vy1, vz1;

        __m128i vseed = _mm_set1_epi32(seed);
        __m128i v1 = _mm_set1_epi32(1);
        __m128 v1f = _mm_set1_ps(1.0f);

        __m128 vu, vv, vw; // float u, v, w;
        __m128 vx = _mm_load_ps(x + ii),
               vy = _mm_load_ps(y + ii),
               vz = _mm_load_ps(z + ii); //float x = *xv, y = *yv, z = *zv;
        __m128i vA, vAA, vAB, vB, vBA, vBB; // int A, AA, AB, B, BA, BB;
        __m128 vfloorx = _mm_floor_ps(vx);
        __m128 vfloory = _mm_floor_ps(vy);
        __m128 vfloorz = _mm_floor_ps(vz);

        __m128i v255 = _mm_set1_epi32(255);
        __m128i vX = _mm_and_si128(_mm_cvtps_epi32(vfloorx), v255), // int X = (int)floorf(x) & 255,
                vY = _mm_and_si128(_mm_cvtps_epi32(vfloory), v255), //     Y = (int)floorf(y) & 255,
                vZ = _mm_and_si128(_mm_cvtps_epi32(vfloorz), v255); //     Z = (int)floorf(z) & 255;
        vx = _mm_sub_ps(vx, vfloorx); // x -= floorf(x);
        vy = _mm_sub_ps(vy, vfloory); // y -= floorf(y);
        vz = _mm_sub_ps(vz, vfloorz); // z -= floorf(z);
        vu = fade_sse(vx); // u = fade(x),
        vv = fade_sse(vy); // v = fade(y),
        vw = fade_sse(vz); // w = fade(z);

        vA  = _mm_add_epi32(_mm_xor_si128(vseed, _p_sse(                  vX )), vY); // A  = (seed^_p[X  ])+Y;
        vAA = _mm_add_epi32(_mm_xor_si128(vseed, _p_sse(                  vA )), vZ); // AA = (seed^_p[A  ])+Z;
        vAB = _mm_add_epi32(_mm_xor_si128(vseed, _p_sse(_mm_add_epi32(v1, vA))), vZ); // AB = (seed^_p[A+1])+Z;
        vB  = _mm_add_epi32(_mm_xor_si128(vseed, _p_sse(_mm_add_epi32(v1, vX))), vY); // B  = (seed^_p[X+1])+Y;
        vBA = _mm_add_epi32(_mm_xor_si128(vseed, _p_sse(                  vB )), vZ); // BA = (seed^_p[B  ])+Z;
        vBB = _mm_add_epi32(_mm_xor_si128(vseed, _p_sse(_mm_add_epi32(v1, vB))), vZ); // BB = (seed^_p[B+1])+Z;

        vpAA  = _p_sse(vAA);
        vpBA  = _p_sse(vBA);
        vpAB  = _p_sse(vAB);
        vpBB  = _p_sse(vBB);
        vpAA1 = _p_sse(_mm_add_epi32(v1, vAA));
        vpBA1 = _p_sse(_mm_add_epi32(v1, vBA));
        vpAB1 = _p_sse(_mm_add_epi32(v1, vAB));
        vpBB1 = _p_sse(_mm_add_epi32(v1, vBB));

        vx1 = _mm_sub_ps(vx, v1f);
        vy1 = _mm_sub_ps(vy, v1f);
        vz1 = _mm_sub_ps(vz, v1f);

        //n[ii] = lerp(w, lerp(v, lerp(u, grad(seed ^ _p[AA  ], x  , y  , z   ),
        //                                grad(seed ^ _p[BA  ], x-1, y  , z   )),
        //                        lerp(u, grad(seed ^ _p[AB  ], x  , y-1, z   ),
        //                                grad(seed ^ _p[BB  ], x-1, y-1, z   ))),
        //                lerp(v, lerp(u, grad(seed ^ _p[AA+1], x  , y  , z-1 ),
        //                                grad(seed ^ _p[BA+1], x-1, y  , z-1 )),
        //                        lerp(u, grad(seed ^ _p[AB+1], x  , y-1, z-1 ),
        //                                grad(seed ^ _p[BB+1], x-1, y-1, z-1 ))));

        vn = lerp_sse(vw, lerp_sse(vv, lerp_sse(vu, grad_sse(_mm_xor_si128(vseed, vpAA ), vx , vy , vz  ),
                                                    grad_sse(_mm_xor_si128(vseed, vpBA ), vx1, vy , vz  )),
                                       lerp_sse(vu, grad_sse(_mm_xor_si128(vseed, vpAB ), vx , vy1, vz  ),
                                                    grad_sse(_mm_xor_si128(vseed, vpBB ), vx1, vy1, vz  ))),
                          lerp_sse(vv, lerp_sse(vu, grad_sse(_mm_xor_si128(vseed, vpAA1), vx , vy , vz1 ),
                                                    grad_sse(_mm_xor_si128(vseed, vpBA1), vx1, vy , vz1 )),
                                       lerp_sse(vu, grad_sse(_mm_xor_si128(vseed, vpAB1), vx , vy1, vz1 ),
                                                    grad_sse(_mm_xor_si128(vseed, vpBB1), vx1, vy1, vz1 ))));
        _mm_store_ps(n+ii, vn);
    }
#endif

    //
    // AVX
    //
#if 0
    _mm256_zeroall();
    for(ii=0; ii<count; ii += 4) {
        __m256 vn;
        __m256i vpAA, vpBA, vpAB, vpBB, vpAA1, vpBA1, vpAB1, vpBB1;

        __m256 vx1, vy1, vz1;

        __m256i vseed = _mm256_set1_epi32(seed);
        __m256i v1 = _mm256_set1_epi32(1);
        __m256 v1f = _mm256_set1_ps(1.0f);

        __m256 vu, vv, vw; // float u, v, w;
        __m256 vx = _mm256_load_ps(x + ii),
               vy = _mm256_load_ps(y + ii),
               vz = _mm256_load_ps(z + ii); //float x = *xv, y = *yv, z = *zv;
        __m256i vA, vAA, vAB, vB, vBA, vBB; // int A, AA, AB, B, BA, BB;
        __m256 vfloorx = _mm256_floor_ps(vx);
        __m256 vfloory = _mm256_floor_ps(vy);
        __m256 vfloorz = _mm256_floor_ps(vz);


        __m256i v255 = _mm256_set1_epi32(255);
        __m256i vX = _mm256_and_ps(_mm256_cvtps_epi32(vfloorx), v255), // int X = (int)floorf(x) & 255,
                vY = _mm256_and_ps(_mm256_cvtps_epi32(vfloory), v255), //     Y = (int)floorf(y) & 255,
                vZ = _mm256_and_ps(_mm256_cvtps_epi32(vfloorz), v255); //     Z = (int)floorf(z) & 255;
        vx = _mm256_sub_ps(vx, vfloorx); // x -= floorf(x);
        vy = _mm256_sub_ps(vy, vfloory); // y -= floorf(y);
        vz = _mm256_sub_ps(vz, vfloorz); // z -= floorf(z);
        vu = fade_avx(vx); // u = fade(x),
        vv = fade_avx(vy); // v = fade(y),
        vw = fade_avx(vz); // w = fade(z);

        vA  = _mm_add_epi32(_mm_xor_si128(vseed, _p_avx(                  vX )), vY); // A  = (seed^_p[X  ])+Y;
        vAA = _mm_add_epi32(_mm_xor_si128(vseed, _p_avx(                  vA )), vZ); // AA = (seed^_p[A  ])+Z;
        vAB = _mm_add_epi32(_mm_xor_si128(vseed, _p_avx(_mm_add_epi32(v1, vA))), vZ); // AB = (seed^_p[A+1])+Z;
        vB  = _mm_add_epi32(_mm_xor_si128(vseed, _p_avx(_mm_add_epi32(v1, vX))), vY); // B  = (seed^_p[X+1])+Y;
        vBA = _mm_add_epi32(_mm_xor_si128(vseed, _p_avx(                  vB )), vZ); // BA = (seed^_p[B  ])+Z;
        vBB = _mm_add_epi32(_mm_xor_si128(vseed, _p_avx(_mm_add_epi32(v1, vB))), vZ); // BB = (seed^_p[B+1])+Z;

        vpAA  = _p_avx(vAA);
        vpBA  = _p_avx(vBA);
        vpAB  = _p_avx(vAB);
        vpBB  = _p_avx(vBB);
        vpAA1 = _p_avx(_mm_add_epi32(v1, vAA));
        vpBA1 = _p_avx(_mm_add_epi32(v1, vBA));
        vpAB1 = _p_avx(_mm_add_epi32(v1, vAB));
        vpBB1 = _p_avx(_mm_add_epi32(v1, vBB));

        vx1 = _mm256_sub_ps(vx, v1f);
        vy1 = _mm256_sub_ps(vy, v1f);
        vz1 = _mm256_sub_ps(vz, v1f);

        //n[ii] = lerp(w, lerp(v, lerp(u, grad(seed ^ _p[AA  ], x  , y  , z   ),
        //                                grad(seed ^ _p[BA  ], x-1, y  , z   )),
        //                        lerp(u, grad(seed ^ _p[AB  ], x  , y-1, z   ),
        //                                grad(seed ^ _p[BB  ], x-1, y-1, z   ))),
        //                lerp(v, lerp(u, grad(seed ^ _p[AA+1], x  , y  , z-1 ),
        //                                grad(seed ^ _p[BA+1], x-1, y  , z-1 )),
        //                        lerp(u, grad(seed ^ _p[AB+1], x  , y-1, z-1 ),
        //                                grad(seed ^ _p[BB+1], x-1, y-1, z-1 ))));

        vn = lerp_avx(vw, lerp_avx(vv, lerp_avx(vu, grad_avx(_mm_xor_si128(vseed, vpAA ), vx , vy , vz  ),
                                                    grad_avx(_mm_xor_si128(vseed, vpBA ), vx1, vy , vz  )),
                                       lerp_avx(vu, grad_avx(_mm_xor_si128(vseed, vpAB ), vx , vy1, vz  ),
                                                    grad_avx(_mm_xor_si128(vseed, vpBB ), vx1, vy1, vz  ))),
                          lerp_avx(vv, lerp_avx(vu, grad_avx(_mm_xor_si128(vseed, vpAA1), vx , vy , vz1 ),
                                                    grad_avx(_mm_xor_si128(vseed, vpBA1), vx1, vy , vz1 )),
                                       lerp_avx(vu, grad_avx(_mm_xor_si128(vseed, vpAB1), vx , vy1, vz1 ),
                                                    grad_avx(_mm_xor_si128(vseed, vpBB1), vx1, vy1, vz1 ))));
        _mm_store_ps(n+ii, vn);
    }
#endif
}
