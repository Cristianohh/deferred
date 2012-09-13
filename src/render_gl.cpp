/*! @file render_gl.cpp
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "render.h"
#include <stddef.h>

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
    #include <gl/gl3.h>
    #pragma warning(disable:4127) /* Conditional expression is constant */
    #define snprintf _snprintf
#endif

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
    _window = window;
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}
void shutdown(void) {
}
void render(void) {
    _clear();
    _present();
}

private:
void _clear(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void _present(void) {
#ifdef __APPLE__
    _osx_flush_buffer(_window);
#else
#endif
}

private:

void* _window;

};

/*
 * External
 */
Render* Render::create() {
    return new RenderGL;
}
void Render::destroy(Render* render) {
    delete render;
}
