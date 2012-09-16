/*! @file render_gl.cpp
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "render.h"
#include <stddef.h>
#include <stdio.h>
#include "assert.h"

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

    kNUM_PROGRAMS
};

enum { kMAX_MESHES = 1024 };

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
{
}
~RenderGL() {
}

void* window(void) { return _window; }

void initialize(void* window) {
#ifdef _WIN32
    HWND hWnd = (HWND)window;
    HDC hDC = GetDC(hWnd);

    FRAGMENTFORMATDESCRIPTOR   pfd = {0};
	pfd.nSize       = sizeof(pfd);
	pfd.nVersion    = 1;
	pfd.dwFlags     = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iFragmentType  = PFD_TYPE_RGBA;
	pfd.cColorBits  = 24;
	pfd.cDepthBits  = 24;
	pfd.iLayerType  = PFD_MAIN_PLANE;

    int fragment_format = ChooseFragmentFormat(hDC, &pfd);
    assert(fragment_format);

    int result = SetFragmentFormat(hDC, fragment_format, &pfd);
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
    glClearColor(0.6f, 0.2f, 0.5f, 1.0f);
    glClearDepth(1.0f);
    _load_shaders();
    _create_uniform_buffers();
    _create_base_meshes();
    _clear(); // Clear once so the first present isn't garbage


    glDisable(GL_CULL_FACE);
    CheckGLError();
    glDisable(GL_DEPTH_TEST);
    CheckGLError();
}
void shutdown(void) {
    _unload_shaders();
    glDeleteBuffers(kNUM_UNIFORM_BUFFERS, _uniform_buffers);
}
void render(void) {
    _present();
    _clear();
    _draw_mesh(1, k2DProgram);
}
void resize(int width, int height) {
    glViewport(0, 0, width, height);
    _orthographic_projection = float4x4OrthographicOffCenterLH(0, width, height, 0, 0.0f, 1.0f);
    _perspective_projection = float4x4PerspectiveFovLH(DegToRad(50.0f), width/(float)height, 0.1f, 10000.0f);
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
}
void _unload_shaders(void) {
    for(int ii=0;ii<kNUM_VERTEX_SHADERS;++ii)
        glDeleteShader(_vertex_shaders[ii]);
    for(int ii=0;ii<kNUM_FRAGMENT_SHADERS;++ii)
        glDeleteShader(_fragment_shaders[ii]);
}
void _create_uniform_buffers(void) {
    for(int ii=0;ii<kNUM_UNIFORM_BUFFERS;++ii) {
        _uniform_buffers[ii] = _create_buffer(GL_UNIFORM_BUFFER, sizeof(float4x4), &float4x4identity);
        assert(_uniform_buffers[ii]);
    }
}
void _create_base_meshes(void) {
    _meshes[_num_meshes++] = _create_mesh(ARRAYSIZE(kCubeVertices), kVtxPosNormTex,
                                          ARRAYSIZE(kCubeIndices), sizeof(kCubeIndices[0]),
                                          kCubeVertices, kCubeIndices);
    _meshes[_num_meshes++] = _create_mesh(ARRAYSIZE(kQuadVertices), kVtxPosTex,
                                          ARRAYSIZE(kQuadIndices), sizeof(kQuadIndices[0]),
                                          kQuadVertices, kQuadIndices);
}
void _draw_mesh(int mesh_id, int program_id) {
    const Mesh& mesh = _meshes[mesh_id];
    glBindVertexArray(mesh.vao);
    _validate_program(_programs[program_id]);
    glUseProgram(_programs[program_id]);
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

float4x4    _perspective_projection;
float4x4    _orthographic_projection;

};

/*
 * External
 */
Render* Render::_create_ogl(void) {
    return new RenderGL;
}
