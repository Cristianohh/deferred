/*! @file OpenGLView.m
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#import "OpenGLView.h"
#include <assert.h>

#define CheckGLError()                              \
    do {                                            \
        GLenum _glError = glGetError();             \
        if(_glError != GL_NO_ERROR) {               \
            printf("OpenGL Error: %d\n", _glError); \
        }                                           \
        assert(_glError == GL_NO_ERROR);            \
    } while(0)

@implementation OpenGLView

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        /* Initialization code here. */
        NSOpenGLPixelFormatAttribute attributes[] = 
        {
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,/* OpenGL 3.2 profile */
            NSOpenGLPFAAccelerated,                                 /* Only use hardware acceleration */
            NSOpenGLPFADoubleBuffer,                                /* Double buffer it */
            NSOpenGLPFAColorSize, (NSOpenGLPixelFormatAttribute)32, /* 32-bit color buffer */
            NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)24, /* 24-bit depth buffer */
            (NSOpenGLPixelFormatAttribute)nil,
        };
        GLint vsync = 0;
        
        /* Support high-resolution backing (retina) */
        [self  setWantsBestResolutionOpenGLSurface:YES];
        
        /* Create the pixel format */
        [self setPixelFormat:[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes]];
        [self setOpenGLContext:[[NSOpenGLContext alloc] initWithFormat:[self pixelFormat] shareContext:nil]];
        [[self openGLContext] makeCurrentContext];
        
        /* Turn off VSync */
        [[self openGLContext] setValues:&vsync forParameter:NSOpenGLCPSwapInterval];
    }

    return self;
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}
- (BOOL)preservesContentDuringLiveResize
{
    return YES;
}
- (void)drawRect:(NSRect)dirtyRect
{
    /* Drawing code here. */
    (void)sizeof(dirtyRect);
}
- (void)lockFocus
{
    NSOpenGLContext* context = [self openGLContext];
    
    [super lockFocus];
    if ([context view] != self) {
        [context setView:self];
    }
    [context makeCurrentContext];
}
- (void)update
{
    [[self openGLContext] update];
    CheckGLError();
}
- (void)reshape
{
    NSRect bounds = [self convertRectToBacking:[self bounds]];
    
    [[self openGLContext] makeCurrentContext];
    CheckGLError();
    CGLLockContext([[self openGLContext]CGLContextObj]);
    CheckGLError();
    
    CheckGLError();
    printf("W: %d  H: %d\n", (int)bounds.size.width, (int)bounds.size.height);
    
    [[self openGLContext] update];
    CheckGLError();
    CGLUnlockContext([[self openGLContext] CGLContextObj]);
    CheckGLError();
}
- (void)windowResized:(NSNotification *)notification;
{
    [self reshape];
    (void)sizeof(notification);
}
- (void)viewDidMoveToWindow
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(windowResized:)
                                                 name:NSWindowDidResizeNotification
                                               object:[self window]];
}

- (void)toggleFullScreen:(id)sender
{
    [[self window] toggleFullScreen:sender];
}
@end
