/*! @file render.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/13/12 1:58 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 *	@addtogroup render render
 *	@{
 */
#ifndef __render_h__
#define __render_h__

#include <stdint.h>
#include <stddef.h>
#include "vec_math.h"

#define MAX_LIGHTS 511

struct VtxPosNormTanBitanTex {
    float3  pos;
    float3  norm;
    float3  tan;
    float3  bitan;
    float2  tex;
};
struct VtxPosNormTex {
    float3  pos;
    float3  norm;
    float2  tex;
};
struct VtxPosTex {
    float3  pos;
    float2  tex;
};
enum VertexType {
    kVtxPosNormTex,
    kVtxPosTex,
    kVtxPosNormTanBitanTex,

    kNUM_VERTEX_TYPES
};
struct Light {
    float4  pos;
    float4  color;
};

typedef int32_t MeshID;
typedef int32_t TextureID;
typedef float LightType;
static const LightType  kDirectionalLight = 0.0f;
static const LightType  kPointLight = 1.0f;

class Render {
public:
    virtual ~Render() {}

    virtual void initialize(void* window) = 0;
    virtual void shutdown(void) = 0;

    virtual void render(void) = 0;
    virtual void resize(int width, int height) = 0;

    virtual MeshID load_mesh(const char* filename) = 0;
    virtual MeshID create_mesh(uint32_t vertex_count, VertexType vertex_type,
                               uint32_t index_count, size_t index_size,
                               const void* vertices, const void* indices) = 0;
    virtual TextureID load_texture(const char* filename) = 0;

    virtual void* window(void) = 0;

    virtual MeshID cube_mesh(void) = 0;
    virtual MeshID quad_mesh(void) = 0;
    virtual MeshID sphere_mesh(void) = 0;

    virtual void set_3d_view_matrix(const float4x4& view) = 0;
    virtual void set_2d_view_matrix(const float4x4& view) = 0;
    virtual void draw_3d(MeshID mesh, TextureID texture, TextureID normal_texture, const float4x4& transform) = 0;
    virtual void draw_2d(MeshID mesh, TextureID texture, const float4x4& transform) = 0;
    virtual void draw_light(const float4& light, const float4& color, LightType type) = 0;

    virtual void toggle_debug_graphics(void) = 0;
    virtual void toggle_deferred(void) = 0;

    static Render* create(void);
    static void destroy(Render* render);
private:
    static Render* _create_ogl(void);
};

/* @} */
#endif /* include guard */
