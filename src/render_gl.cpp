/*! @file render_gl.cpp
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "render.h"

#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #include <OpenGLES/ES2/gl.h>
    #elif TARGET_OS_MAC
        #include <OpenGL/gl3.h>
    #endif
    // TODO: flushing the buffer requires Obj-C in OS X. This is a hack to
    // include an Obj-C function
    extern "C" void _osx_flush_buffer(void* window);
#elif defined(_WIN32)
    #include <gl/glew.h>
    #include <gl/wglew.h>
    #include <gl/glcorearb.h>
    #pragma warning(disable:4127) /* Conditional expression is constant */
    #define snprintf _snprintf
#endif

#include <stddef.h>
#include <stdio.h>
#include "assert.h"
#include "stb_image.h"
#include "application.h"
#include "vec_math.h"
#include "geometry.h"
#include "render_gl_helper.h"


#define CheckGLError()                  \
    do {                                \
        GLenum _glError = glGetError(); \
        if(_glError != GL_NO_ERROR) {   \
            debug_output("OpenGL Error: %d\n", _glError);\
        }                               \
        assert(_glError == GL_NO_ERROR);\
    } while(__LINE__ == 0)

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

namespace {

enum UniformBufferType {
    kWorldTransformBuffer,
    kViewProjTransformBuffer,
    kLightBuffer,

    kNUM_UNIFORM_BUFFERS
};
enum VertexShaderType {
    kVSPos,

    kNUM_VERTEX_SHADERS
};
enum FragmentShaderType {
    kFSColor,

    kNUM_FRAGMENT_SHADERS
};
enum ProgramType {
    k3DProgram,
    k2DProgram,
    kDeferredProgram,
    kDeferredLightProgram,
    kDebugProgram,

    kNUM_PROGRAMS
};

struct LightBuffer
{
    Light   lights[MAX_LIGHTS];
    int     num_lights;
    int     _padding[3];
};

enum { kMAX_MESHES = 1024, kMAX_TEXTURES = 64, kMAX_RENDER_COMMANDS = 1024*8 };

static const VertexDescription kVertexDescriptions[kNUM_VERTEX_TYPES][8] =
{
    { // kVtxPosNormTex
        { 0, 3 },
        { 1, 3 },
        { 2, 2 },
        { 0, 0 },
    },
    { // kVtxPosTex
        { 0, 3 },
        { 1, 2 },
        { 0, 0 },
    }
};

}

class RenderGL : public Render {
public:

RenderGL()
    : _window(NULL)
    , _num_meshes(0)
    , _num_textures(0)
    , _num_2d_render_commands(0)
    , _num_3d_render_commands(0)
    , _3d_view(float4x4identity)
    , _2d_view(float4x4identity)
    , _width(128)
    , _height(128)
    , _deferred(1)
    , _debug(0)
{
    _light_buffer.num_lights = 0;
}
~RenderGL() {
}

void* window(void) { return _window; }

void initialize(void* window) {
#ifdef _WIN32
    HWND hWnd = (HWND)window;
    HDC hDC = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR   pfd = {0};
    pfd.nSize       = sizeof(pfd);
    pfd.nVersion    = 1;
    pfd.dwFlags     = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType  = PFD_TYPE_RGBA;
    pfd.cColorBits  = 24;
    pfd.cDepthBits  = 24;
    pfd.iLayerType  = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(hDC, &pfd);
    assert(pixel_format);

    int result = SetPixelFormat(hDC, pixel_format, &pfd);
    assert(result);

    // Create a dummy OpenGL 1.1 context to use for Glew initialization
    HGLRC first_GLRC = wglCreateContext(hDC);
    assert(first_GLRC);
    wglMakeCurrent(hDC, first_GLRC);
    CheckGLError();

    // Glew
    GLenum error = glewInit();
    CheckGLError();
    if(error != GLEW_OK) {
        debug_output("Glew Error: %s\n", glewGetErrorString(error));
    }
    assert(error == GLEW_OK);
    assert(wglewIsSupported("WGL_ARB_extensions_string") == 1);
    assert(wglewIsSupported("WGL_ARB_create_context") == 1);

    // Now create the real, OpenGL 3.2 context
    GLint attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,
        0,
    };
    HGLRC new_GLRC = wglCreateContextAttribsARB(hDC, 0, attributes);
    assert(new_GLRC);
    CheckGLError();

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(first_GLRC);
    wglMakeCurrent(hDC, new_GLRC);
    CheckGLError();
    _dc = hDC;

