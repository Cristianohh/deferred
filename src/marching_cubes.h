/*! @file marching_cubes.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 11/16/12 2:22 AM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __marching_cubes_h__
#define __marching_cubes_h__

#include "vec_math.h"
#include "render.h"
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float3  p[8];
    float   val[8];
} gridcell_t;
typedef struct {
    float3 p[3];
} triangle_t;

typedef float density_func_t(float3 p);


int Polygonise(gridcell_t grid, float isolevel, triangle_t triangles[5]);
void generate_terrain(density_func_t function, std::vector<VtxPosNormTex>& vertices, std::vector<uint32_t>& indices);
void generate_terrain_points(density_func_t function, float3 min, float3 max, float granularity, std::vector<float3>& vertices);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif /* include guard */
