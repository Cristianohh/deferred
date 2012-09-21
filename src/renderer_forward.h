/*! @file renderer_forward.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/17/12 10:19 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __renderer_forward_renderer_forward__
#define __renderer_forward_renderer_forward__

#include <memory.h>
#include "render_gl_helper.h"
#include "renderer.h"

class RendererForward : public Renderer {
public:

void init(void) {
    GLuint vs = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/forward/3D.vsh");
    GLuint fs = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/forward/3D.fsh");
    _program = _create_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    _world_uniform = glGetUniformLocation(_program, "kWorld");
    _viewproj_uniform = glGetUniformLocation(_program, "kViewProj");
    _diffuse_uniform = glGetUniformLocation(_program, "kDiffuseTex");
    _camera_position_uniform = glGetUniformLocation(_program, "kCameraPosition");
    _light_buffer_uniform = glGetUniformBlockIndex(_program, "LightBuffer");

    _light_buffer_buffer = _create_buffer(GL_UNIFORM_BUFFER, sizeof(LightBuffer), &_light_buffer);
}
void shutdown(void) {
    glDeleteProgram(_program);
    glDeleteBuffers(1, &_light_buffer_buffer);
}
void render(const float4x4& view, const float4x4& proj, GLuint frame_buffer,
            const Renderable* renderables, int num_renderables,
            const Light* lights, int num_lights)
{
    float4x4 inv_view = float4x4inverse(&view);
    float4x4 view_proj = float4x4multiply(&inv_view, &proj);
    memcpy(_light_buffer.lights, lights, num_lights*sizeof(Light));
    _light_buffer.num_lights = num_lights;

    glBindBuffer(GL_UNIFORM_BUFFER, _light_buffer_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(_light_buffer), &_light_buffer, GL_DYNAMIC_DRAW);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    glUseProgram(_program);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, _light_buffer_buffer);
    glUniformMatrix4fv(_viewproj_uniform, 1, GL_FALSE, (float*)&view_proj);
    glUniform3fv(_camera_position_uniform, 1, (float*)&view.r3);
    for(int ii=0;ii<num_renderables;++ii) {
        const Renderable& r = renderables[ii];
        glUniformMatrix4fv(_world_uniform, 1, GL_FALSE, (float*)&r.transform);
        glBindTexture(GL_TEXTURE_2D, r.texture);
        glBindVertexArray(r.vao);
        _validate_program(_program);
        glDrawElements(GL_TRIANGLES, (GLsizei)r.index_count, r.index_format, NULL);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

private:

GLuint  _program;
GLuint  _viewproj_uniform;
GLuint  _world_uniform;
GLuint  _diffuse_uniform;
GLuint  _camera_position_uniform;

GLuint  _light_buffer_uniform;
GLuint  _light_buffer_buffer;

LightBuffer _light_buffer;

};


#endif /* include guard */
