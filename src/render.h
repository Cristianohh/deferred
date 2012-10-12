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
#include "resource_manager.h"

#define MAX_LIGHTS (1024*3)

#ifndef ARRAYSIZE
    #define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))
#endif

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
    float3  pos;
    float   size;
    float3  dir;
    float   type;
    float3  color;  // A: type
    float   inner_cos;
    float   outer_cos;
    float3  _padding;
};

typedef float LightType;
static const LightType kDirectionalLight = 0.0f;
static const LightType kPointLight = 1.0f;
static const LightType kSpotLight = 2.0f;

struct Material {
    Resource    albedo_tex;
    Resource    normal_tex;
    Resource    specular_tex;
    float3      specular_color;
    float       specular_power;
    float       specular_coefficient;
};


class Render {
public:
    virtual ~Render() {}

    virtual void initialize(void* window) = 0;
    virtual void shutdown(void) = 0;

    virtual void render(void) = 0;
    virtual void resize(int width, int height) = 0;

    virtual Resource create_mesh(uint32_t vertex_count, VertexType vertex_type,
                                 uint32_t index_count, size_t index_size,
                                 const void* vertices, const void* indices) = 0;

    virtual void* window(void) = 0;

    virtual Resource cube_mesh(void) = 0;
    virtual Resource quad_mesh(void) = 0;
    virtual Resource sphere_mesh(void) = 0;

    virtual void set_3d_view_matrix(const float4x4& view) = 0;
    virtual void set_2d_view_matrix(const float4x4& view) = 0;
    virtual void draw_3d(Resource mesh, const Material* material, const float4x4& transform) = 0;
    virtual void draw_light(const Light& light) = 0;

    virtual void toggle_debug_graphics(void) = 0;
    virtual void toggle_deferred(void) = 0;

    static Render* create(void);
    static void destroy(Render* render);

    static Resource load_texture(const char* filename, void* ud);
    static void unload_texture(Resource resource, void* ud);
    static Resource load_mesh(const char* mesh, void* ud);
    static void unload_mesh(Resource resource, void* ud);

protected:
    virtual Resource _load_texture(const char* filename) = 0;
    virtual void _unload_texture(Resource resource) = 0;
    
    virtual Resource _load_mesh(const char* filename) = 0;
    virtual void _unload_mesh(Resource resource) = 0;

private:
    static Render* _create_ogl(void);
};

/* @} */
#endif /* include guard */
