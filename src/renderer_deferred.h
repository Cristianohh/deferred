/*! @file renderer_deferred.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/17/12 9:09 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __renderer_deferred__
#define __renderer_deferred__

#include "render_gl_helper.h"
#include "renderer.h"

// GBuffer format
//  [0] RGB: Albedo    
//  [1] RGB: WS Normal  A: Spec coefficient
//  [2] RGB: Spec Color A: Spec exponent
//  [3] R: Depth

class RendererDeferred : public Renderer {
public:

void init(void) {
    glGenFramebuffers(1, &_frame_buffer);
    glGenRenderbuffers(1, &_depth_buffer);
    glGenRenderbuffers(ARRAYSIZE(_gbuffer), _gbuffer);
    glGenTextures(ARRAYSIZE(_gbuffer_tex), _gbuffer_tex);

    { // Deferred Geometry
        GLuint vs = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/deferred/geometry.vsh");
        GLuint fs = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/deferred/geometry.fsh");
        _geom_program = _create_program(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        _geom_world_uniform = glGetUniformLocation(_geom_program, "kWorld");
        _geom_viewproj_uniform = glGetUniformLocation(_geom_program, "kViewProj");
        _geom_diffuse_uniform = glGetUniformLocation(_geom_program, "kDiffuseTex");
        _geom_normal_uniform = glGetUniformLocation(_geom_program, "kNormalTex");
    }    
    { // Deferred lighting
        GLuint vs = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/deferred/light.vsh");
        GLuint fs = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/deferred/light.fsh");
        _light_program = _create_program(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        _light_world_uniform = glGetUniformLocation(_light_program, "kWorld");
        _light_viewproj_uniform = glGetUniformLocation(_light_program, "kViewProj");
        _light_gbuffer_uniform = glGetUniformLocation(_light_program, "GBuffer");
        _light_light_uniform = glGetUniformLocation(_light_program, "kLight");
        _light_inv_viewproj_uniform = glGetUniformLocation(_light_program, "kInverseViewProj");
        _light_cam_pos_uniform = glGetUniformLocation(_light_program, "kCameraPosition");
    }
}
void shutdown(void) {
    glDeleteProgram(_geom_program);
    glDeleteProgram(_light_program);
    
    glDeleteRenderbuffers(ARRAYSIZE(_gbuffer), _gbuffer);
    glDeleteTextures(ARRAYSIZE(_gbuffer_tex), _gbuffer_tex);

    glDeleteRenderbuffers(1, &_depth_buffer);
    glDeleteFramebuffers(1, &_frame_buffer);
}
void resize(int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
    glViewport(0, 0, width, height);

    glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_buffer);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _gbuffer[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _gbuffer[0]);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _gbuffer[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA16F, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, _gbuffer[1]);
    CheckGLError();
    
    glBindRenderbuffer(GL_RENDERBUFFER, _gbuffer[2]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, _gbuffer[2]);
    CheckGLError();
    
    glBindRenderbuffer(GL_RENDERBUFFER, _gbuffer[3]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_RENDERBUFFER, _gbuffer[3]);
    CheckGLError();

    glBindTexture(GL_TEXTURE_2D, _gbuffer_tex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _gbuffer_tex[0], 0);
    CheckGLError();

    glBindTexture(GL_TEXTURE_2D, _gbuffer_tex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _gbuffer_tex[1], 0);
    CheckGLError();
    
    glBindTexture(GL_TEXTURE_2D, _gbuffer_tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, _gbuffer_tex[2], 0);
    CheckGLError();
    
    glBindTexture(GL_TEXTURE_2D, _gbuffer_tex[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, _gbuffer_tex[3], 0);
    CheckGLError();

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( status != GL_FRAMEBUFFER_COMPLETE) {
        debug_output("FBO initialization failed\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void render(const float4x4& view, const float4x4& proj, GLuint frame_buffer,
            const Renderable* renderables, int num_renderables,
            const Light* lights, int num_lights)
{
    float4x4 inv_view = float4x4inverse(&view);
    float4x4 view_proj = float4x4multiply(&inv_view, &proj);
    { // Render geometry
        glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
        glDrawBuffers(4, buffers);

        glUseProgram(_geom_program);

        glUniformMatrix4fv(_geom_viewproj_uniform, 1, GL_FALSE, (float*)&view_proj);
        for(int ii=0;ii<num_renderables;++ii) {
            const Renderable& r = renderables[ii];
            glUniformMatrix4fv(_geom_world_uniform, 1, GL_FALSE, (float*)&r.transform);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, r.texture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, r.normal_texture);
            glUniform1i(_geom_diffuse_uniform, 0);
            glUniform1i(_geom_normal_uniform, 1);
            glBindVertexArray(r.vao);
            _validate_program(_geom_program);
            glDrawElements(GL_TRIANGLES, (GLsizei)r.index_count, r.index_format, NULL);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    { // Render lights
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
        GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, buffers);
        
        glUseProgram(_light_program);
        for(int ii=0;ii<(int)ARRAYSIZE(_gbuffer_tex);++ii) {
            glActiveTexture(GL_TEXTURE0+ii);
            glBindTexture(GL_TEXTURE_2D, _gbuffer_tex[ii]);
        }
        int i[] = {0,1,2,3};
        glUniform1iv(_light_gbuffer_uniform, ARRAYSIZE(i), i);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(GL_FALSE);
        
        float4x4 inv_viewproj = float4x4inverse(&view_proj);
        glUniformMatrix4fv(_light_viewproj_uniform, 1, GL_FALSE, (float*)&view_proj);
        glUniformMatrix4fv(_light_inv_viewproj_uniform, 1, GL_FALSE, (float*)&inv_viewproj);
        glUniform3fv(_light_cam_pos_uniform, 1, (float*)&view.r3);
        for(int ii=0;ii<num_lights;++ii) {
            Light light = lights[ii];
            if(light.type == kDirectionalLight) {            
                glUniformMatrix4fv(_light_world_uniform, 1, GL_FALSE, (float*)&float4x4identity);
                glUniformMatrix4fv(_light_viewproj_uniform, 1, GL_FALSE, (float*)&float4x4identity);
                glUniform4fv(_light_light_uniform, 4, (float*)&light);
                
                glBindVertexArray(_fullscreen_mesh.vao);
                _validate_program(_light_program);
                glDrawElements(GL_TRIANGLES, (GLsizei)_fullscreen_mesh.index_count, _fullscreen_mesh.index_format, NULL);
                
                glUniformMatrix4fv(_light_viewproj_uniform, 1, GL_FALSE, (float*)&view_proj);
            } else if(light.type == kPointLight || light.type == kSpotLight) {
                float4x4 transform = float4x4Scale(light.size, light.size, light.size);
                transform.r3.x = light.pos.x;
                transform.r3.y = light.pos.y;
                transform.r3.z = light.pos.z;

                float3 v = float3subtract((float3*)&transform.r3, (float3*)&light.pos);
                if(float3lengthSq(&v) < light.size*light.size) {
                    glCullFace(GL_FRONT);
                }
                glUniformMatrix4fv(_light_world_uniform, 1, GL_FALSE, (float*)&transform);
                glUniform4fv(_light_light_uniform, 4, (float*)&light);
                glBindVertexArray(_sphere_mesh.vao);
                _validate_program(_light_program);
                glDrawElements(GL_TRIANGLES, (GLsizei)_sphere_mesh.index_count, _sphere_mesh.index_format, NULL);
                glCullFace(GL_BACK);
            }
        }

        glDepthMask(GL_TRUE);
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
void set_sphere_mesh(const Mesh& mesh) { _sphere_mesh = mesh; }
void set_fullscreen_mesh(const Mesh& mesh) { _fullscreen_mesh = mesh; }

GLuint gbuffer_tex(int index) { return _gbuffer_tex[index]; }

private:

Mesh    _sphere_mesh;
Mesh    _fullscreen_mesh;

GLuint  _geom_program;
GLuint  _geom_world_uniform;
GLuint  _geom_viewproj_uniform;
GLuint  _geom_diffuse_uniform;
GLuint  _geom_normal_uniform;

GLuint  _light_program;
GLuint  _light_world_uniform;
GLuint  _light_viewproj_uniform;
GLuint  _light_light_uniform;
GLuint  _light_gbuffer_uniform;
GLuint  _light_inv_viewproj_uniform;
GLuint  _light_cam_pos_uniform;

GLuint  _frame_buffer;
GLuint  _depth_buffer;

GLuint  _gbuffer[4];
GLuint  _gbuffer_tex[4];

};

#endif /* include guard */
