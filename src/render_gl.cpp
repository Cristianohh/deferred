/*! @file render_gl.cpp
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "render.h"
#include <stddef.h>
#include <stdio.h>
#include "assert.h"
#include "stb_image.h"
#include "application.h"
#include "vec_math.h"

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

struct Mesh {
    GLuint      vao;
    GLuint      vertex_buffer;
    GLuint      index_buffer;
    uint32_t    index_count;
    GLenum      index_format;
};

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

struct RenderCommand {
    float4x4    transform;
    MeshID      mesh;
    TextureID   texture;
};
struct LightBuffer
{
    float4  lights[24];
    int     num_lights;
    int     _padding[3];
};

enum { kMAX_MESHES = 1024, kMAX_TEXTURES = 64, kMAX_RENDER_COMMANDS = 1024*8 };

static const struct VertexDescription {
    uint32_t slot;
    int count;
} kVertexDescriptions[kNUM_VERTEX_TYPES][8] =
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

static const VtxPosNormTex kCubeVertices[] =
{
    /* Top */
    { {-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f} },
    { { 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f} },
    { { 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f} },
    { {-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f} },

    /* Bottom */
    { {-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f} },
    { { 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f} },
    { { 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f} },
    { {-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f} },

    /* Left */
    { {-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
    { {-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
    { {-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },
    { {-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },

    /* Right */
    { { 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
    { { 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
    { { 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },
    { { 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },

    /* Front */
    { {-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f} },
    { { 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f} },
    { { 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f} },
    { {-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f} },

    /* Back */
    { {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f} },
    { { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f} },
    { { 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f} },
    { {-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f} },
};
static const unsigned short kCubeIndices[] =
{
    3,1,0,
    2,1,3,

    6,4,5,
    7,4,6,

    11,9,8,
    10,9,11,

    14,12,13,
    15,12,14,

    19,17,16,
    18,17,19,

    22,20,21,
    23,20,22
};

static const VtxPosTex kQuadVertices[] =
{
    { {-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f} },
    { { 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f} },
    { { 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f} },
    { {-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f} },
};
static const unsigned short kQuadIndices[] =
{
    3,1,0,
    2,1,3,
};


GLuint _compile_shader(GLenum shader_type, const char* filename) {
    MessageBoxResult result = kMBOK;
    GLint status = GL_TRUE;
    GLuint shader = 0;
    CheckGLError();
    do {
        result = kMBOK;
        GLchar buffer[1024*64]; // 64k should be a safe size
        FILE* file = fopen(filename, "rt");
        assert(file);
        size_t bytes_read = fread(buffer, sizeof(GLchar), sizeof(buffer), file);
        buffer[bytes_read] = '\0';
        fclose(file);

        shader = glCreateShader(shader_type);
        const GLchar* p = buffer;
        glShaderSource(shader, 1, &p, NULL);
        CheckGLError();
        glCompileShader(shader);
        CheckGLError();
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        CheckGLError();
        if(status == GL_FALSE) {
            char printBuffer[1024*16];
            char statusBuffer[1024*16];
            glGetShaderInfoLog(shader, sizeof(statusBuffer), NULL, statusBuffer);
            CheckGLError();

            snprintf(printBuffer, sizeof(printBuffer), "Error compiling shader: %s\n%s", filename, statusBuffer);
            result = message_box("Shader Compile Error", printBuffer);
            glDeleteShader(shader);
            CheckGLError();
            shader = 0;
        }
    } while(result == kMBRetry);

    return shader;
}
GLuint _create_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLint status = GL_TRUE;
    GLuint program = glCreateProgram();
    CheckGLError();
    glAttachShader(program, vertex_shader);
    CheckGLError();
    glAttachShader(program, fragment_shader);
    CheckGLError();
    glLinkProgram(program);
    CheckGLError();
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    CheckGLError();
    if(status == GL_FALSE) {
        char printBuffer[1024];
        char statusBuffer[1024];
        glGetProgramInfoLog(program, sizeof(statusBuffer), NULL, statusBuffer);
        CheckGLError();

        snprintf(printBuffer, sizeof(printBuffer), "Link error: %s", statusBuffer);
        debug_output("%s\n", printBuffer);
        glDeleteProgram(program);
        CheckGLError();
        program = 0;
    }
    glUseProgram(0);
    return program;
}
void _validate_program(GLuint program) {
    GLint status = GL_TRUE;
    glValidateProgram(program);
    CheckGLError();
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    CheckGLError();
    if(status == GL_FALSE) {
        char printBuffer[1024];
        char statusBuffer[1024];
        glGetProgramInfoLog(program, sizeof(statusBuffer), NULL, statusBuffer);
        CheckGLError();

        snprintf(printBuffer, sizeof(printBuffer), "Link error: %s", statusBuffer);
        debug_output("%s\n", printBuffer);
    }
}
GLuint _create_buffer(GLenum type, size_t size, const void* data) {\
    GLuint buffer;
    glGenBuffers(1, &buffer);
    CheckGLError();
    assert(buffer);
    glBindBuffer(type, buffer);
    CheckGLError();
    glBufferData(type, (GLsizeiptr)size, data, GL_DYNAMIC_DRAW);
    CheckGLError();
    glBindBuffer(type, 0);
    CheckGLError();
    return buffer;
}
static Mesh _create_mesh(uint32_t vertex_count, VertexType vertex_type,
                         uint32_t index_count, size_t index_size,
                         const void* vertices, const void* indices)
{
    const VertexDescription* vertex_desc = kVertexDescriptions[vertex_type];
    size_t vertex_size = (vertex_type == kVtxPosNormTex) ? sizeof(VtxPosNormTex) : sizeof(VtxPosTex);
    GLuint vertex_buffer = _create_buffer(GL_ARRAY_BUFFER, vertex_count*vertex_size, vertices);
    GLuint index_buffer = _create_buffer(GL_ELEMENT_ARRAY_BUFFER, index_size*index_count, indices);
    intptr_t offset = 0;

    GLuint vao;
    glGenVertexArrays(1, &vao);
    assert(vao);
    CheckGLError();
    glBindVertexArray(vao);
    CheckGLError();
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    CheckGLError();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    CheckGLError();
    while(vertex_desc && vertex_desc->count) {
        glEnableVertexAttribArray(vertex_desc->slot);
        CheckGLError();
        glVertexAttribPointer(vertex_desc->slot, vertex_desc->count, GL_FLOAT, GL_FALSE, (GLsizei)vertex_size, (void*)offset);
        CheckGLError();
        offset += sizeof(float) * (uint32_t)vertex_desc->count;
        ++vertex_desc;
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CheckGLError();

    Mesh mesh;
    mesh.index_buffer = index_buffer;
    mesh.vertex_buffer = vertex_buffer;
    mesh.index_count = index_count;
    mesh.index_format = (index_size == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    mesh.vao = vao;
    return mesh;
}

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
    , _width(1440)
    , _height(900)
    , _deferred(0)
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


#if 1
    glGenFramebuffers(1, &_frame_buffer);
    glGenRenderbuffers(1, &_color_buffer);
    glGenRenderbuffers(1, &_depth_buffer);
    glGenRenderbuffers(1, &_normal_buffer);
    glGenRenderbuffers(1, &_position_buffer);

    glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _color_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    CheckGLError();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _color_buffer);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _normal_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    CheckGLError();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, _normal_buffer);
    CheckGLError();
    
    glBindRenderbuffer(GL_RENDERBUFFER, _position_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, _width, _height);
    CheckGLError();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, _position_buffer);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _width, _height);
    CheckGLError();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_buffer);
    CheckGLError();

    glGenTextures(1, &_color_texture);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _color_texture);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _color_texture, 0);
    CheckGLError();

    glGenTextures(1, &_normal_texture);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _normal_texture);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _normal_texture, 0);
    CheckGLError();
    

    glGenTextures(1, &_position_texture);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _position_texture);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, _position_texture, 0);
    CheckGLError();

    glGenTextures(1, &_depth_texture);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _depth_texture);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_texture, 0);
    CheckGLError();

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( status != GL_FRAMEBUFFER_COMPLETE) {
        debug_output("FBO initialization failed\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    _textures[_num_textures] = _color_texture;
    _color_texture = _num_textures++;
    _textures[_num_textures] = _normal_texture;
    _normal_texture = _num_textures++;
    _textures[_num_textures] = _depth_texture;
    _depth_texture = _num_textures++;
    _textures[_num_textures] = _position_texture;
    _position_texture = _num_textures++;
#endif
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
    if(1 || _deferred) {
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
        _draw_mesh(command.mesh, command.texture, command.transform, k3DProgram);
    }

    if(_debug) {
        glDisable(GL_CULL_FACE);
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        CheckGLError();
        for(int ii=0;ii<_light_buffer.num_lights;++ii) {
            const float4& light = _light_buffer.lights[ii];
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

    if(1 || _deferred) {
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
    // Setup the frame buffer
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
    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(ARRAYSIZE(buffers), buffers);
    CheckGLError();

    // 3D objects
    float4x4 view_proj = float4x4multiply(&_3d_view, &_perspective_projection);
    glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kViewProjTransformBuffer]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &view_proj, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    for(int ii=0; ii<_num_3d_render_commands; ++ii) {
        const RenderCommand& command = _3d_render_commands[ii];
        _draw_mesh(command.mesh, command.texture, command.transform, kDeferredProgram);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    if(_debug) {
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

    } else {
        // Render the scene from the render target
        glUseProgram(_programs[kDeferredLightProgram]);
        CheckGLError();
        glActiveTexture(GL_TEXTURE0);
        CheckGLError();
        glBindTexture(GL_TEXTURE_2D, _textures[_color_texture]);
        CheckGLError();
        glActiveTexture(GL_TEXTURE1);
        CheckGLError();
        glBindTexture(GL_TEXTURE_2D, _textures[_normal_texture]);
        CheckGLError();
        glActiveTexture(GL_TEXTURE2);
        CheckGLError();
        glBindTexture(GL_TEXTURE_2D, _textures[_position_texture]);
        CheckGLError();
        int loc = glGetUniformLocation(_programs[kDeferredLightProgram], "color_texture");
        glUniform1i(loc, 0);
        loc = glGetUniformLocation(_programs[kDeferredLightProgram], "normal_texture");
        glUniform1i(loc, 1);
        loc = glGetUniformLocation(_programs[kDeferredLightProgram], "position_texture");
        glUniform1i(loc, 2);

        glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kLightBuffer]);
        CheckGLError();
        glBufferData(GL_UNIFORM_BUFFER, sizeof(_light_buffer), &_light_buffer, GL_DYNAMIC_DRAW);
        CheckGLError();
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        CheckGLError();

        glBindBufferBase(GL_UNIFORM_BUFFER, 2, _uniform_buffers[kLightBuffer]);
        CheckGLError();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kViewProjTransformBuffer]);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &float4x4identity, GL_DYNAMIC_DRAW);
        
        const Mesh& mesh = _meshes[_quad_mesh];
        for(int ii=0;ii<1;++ii) {
            float4x4 transform = float4x4Scale(2.0f, 2.0f, 1.0f);
    
            glBindVertexArray(mesh.vao);
            glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kWorldTransformBuffer]);
            CheckGLError();
            glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &transform, GL_DYNAMIC_DRAW);
            CheckGLError();
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            CheckGLError();
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, mesh.index_format, NULL);
            CheckGLError();
        }
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_BLEND);
        glUseProgram(0);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    _num_3d_render_commands = _num_2d_render_commands = 0;
    _light_buffer.num_lights = 0;
}
void resize(int width, int height) {
    _width = width;
    _height = height;
    glViewport(0, 0, width, height);
    _orthographic_projection = float4x4OrthographicOffCenterLH(0, width, height, 0, 0.0f, 1.0f);
    _perspective_projection = float4x4PerspectiveFovLH(DegToRad(50.0f), width/(float)height, 0.1f, 10000.0f);

#if 1
    // Resize render targets
    glBindRenderbuffer(GL_RENDERBUFFER, _color_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    CheckGLError();
    glBindRenderbuffer(GL_RENDERBUFFER, _normal_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    CheckGLError();
    glBindRenderbuffer(GL_RENDERBUFFER, _position_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, _width, _height);
    CheckGLError();
    glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
    CheckGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _width, _height);
    CheckGLError();

    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _textures[_color_texture]);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _textures[_normal_texture]);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _textures[_position_texture]);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, _textures[_depth_texture]);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    CheckGLError();

    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGLError();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    CheckGLError();
