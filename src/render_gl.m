/*! @file render_gl.m
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

extern void _osx_flush_buffer(void* window);

#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if (TARGET_OS_MAC && !(TARGET_OS_IPHONE))
        #include <AppKit/AppKit.h>
    #endif
#endif

/*
 * External
 */
void _osx_flush_buffer(void* window) {
    NSOpenGLView* view = (NSOpenGLView*)[(__bridge NSWindow*)window contentView];
    [[view openGLContext] flushBuffer];
}
