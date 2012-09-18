/*! @file renderer.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/17/12 10:15 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __renderer_renderer__
#define __renderer_renderer__


class Renderer {
public:

virtual ~Renderer() { }
virtual void init(void) = 0;
virtual void shutdown(void) = 0;
virtual void resize(int, int) { }
virtual void render(const float4x4& view, const float4x4& proj, GLuint frame_buffer,
                    const Renderable* renderables, int num_renderables,
                    const Light* lights, int num_lights) = 0;

};


#endif /* include guard */