    // Disable vsync
    assert(wglewIsSupported("WGL_EXT_swap_control") == 1);
    wglSwapIntervalEXT(0);
    CheckGLError();
#endif
    _window = window;
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    _load_shaders();
    _create_uniform_buffers();
    _create_base_meshes();
    _clear(); // Clear once so the first present isn't garbage

    glFrontFace(GL_CW);
    CheckGLError();
    glDisable(GL_CULL_FACE);
    CheckGLError();
    glEnable(GL_DEPTH_TEST);
    CheckGLError();

    _create_gbuffer();
}
void shutdown(void) {
    _unload_shaders();
    glDeleteBuffers(kNUM_UNIFORM_BUFFERS, _uniform_buffers);
}
void render(void) {
    _present();
    _clear();
    if(_deferred) {
        _render_deferred();
        return;
    }

    // Setup the frame buffer
    if(1) {
        glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
        CheckGLError();
        glViewport(0, 0, _width, _height);
        CheckGLError();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        CheckGLError();
        glActiveTexture(GL_TEXTURE0);
        CheckGLError();
        //glEnable(GL_TEXTURE_2D);
        CheckGLError();
        GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, buffers);
        CheckGLError();
    }

    // 3D objects
    float4x4 view_proj = float4x4multiply(&_3d_view, &_perspective_projection);
    glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kViewProjTransformBuffer]);
    CheckGLError();
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &view_proj, GL_DYNAMIC_DRAW);
    CheckGLError();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    CheckGLError();

    glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kLightBuffer]);
    CheckGLError();
    glBufferData(GL_UNIFORM_BUFFER, sizeof(_light_buffer), &_light_buffer, GL_DYNAMIC_DRAW);
    CheckGLError();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    CheckGLError();

    glBindBufferBase(GL_UNIFORM_BUFFER, 2, _uniform_buffers[kLightBuffer]);
    CheckGLError();

    for(int ii=0; ii<_num_3d_render_commands; ++ii) {
        const RenderCommand& command = _3d_render_commands[ii];
        _draw_mesh(command.mesh, _textures[command.texture], command.transform, k3DProgram);
    }

    if(_debug) {
        glDisable(GL_CULL_FACE);
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        CheckGLError();
        for(int ii=0;ii<_light_buffer.num_lights;++ii) {
            const float4& light = _light_buffer.lights[ii].pos;
            float4x4 transform = float4x4Scale(light.w, light.w, light.w);
            transform.r3.x = light.x;
            transform.r3.y = light.y;
            transform.r3.z = light.z;
            _draw_mesh(_sphere_mesh, 0, transform, kDebugProgram);
        }
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        CheckGLError();
        glEnable(GL_CULL_FACE);
    }

    glDisable(GL_DEPTH_TEST);

    if(1) {
        // Render the scene from the render target
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kViewProjTransformBuffer]);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &float4x4identity, GL_DYNAMIC_DRAW);
        glDisable(GL_CULL_FACE);
        _draw_mesh(_quad_mesh, _color_texture, float4x4Scale(2.0f, -2.0f, 1.0f), k2DProgram);
        glEnable(GL_CULL_FACE);
    }

    // 2D gui objects
    view_proj = float4x4multiply(&_2d_view, &_orthographic_projection);
    glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kViewProjTransformBuffer]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &view_proj, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    for(int ii=0; ii<_num_2d_render_commands; ++ii) {
        const RenderCommand& command = _2d_render_commands[ii];
        _draw_mesh(command.mesh, command.texture, command.transform, k2DProgram);
    }
    
    glEnable(GL_DEPTH_TEST);
    
    _num_3d_render_commands = _num_2d_render_commands = 0;
    _light_buffer.num_lights = 0;
}
void _render_deferred(void) {
    float4x4 view_proj = float4x4multiply(&_3d_view, &_perspective_projection);
    {
        static int view_proj_loc = glGetUniformLocation(_deferred_program, "kViewProj");
        static int world_loc = glGetUniformLocation(_deferred_program, "kWorld");

        // Setup the frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(ARRAYSIZE(buffers), buffers);

        // 3D objects
        glUseProgram(_deferred_program);
        glValidateProgram(_deferred_program);

        glUniformMatrix4fv(view_proj_loc, 1, GL_FALSE, (float*)&view_proj);
        for(int ii=0; ii<_num_3d_render_commands; ++ii) {
            const RenderCommand& command = _3d_render_commands[ii];
            const Mesh& mesh = _meshes[command.mesh];

            glUniformMatrix4fv(world_loc, 1, GL_FALSE, (float*)&command.transform);
            glBindTexture(GL_TEXTURE_2D, _textures[command.texture]);
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, mesh.index_format, NULL);
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }
    {        
        static int light_loc = glGetUniformLocation(_deferred_light_program, "kLight");
        static int view_proj_loc = glGetUniformLocation(_deferred_light_program, "kViewProj");
        static int world_loc = glGetUniformLocation(_deferred_light_program, "kWorld");
        static int loc = glGetUniformLocation(_deferred_light_program, "GBuffer");

        // Render the scene from the render target
        glUseProgram(_deferred_light_program);
        CheckGLError();
        _validate_program(_deferred_light_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _color_texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _normal_texture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, _position_texture);

        int i[] = {0,1,2};
        glUniform1iv(loc, 3, i);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
            
        glUniformMatrix4fv(view_proj_loc, 1, GL_FALSE, (float*)&view_proj);

        const Mesh& mesh = _meshes[_sphere_mesh];
        for(int ii=0;ii<_light_buffer.num_lights;++ii) {
            const Light& light = _light_buffer.lights[ii];
            float4x4 transform = float4x4Scale(light.pos.w, light.pos.w, light.pos.w);
            transform.r3.x = light.pos.x;
            transform.r3.y = light.pos.y;
            transform.r3.z = light.pos.z;

            glUniform4fv(light_loc, 2, (float*)&light);
            glUniformMatrix4fv(world_loc, 1, GL_FALSE, (float*)&transform);
            
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, mesh.index_format, NULL);
        }
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_BLEND);

        if(_debug) {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kViewProjTransformBuffer]);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &float4x4identity, GL_DYNAMIC_DRAW);
            float4x4 t = float4x4Scale(1.0f, -1.0f, 1.0f);
            t.r3.x = -0.5f;
            t.r3.y = -0.5f;
            _draw_mesh(_quad_mesh, _color_texture, t, k2DProgram);
            t.r3.y = 0.5f;
            _draw_mesh(_quad_mesh, _normal_texture, t, k2DProgram);
            t.r3.x = 0.5f;
            _draw_mesh(_quad_mesh, _position_texture, t, k2DProgram);
        }
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    _num_3d_render_commands = _num_2d_render_commands = 0;
    _light_buffer.num_lights = 0;
}
void resize(int width, int height) {
    _width = width;
    _height = height;
    glViewport(0, 0, width, height);
    _orthographic_projection = float4x4OrthographicOffCenterLH(0, width, height, 0, 0.0f, 1.0f);
    _perspective_projection = float4x4PerspectiveFovLH(DegToRad(50.0f), width/(float)height, 0.1f, 10000.0f);

    // Resize render targets
    _resize_gbuffer();
}

