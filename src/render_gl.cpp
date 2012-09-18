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

#include "renderer.h"
#include "renderer_deferred.h"
#include "renderer_forward.h"

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
    _create_base_meshes();
    _clear(); // Clear once so the first present isn't garbage

    glFrontFace(GL_CW);
    CheckGLError();
    glEnable(GL_CULL_FACE);
    CheckGLError();
    glEnable(GL_DEPTH_TEST);
    CheckGLError();

    _create_framebuffer();

    _forward_renderer.init();
    _deferred_renderer.init();
    _deferred_renderer.set_sphere_mesh(_meshes[_sphere_mesh]);
}
void shutdown(void) {
    _forward_renderer.shutdown();
    _deferred_renderer.shutdown();
}
void render(void) {
    _present();
    _clear();

    glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
    _clear();

    float4x4 view_proj = float4x4multiply(&_3d_view, &_perspective_projection);

    Renderable renderables[1024];
    for(int ii=0;ii<_num_3d_render_commands;++ii) {
        const RenderCommand& command = _3d_render_commands[ii];
        const Mesh& mesh = _meshes[command.mesh];

        Renderable& r = renderables[ii];
        r.texture = _textures[command.texture];
        r.vao = mesh.vao;
        r.index_count = mesh.index_count;
        r.index_format = mesh.index_format;
        r.transform = command.transform;
    }

    if(_deferred)
        _deferred_renderer.render(view_proj, _frame_buffer,
                                  renderables, _num_3d_render_commands,
                                  _light_buffer.lights, _light_buffer.num_lights);
    else
        _forward_renderer.render(view_proj, _frame_buffer,
                                 renderables, _num_3d_render_commands,
                                 _light_buffer.lights, _light_buffer.num_lights);

    // Render the scene from the render target
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    _render_fullscreen(_color_texture);
    
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
    _resize_framebuffer();
    _forward_renderer.resize(width, height);
    _deferred_renderer.resize(width, height);
}
void _render_fullscreen(GLuint texture) {
    glUseProgram(_fullscreen_program);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(_meshes[_fullscreen_quad_mesh].vao);
    _validate_program(_fullscreen_program);
    glDrawElements(GL_TRIANGLES, (GLsizei)_meshes[_fullscreen_quad_mesh].index_count, _meshes[_fullscreen_quad_mesh].index_format, NULL);
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
    
    GLuint vs = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/2D.vsh");
    GLuint fs = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/2D.fsh");
    _2d_program = _create_program(vs, fs);
    glDeleteShader(vs);

    vs = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/2D_fullscreen.vsh");
    _fullscreen_program = _create_program(vs, fs);
    glDeleteShader(fs);

    _2d_viewproj_uniform = glGetUniformLocation(_2d_program, "kViewProj");
    _2d_world_uniform = glGetUniformLocation(_2d_program, "kWorld");
}
void _create_base_meshes(void) {
    _cube_mesh = create_mesh(ARRAYSIZE(kCubeVertices), kVtxPosNormTex,
                             ARRAYSIZE(kCubeIndices), sizeof(kCubeIndices[0]),
                             kCubeVertices, kCubeIndices);
    _quad_mesh = create_mesh(ARRAYSIZE(kQuadVertices), kVtxPosTex,
                             ARRAYSIZE(kQuadIndices), sizeof(kQuadIndices[0]),
                             kQuadVertices, kQuadIndices);
    _fullscreen_quad_mesh = create_mesh(ARRAYSIZE(kFullscreenQuadVertices), kVtxPosTex,
                                        ARRAYSIZE(kQuadIndices), sizeof(kQuadIndices[0]),
                                        kFullscreenQuadVertices, kQuadIndices);
    _sphere_mesh = load_mesh("assets/sphere.mesh");
}
void _resize_framebuffer(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
    glViewport(0, 0, _width, _height);

    glBindRenderbuffer(GL_RENDERBUFFER, _color_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _color_buffer);
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

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( status != GL_FRAMEBUFFER_COMPLETE) {
        debug_output("FBO initialization failed\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void _create_framebuffer(void) {

    glGenFramebuffers(1, &_frame_buffer);
    glGenRenderbuffers(1, &_color_buffer);
    glGenRenderbuffers(1, &_depth_buffer);

    glGenTextures(1, &_color_texture);
}

private:

#ifdef _WIN32
HDC     _dc;
#endif
void*   _window;

Mesh    _meshes[kMAX_MESHES];
int     _num_meshes;
MeshID  _cube_mesh;
MeshID  _quad_mesh;
MeshID  _sphere_mesh;
MeshID  _fullscreen_quad_mesh;

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
GLuint  _color_buffer;
GLuint  _depth_buffer;

GLuint  _color_texture;

GLuint  _fullscreen_program;

GLuint  _2d_program;
GLuint  _2d_world_uniform;
GLuint  _2d_viewproj_uniform;

int     _width;
int     _height;

RendererDeferred    _deferred_renderer;
RendererForward     _forward_renderer;

};

/*
 * External
 */
Render* Render::_create_ogl(void) {
    return new RenderGL;
}
