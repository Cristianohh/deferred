/*! @file AppDelegate.m
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#import "AppDelegate.h"
#import <AppKit/AppKit.h>
#import "OpenGLView.h"

int ApplicationMain(int argc, const char* argv[]) {
    NSDictionary* infoDictionary = [[NSBundle mainBundle] infoDictionary];
    Class principalClass = NSClassFromString([infoDictionary objectForKey:@"NSPrincipalClass"]);
    NSApplication* application = NULL;
    NSString *mainNibName = NULL;
    NSNib *mainNib = NULL;

    if([principalClass respondsToSelector:@selector(sharedApplication)] == 0) {
        NSLog(@"NSPrincipalClass does not respond to `sharedApplication");
        return 1;
    }
    application = [principalClass sharedApplication];
    mainNibName = [infoDictionary objectForKey:@"NSMainNibFile"];
    mainNib = [[NSNib alloc] initWithNibNamed:mainNibName bundle:[NSBundle mainBundle]];
    [mainNib instantiateNibWithOwner:application topLevelObjects:nil];

    /* Start the loop */
    if([application respondsToSelector:@selector(run)]) {
        [application performSelectorOnMainThread:@selector(run) withObject:nil waitUntilDone:YES];
    }
    return 0;
    (void)sizeof(argc);
    (void)sizeof(argv[0]);
}
void* app_get_window(void) {
    return (__bridge void*)[[NSApp delegate] window];
}
MessageBoxResult message_box(const char* header, const char* message) {
    /*convert the strings from char* to CFStringRef */
    CFStringRef header_ref  = CFStringCreateWithCString(NULL, header, kCFStringEncodingASCII);
    CFStringRef message_ref = CFStringCreateWithCString(NULL, message, kCFStringEncodingASCII);

    CFOptionFlags result;  /*result code from the message box */
   
    /*launch the message box */
    CFUserNotificationDisplayAlert( 0.0f, /* no timeout */
                                    kCFUserNotificationNoteAlertLevel, /*change it depending message_type flags ( MB_ICONASTERISK.... etc.) */
                                    NULL, /*icon url, use default, you can change it depending message_type flags */
                                    NULL, /*not used */
                                    NULL, /*localization of strings */
                                    header_ref, /*header text  */
                                    message_ref, /*message text */
                                    NULL, /*default "ok" text in button */
                                    CFSTR("Cancel"), /*alternate button title */
                                    CFSTR("Retry"), /*other button title, null--> no other button */
                                    &result /*response flags */
                                    );

    /*Clean up the strings */
    CFRelease( header_ref );
    CFRelease( message_ref );

    /*Convert the result */
    switch(result) {
    case kCFUserNotificationDefaultResponse:
        return kMBOK;
    case kCFUserNotificationAlternateResponse:
        return kMBCancel;
    case kCFUserNotificationOtherResponse:
        return kMBRetry;
    }
    return (MessageBoxResult)-1;
}

static SystemEvent  _event_queue[1024];
static uint         _write_pos = 0;
static uint         _read_pos = 0;

void _app_push_event(SystemEvent event) {
    _event_queue[_write_pos%1024] = event;
    _write_pos++;
}

const SystemEvent* app_pop_event(void) {
    if(_read_pos == _write_pos)
        return NULL;
    return &_event_queue[(_read_pos++) % 1024];
}

@interface OpenGLWindow : NSWindow
@property BOOL fullscreen;
@end

@implementation OpenGLWindow

- (void)toggleFullScreen:(id)sender
{
    NSRect screenRect = [[NSScreen mainScreen] frame];
    _fullscreen = !_fullscreen;
    if(_fullscreen) {
        [self setStyleMask:NSBorderlessWindowMask];
        
        [self setContentSize:screenRect.size];
        [self setFrameTopLeftPoint:NSMakePoint(0.0f, screenRect.size.height)];
        /*[self setFrame:screenRect display:YES];*/
        [self setLevel:NSMainMenuWindowLevel+1];
        [self setHidesOnDeactivate:YES];
    } else {
        unsigned int width = (unsigned int)(screenRect.size.width/2);
        unsigned int height = (unsigned int)(screenRect.size.height/2);
        
        [self setStyleMask:NSTitledWindowMask | NSResizableWindowMask];
        [self setContentSize:NSMakeSize(width, height)];
        [self setFrameTopLeftPoint:NSMakePoint(0.0f, screenRect.size.height)];
        [self setLevel:NSNormalWindowLevel];
        [self setHidesOnDeactivate:NO];
    }
    [[self windowController] showWindow:nil];
    (void)sizeof(sender);
}
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
- (void)keyDown:(NSEvent *)theEvent
{
    printf("%d\n",[theEvent keyCode]);
}

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    /* Insert code here to initialize your application */
    NSBundle* bundle = [NSBundle mainBundle];
    NSRect screenRect = [[NSScreen mainScreen] frame];
    unsigned int width = (unsigned int)(screenRect.size.width/2.0f);
    unsigned int height = (unsigned int)(screenRect.size.height/2.0f);
    NSRect frame = NSMakeRect(0, 0, width, height);


    [self setWindow:[[OpenGLWindow alloc] initWithContentRect:frame
                                                    styleMask:NSTitledWindowMask | NSResizableWindowMask
                                                      backing:NSBackingStoreBuffered
                                                        defer:YES]];
    [[self window] setFrameTopLeftPoint:NSMakePoint(0.0f, screenRect.size.height)];
    
    [[self window] setContentView:[[OpenGLView alloc] init]];
    [[self window] setOpaque:YES];
    [self setController:[[NSWindowController alloc] initWithWindow:[self window]]];
    [[self controller] showWindow:nil];

    /*chdir([[bundle resourcePath] UTF8String]); */ /* Set cwd to Content/Resources */
    chdir([[[bundle bundlePath] stringByDeletingLastPathComponent] UTF8String]);
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
