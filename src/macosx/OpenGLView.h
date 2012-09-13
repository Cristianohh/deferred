/*! @file OpenGLView.h
 *  @brief Basic NSOpenGLView implementation
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>

/*! @details This can be used directly or simply as an example. The `NSView`
 *    simply has to have a `pixelFormat` and `openGLContext`. `NSOpenGLView`
 *    defines these by default.
 */
@interface OpenGLView : NSView

@property NSOpenGLPixelFormat*  pixelFormat; /*!< The pixel format */
@property NSOpenGLContext*      openGLContext; /*!< The OpenGL context */

@end
