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

#include "vec_math.h"

struct VtxPosNormTex {
    float3    pos;
    float3    norm;
    float2    tex;
};
struct VtxPosTex {
    float3    pos;
    float2    tex;
};
enum VertexType {
    kVtxPosNormTex,
    kVtxPosTex,

    kNUM_VERTEX_TYPES
};

class Render {
public:
    virtual ~Render() {}

    virtual void initialize(void* window) = 0;
    virtual void shutdown(void) = 0;

    virtual void render(void) = 0;
    virtual void resize(int width, int height) = 0;

    virtual void* window(void) = 0;

    static Render* create(void);
    static void destroy(Render* render);
private:
    static Render* _create_ogl(void);
};

/* @} */
#endif /* include guard */
