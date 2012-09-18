/*! @file renderer_forward.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/17/12 10:19 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __renderer_forward_renderer_forward__
#define __renderer_forward_renderer_forward__


#include "render_gl_helper.h"
#include "renderer.h"

class RendererForward : public Renderer {
public:

void init(void) {
}
void shutdown(void) {
}
void render(const float4x4& view_proj, GLuint frame_buffer,
            const Renderable* renderables, int num_renderables,
            const Light* lights, int num_lights)
{

}

private:

};


#endif /* include guard */