#endif
}

MeshID create_mesh(uint32_t vertex_count, VertexType vertex_type,
                   uint32_t index_count, size_t index_size,
                   const void* vertices, const void* indices) {
    MeshID id = _num_meshes++;
    _meshes[id] = _create_mesh(vertex_count, vertex_type, index_count, index_size, vertices, indices);
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

void draw_light(const float4& light) {
    _light_buffer.lights[_light_buffer.num_lights++] = light;
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
    _programs[kDeferredProgram] = _create_program(vs_deferred, fs_deferred);
    glDeleteShader(vs_deferred);
    glDeleteShader(fs_deferred);

    buffer_index = glGetUniformBlockIndex(_programs[kDeferredProgram], "PerFrame");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[kDeferredProgram], buffer_index, 0);
    buffer_index = glGetUniformBlockIndex(_programs[kDeferredProgram], "PerObject");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[kDeferredProgram], buffer_index, 1);
    
    // Deferred light
    vs_deferred = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/deferred_light.vsh");
    fs_deferred = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/deferred_light.fsh");
    _programs[kDeferredLightProgram] = _create_program(vs_deferred, fs_deferred);
    glDeleteShader(vs_deferred);
    glDeleteShader(fs_deferred);

    buffer_index = glGetUniformBlockIndex(_programs[kDeferredLightProgram], "PerFrame");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[kDeferredLightProgram], buffer_index, 0);
    buffer_index = glGetUniformBlockIndex(_programs[kDeferredLightProgram], "PerObject");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[kDeferredLightProgram], buffer_index, 1);
    buffer_index = glGetUniformBlockIndex(_programs[kDeferredLightProgram], "LightBuffer");
    assert(buffer_index != GL_INVALID_INDEX);
    glUniformBlockBinding(_programs[kDeferredLightProgram], buffer_index, 2);

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
void _draw_mesh(MeshID mesh_id, TextureID texture_id, const float4x4& transform, int program_id) {
    const Mesh& mesh = _meshes[mesh_id];
    glBindVertexArray(mesh.vao);
    _validate_program(_programs[program_id]);
    glUseProgram(_programs[program_id]);
    glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[kWorldTransformBuffer]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &transform, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, _textures[texture_id]);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, _uniform_buffers[kViewProjTransformBuffer]);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, _uniform_buffers[kWorldTransformBuffer]);
    glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, mesh.index_format, NULL);
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
GLuint  _color_buffer;
GLuint  _depth_buffer;
GLuint  _normal_buffer;
GLuint  _position_buffer;

GLuint  _color_texture;
GLuint  _normal_texture;
GLuint  _depth_texture;
GLuint  _position_texture;

int     _width;
int     _height;

};

/*
 * External
 */
Render* Render::_create_ogl(void) {
    return new RenderGL;
}
