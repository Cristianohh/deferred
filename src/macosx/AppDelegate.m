/*! @file AppDelegate.m
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#import "AppDelegate.h"
#import "OpenGLView.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    /* Insert code here to initialize your application */
    NSBundle* bundle = [NSBundle mainBundle];
    NSRect screenRect = [[NSScreen mainScreen] frame];
    unsigned int width = (unsigned int)(screenRect.size.width/2.0f);
    unsigned int height = (unsigned int)(screenRect.size.height/2.0f);
    NSRect frame = NSMakeRect(0, screenRect.size.height-height, width, height);
    
    [self setWindow:[[NSWindow alloc] initWithContentRect:frame
                                                styleMask:NSTitledWindowMask | NSResizableWindowMask 
                                                  backing:NSBackingStoreBuffered 
                                                    defer:NO]];
    
    [[self window] setContentView:[[OpenGLView alloc] init]];
    [[self window] makeKeyAndOrderFront:nil];

    chdir([[bundle resourcePath] UTF8String]); /* Set cwd to Content/Resources */
    {   /* Get argc/argv */
        NSArray *arguments = [[NSProcessInfo processInfo] arguments];
        int argc = (int)[arguments count];
        const char* argv[32] = { 0 };
        int ii;
        for(ii=0; ii<argc; ++ii)
        {
            NSString* arg = [arguments objectAtIndex:(NSUInteger)ii];
            argv[ii] = [arg UTF8String];
        }
        on_init(argc, argv);
    }
    
    (void)sizeof(aNotification);
}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    on_shutdown();
    return NSTerminateNow;
    (void)sizeof(sender);
}

@end
