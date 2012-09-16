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

namespace {

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
        program = 0;
    }
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
    glBindBuffer(type, buffer);
    CheckGLError();
    glBufferData(type, (GLsizeiptr)size, data, GL_DYNAMIC_DRAW);
    CheckGLError();
    glBindBuffer(type, 0);
    CheckGLError();
    return buffer;
}

}

class RenderGL : public Render {
public:

RenderGL()
    : _window(NULL)
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
    _clear(); // Clear once so the first present isn't garbage
}
void shutdown(void) {
    _unload_shaders();
    glDeleteBuffers(kNUM_UNIFORM_BUFFERS, _uniform_buffers);
}
void render(void) {
    _present();
    _clear();
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
}
void _unload_shaders(void) {
    for(int ii=0;ii<kNUM_VERTEX_SHADERS;++ii)
        glDeleteShader(_vertex_shaders[ii]);
    for(int ii=0;ii<kNUM_FRAGMENT_SHADERS;++ii)
        glDeleteShader(_fragment_shaders[ii]);
}
void _create_uniform_buffers(void) {
    glGenBuffers(kNUM_UNIFORM_BUFFERS, _uniform_buffers);
    for(int ii=0;ii<kNUM_UNIFORM_BUFFERS;++ii) {
        glBindBuffer(GL_UNIFORM_BUFFER, _uniform_buffers[ii]);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(float4x4), &float4x4identity, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}
void _bind_uniform_buffers(void) {
    //GLuint buffer_index = glGetUniformBlockIndex(_programs[kSimpleColor], kUniformBufferBindings[kWorldTransformBuffer]);
    //glUniformBlockBinding(_programs[kSimpleColor], buffer_index, 0);
}

private:

void* _window;
#ifdef _WIN32
HDC _dc;
#endif
GLuint  _vertex_shaders[kNUM_VERTEX_SHADERS];
GLuint  _fragment_shaders[kNUM_FRAGMENT_SHADERS];
GLuint  _programs[kNUM_PROGRAMS];
GLuint  _uniform_buffers[kNUM_UNIFORM_BUFFERS];

};

/*
 * External
 */
Render* Render::_create_ogl(void) {
    return new RenderGL;
}
