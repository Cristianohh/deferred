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

enum VertexShaderType {
kVSPos,

kNUM_VERTEX_SHADERS
};
const char* kVertexShaderNames[] =
{
    /* kVSPos */    "assets/shaders/PosGL.vsh",
};

GLuint _compile_shader(GLenum shader_type, const char* source) {
    GLint status = GL_TRUE;
    GLuint shader = glCreateShader(shader_type);
    CheckGLError();
    glShaderSource(shader, 1, &source, NULL);
    CheckGLError();
    glCompileShader(shader);
    CheckGLError();
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    CheckGLError();
    if(status == GL_FALSE) {
        char printBuffer[1024];
        char statusBuffer[1024];
        glGetShaderInfoLog(shader, sizeof(statusBuffer), NULL, statusBuffer);
        CheckGLError();
        
        snprintf(printBuffer, sizeof(printBuffer), "Shader compile error: %s", statusBuffer);
        debug_output("%s\n", printBuffer);
        glDeleteShader(shader);
        CheckGLError();
        shader = 0;
    }
    return shader;
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
    glClearColor(0.6f, 0.2f, 0.5f, 1.0f);
    glClearDepth(1.0f);
    _load_shaders();
    _clear(); // Clear once so the first present isn't garbage
}
void shutdown(void) {
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
    // Vertex Shaders
    for(int ii=0;ii<kNUM_VERTEX_SHADERS;++ii) {
        MessageBoxResult retry = kMBOK;
        do {
            char buffer[1024*64]; // 64k should be a safe size
            FILE* file = fopen(kVertexShaderNames[ii], "rt");
            assert(file);
            size_t bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file);
            buffer[bytes_read] = '\0';
            fclose(file);
            _vertex_shaders[ii] = _compile_shader(GL_VERTEX_SHADER, buffer);
            if(_vertex_shaders[ii] == 0) {
                char message[512];
                snprintf(message, sizeof(message), "Error compiling shader %s", kVertexShaderNames[ii]);
                retry = message_box("Shader Compile Error", message);
            }
        } while(retry == kMBRetry);
    }
}

private:

void* _window;
#ifdef _WIN32
HDC _dc;
#endif
GLuint  _vertex_shaders[kNUM_VERTEX_SHADERS];

};

/*
 * External
 */
Render* Render::_create_ogl(void) {
    return new RenderGL;
}