MeshID create_mesh(uint32_t vertex_count, VertexType vertex_type,
                   uint32_t index_count, size_t index_size,
                   const void* vertices, const void* indices) {
    MeshID id = _num_meshes++;
    _meshes[id] = _create_mesh(vertex_count, (vertex_type == kVtxPosNormTex) ? sizeof(VtxPosNormTex) : sizeof(VtxPosTex),
                              index_count, index_size,
                              vertices, indices,
                              kVertexDescriptions[vertex_type]);
    return id;
}

MeshID cube_mesh(void) { return _cube_mesh; }
MeshID quad_mesh(void) { return _quad_mesh; }
MeshID sphere_mesh(void) { return _sphere_mesh; }
void toggle_debug_graphics(void) { _debug = !_debug; }
void toggle_deferred(void) {
    _deferred = !_deferred;
    if(_deferred)
        debug_output("Deferred\n");
    else
        debug_output("Forward\n");
}

void set_3d_view_matrix(const float4x4& view) {
    _3d_view = view;
}
void set_2d_view_matrix(const float4x4& view) {
    _2d_view = view;
}
void draw_3d(MeshID mesh, TextureID texture, const float4x4& transform) {
    RenderCommand& command = _3d_render_commands[_num_3d_render_commands++];
    command.mesh = mesh;
    command.transform = transform;
    command.texture = texture;
}
void draw_2d(MeshID mesh, TextureID texture, const float4x4& transform) {
    RenderCommand& command = _2d_render_commands[_num_2d_render_commands++];
    command.mesh = mesh;
    command.transform = transform;
    command.texture = texture;
}

