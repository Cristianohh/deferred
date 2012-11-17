/*! @file perlin_noise.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 11/16/12 2:48 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __perlin_noise_h__
#define __perlin_noise_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float noise(uint32_t seed, float x, float y, float z);
void noisev(uint32_t seed, const float* x, const float* y, const float* z, float* n, int count);

#ifdef __cplusplus
} // extern "C" {
#endif


#endif /* include guard */