void draw_light(const float4& light, const float4& color) {
    int index = _light_buffer.num_lights++;
    _light_buffer.lights[index].pos = light;
    _light_buffer.lights[index].color = color;
}
TextureID load_texture(const char* filename) {
    int width, height, components;
    GLenum format;
    void* tex_data = stbi_load(filename, &width, &height, &components, 0);

    GLuint texture;
    glGenTextures(1, &texture);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, texture);
    CheckGLError();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    switch(components)
    {
    case 4:
        format = GL_RGBA;
        components = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        components = GL_RGB;
        break;
    default: assert(0);
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0, format, GL_UNSIGNED_BYTE, tex_data);
    CheckGLError();
    glGenerateMipmap(GL_TEXTURE_2D);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGLError();

    stbi_image_free(tex_data);

    _textures[_num_textures] = texture;
    return _num_textures++;
}

MeshID load_mesh(const char* filename) {
    uint32_t nVertexStride;
    uint32_t nVertexCount;
    uint32_t nIndexSize;
    uint32_t nIndexCount;
    char* pData;

    FILE* pFile = fopen( filename, "rb" );
    fread( &nVertexStride, sizeof( nVertexStride ), 1, pFile );
    fread( &nVertexCount, sizeof( nVertexCount ), 1, pFile );
    fread( &nIndexSize, sizeof( nIndexSize ), 1, pFile );
    nIndexSize = (nIndexSize == 32 ) ? 4 : 2;
    fread( &nIndexCount, sizeof( nIndexCount ), 1, pFile );

    pData   = new char[ (nVertexStride * nVertexCount) + (nIndexCount * nIndexSize) ];

    fread( pData, nVertexStride, nVertexCount, pFile );
    fread( pData + (nVertexStride * nVertexCount), nIndexSize, nIndexCount, pFile );
    fclose( pFile );

    MeshID mesh = create_mesh(nVertexCount, kVtxPosNormTex, nIndexCount, nIndexSize, pData, pData + (nVertexStride * nVertexCount));
    delete [] pData;

    return mesh;
}
private:
void _clear(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGLError();
}
void _present(void) {
#ifdef __APPLE__
    _osx_flush_buffer(_window);
#elif defined(_WIN32)
    SwapBuffers(_dc);
    CheckGLError();
#endif
}
void _load_shaders(void) {
    // 3D
    GLuint vs_3d = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/3D.vsh");
    GLuint fs_3d = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/3D.fsh");
    _programs[k3DProgram] = _create_program(vs_3d, fs_3d);
    glDeleteShader(vs_3d);
    glDeleteShader(fs_3d);

    GLuint buffer_index = glGetUniformBlockIndex(_programs[k3DProgram], "PerFrame");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[k3DProgram], buffer_index, 0);
    buffer_index = glGetUniformBlockIndex(_programs[k3DProgram], "PerObject");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[k3DProgram], buffer_index, 1);
    buffer_index = glGetUniformBlockIndex(_programs[k3DProgram], "LightBuffer");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[k3DProgram], buffer_index, 2);

    // Deferred
    GLuint vs_deferred = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/deferred.vsh");
    GLuint fs_deferred = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/deferred.fsh");
    //_programs[kDeferredProgram] = _create_program(vs_deferred, fs_deferred);
    _deferred_program = _create_program(vs_deferred, fs_deferred);
    glDeleteShader(vs_deferred);
    glDeleteShader(fs_deferred);

//    buffer_index = glGetUniformBlockIndex(_programs[kDeferredProgram], "PerFrame");
//    assert(buffer_index != GL_INVALID_INDEX);
//    glUniformBlockBinding(_programs[kDeferredProgram], buffer_index, 0);
//    buffer_index = glGetUniformBlockIndex(_programs[kDeferredProgram], "PerObject");
//    assert(buffer_index != GL_INVALID_INDEX);
//    glUniformBlockBinding(_programs[kDeferredProgram], buffer_index, 1);

    // Deferred light
    vs_deferred = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/deferred_light.vsh");
    fs_deferred = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/deferred_light.fsh");
    _deferred_light_program = _create_program(vs_deferred, fs_deferred);
    glDeleteShader(vs_deferred);
    glDeleteShader(fs_deferred);

    // 2D
    GLuint vs_2d = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/2D.vsh");
    GLuint fs_2d = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/2D.fsh");
    _programs[k2DProgram] = _create_program(vs_2d, fs_2d);
    glDeleteShader(vs_2d);
    glDeleteShader(fs_2d);

    buffer_index = glGetUniformBlockIndex(_programs[k2DProgram], "PerFrame");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[k2DProgram], buffer_index, 0);
    buffer_index = glGetUniformBlockIndex(_programs[k2DProgram], "PerObject");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[k2DProgram], buffer_index, 1);

    
    // Debug
    GLuint vs_debug = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/debug.vsh");
    GLuint fs_debug = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/debug.fsh");
    _programs[kDebugProgram] = _create_program(vs_debug, fs_debug);
    glDeleteShader(vs_debug);
    glDeleteShader(fs_debug);

    buffer_index = glGetUniformBlockIndex(_programs[kDebugProgram], "PerFrame");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[kDebugProgram], buffer_index, 0);
    buffer_index = glGetUniformBlockIndex(_programs[kDebugProgram], "PerObject");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[kDebugProgram], buffer_index, 1);
        CheckGLError();
}
void _unload_shaders(void) {
    for(int ii=0;ii<kNUM_VERTEX_SHADERS;++ii)
        glDeleteShader(_vertex_shaders[ii]);
    for(int ii=0;ii<kNUM_FRAGMENT_SHADERS;++ii)
        glDeleteShader(_fragment_shaders[ii]);
}
void _create_uniform_buffers(void) {
    _uniform_buffers[kViewProjTransformBuffer] = _create_buffer(GL_UNIFORM_BUFFER, sizeof(float4x4), &float4x4identity);
    assert(_uniform_buffers[kViewProjTransformBuffer]);
    _uniform_buffers[kWorldTransformBuffer] = _create_buffer(GL_UNIFORM_BUFFER, sizeof(float4x4), &float4x4identity);
    assert(_uniform_buffers[kWorldTransformBuffer]);
    
    _uniform_buffers[kLightBuffer] = _create_buffer(GL_UNIFORM_BUFFER, sizeof(LightBuffer), &_light_buffer);
    assert(_uniform_buffers[kLightBuffer]);
    
}
void _create_base_meshes(void) {
    _cube_mesh = create_mesh(ARRAYSIZE(kCubeVertices), kVtxPosNormTex,
                             ARRAYSIZE(kCubeIndices), sizeof(kCubeIndices[0]),
                             kCubeVertices, kCubeIndices);
    _quad_mesh = create_mesh(ARRAYSIZE(kQuadVertices), kVtxPosTex,
                             ARRAYSIZE(kQuadIndices), sizeof(kQuadIndices[0]),
                             kQuadVertices, kQuadIndices);
    _sphere_mesh = load_mesh("assets/sphere.mesh");
}
void _draw_mesh(MeshID mesh_id, GLuint texture, const float4x4& transform, int program_id) {
    const Mesh& mesh = _meshes[mesh_id];
    glBindVertexArray(mesh.vao);
    _validate_program(_programs[program_id]);
    glUseProgram(_programs[program_id]);
    glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kWorldTransformBuffer]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &transform, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, _uniform_buffers[kViewProjTransformBuffer]);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, _uniform_buffers[kWorldTransformBuffer]);
    glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, mesh.index_format, NULL);
}
void _resize_gbuffer(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
    glViewport(0, 0, _width, _height);

    glBindRenderbuffer(GL_RENDERBUFFER, _color_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _color_buffer);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _normal_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, _normal_buffer);
    CheckGLError();
    
    glBindRenderbuffer(GL_RENDERBUFFER, _position_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, _position_buffer);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_buffer);
    CheckGLError();

    glBindTexture(GL_TEXTURE_2D, _color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _color_texture, 0);
    CheckGLError();

    glBindTexture(GL_TEXTURE_2D, _normal_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _normal_texture, 0);
    CheckGLError();
    
    glBindTexture(GL_TEXTURE_2D, _position_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, _position_texture, 0);
    CheckGLError();

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( status != GL_FRAMEBUFFER_COMPLETE) {
        debug_output("FBO initialization failed\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void _create_gbuffer(void) {

    glGenFramebuffers(1, &_frame_buffer);
    glGenRenderbuffers(1, &_color_buffer);
    glGenRenderbuffers(1, &_depth_buffer);
    glGenRenderbuffers(1, &_normal_buffer);
    glGenRenderbuffers(1, &_position_buffer);

    glGenTextures(1, &_color_texture);
    glGenTextures(1, &_normal_texture);
    glGenTextures(1, &_position_texture);
}

private:

#ifdef _WIN32
HDC     _dc;
#endif
void*   _window;
GLuint  _vertex_shaders[kNUM_VERTEX_SHADERS];
GLuint  _fragment_shaders[kNUM_FRAGMENT_SHADERS];
GLuint  _programs[kNUM_PROGRAMS];
GLuint  _uniform_buffers[kNUM_UNIFORM_BUFFERS];

Mesh    _meshes[kMAX_MESHES];
int     _num_meshes;
MeshID  _cube_mesh;
MeshID  _quad_mesh;
MeshID  _sphere_mesh;

GLuint  _textures[kMAX_TEXTURES];
int     _num_textures;

float4x4    _perspective_projection;
float4x4    _3d_view;
float4x4    _orthographic_projection;
float4x4    _2d_view;

RenderCommand   _3d_render_commands[kMAX_RENDER_COMMANDS];
int             _num_3d_render_commands;
RenderCommand   _2d_render_commands[kMAX_RENDER_COMMANDS];
int             _num_2d_render_commands;

LightBuffer _light_buffer;

int _debug;
int _deferred;

GLuint  _frame_buffer;
GLuint  _depth_buffer;

GLuint  _color_buffer;
GLuint  _normal_buffer;
GLuint  _position_buffer;

GLuint  _color_texture;
GLuint  _normal_texture;
GLuint  _position_texture;

GLuint  _deferred_light_program;
GLuint  _deferred_program;

int     _width;
int     _height;

};

/*
 * External
 */
Render* Render::_create_ogl(void) {
    return new RenderGL;
}
