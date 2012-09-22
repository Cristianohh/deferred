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

#define KEY_MAPPING(keyCode, sysKey) case keyCode: return sysKey;
static Key _convert_keycode(uint16_t key)
{
    /* Reference: http://boredzo.org/blog/wp-content/uploads/2007/05/IMTx-virtual-keycodes.pdf */
    
    switch(key)
    {
    /* Qwerty row */
    KEY_MAPPING(12, KEY_Q);
    KEY_MAPPING(13, KEY_W);
    KEY_MAPPING(14, KEY_E);
    KEY_MAPPING(15, KEY_R);
    KEY_MAPPING(17, KEY_T);
    KEY_MAPPING(16, KEY_Y);
    KEY_MAPPING(32, KEY_U);
    KEY_MAPPING(34, KEY_I);
    KEY_MAPPING(31, KEY_O);
    KEY_MAPPING(35, KEY_P);
    
    /* Asdf row */
    KEY_MAPPING(0, KEY_A);
    KEY_MAPPING(1, KEY_S);
    KEY_MAPPING(2, KEY_D);
    KEY_MAPPING(3, KEY_F);
    KEY_MAPPING(5, KEY_G);
    KEY_MAPPING(4, KEY_H);
    KEY_MAPPING(38, KEY_J);
    KEY_MAPPING(40, KEY_K);
    KEY_MAPPING(37, KEY_L);
    
    /* Zxcv row */
    KEY_MAPPING(6, KEY_Z);
    KEY_MAPPING(7, KEY_X);
    KEY_MAPPING(8, KEY_C);
    KEY_MAPPING(9, KEY_V);
    KEY_MAPPING(11, KEY_B);
    KEY_MAPPING(45, KEY_N);
    KEY_MAPPING(46, KEY_M);
    
    /* Numbers */
    KEY_MAPPING(18, KEY_1);
    KEY_MAPPING(19, KEY_2);
    KEY_MAPPING(20, KEY_3);
    KEY_MAPPING(21, KEY_4);
    KEY_MAPPING(23, KEY_5);
    KEY_MAPPING(22, KEY_6);
    KEY_MAPPING(26, KEY_7);
    KEY_MAPPING(28, KEY_8);
    KEY_MAPPING(25, KEY_9);
    KEY_MAPPING(29, KEY_0);
    
    /* Misc */
    KEY_MAPPING(53, KEY_ESCAPE);
    KEY_MAPPING(56, KEY_SHIFT);
    KEY_MAPPING(59, KEY_CTRL);
    KEY_MAPPING(58, KEY_ALT);
    KEY_MAPPING(49, KEY_SPACE);

    KEY_MAPPING(55, KEY_SYS);
    
    /* Arrows */
    KEY_MAPPING(126, KEY_UP);
    KEY_MAPPING(125, KEY_DOWN);
    KEY_MAPPING(123, KEY_LEFT);
    KEY_MAPPING(124, KEY_RIGHT);
    
    /* Function keys */
    KEY_MAPPING(122, KEY_F1);
    KEY_MAPPING(120, KEY_F2);
    KEY_MAPPING(99, KEY_F3);
    KEY_MAPPING(118, KEY_F4);
    KEY_MAPPING(96, KEY_F5);
    KEY_MAPPING(97, KEY_F6);
    KEY_MAPPING(98, KEY_F7);
    KEY_MAPPING(100, KEY_F8);
    KEY_MAPPING(101, KEY_F9);
    KEY_MAPPING(109, KEY_F10);
    KEY_MAPPING(103, KEY_F11);
    KEY_MAPPING(111, KEY_F12);    
    
    default:
        return -1;
    }
}

static char         _keys[KEY_MAX_KEYS] = {0};
static char         _mouse_buttons[MOUSE_MAX_BUTTONS] = {0};
static SystemEvent  _event_queue[1024];
static uint         _write_pos = 0;
static uint         _read_pos = 0;

void _app_push_event(SystemEvent event) {
    _event_queue[_write_pos%1024] = event;
    _write_pos++;
}
int app_is_key_down(Key key) { return _keys[key]; }
int app_is_mouse_button_down(MouseButton button) { return _mouse_buttons[button]; }

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
    Key key = _convert_keycode([theEvent keyCode]);
    if(key == -1)
        return;
    if(_keys[key] == 0) {
        SystemEvent event;
        event.type = kEventKeyDown;
        event.data.key = key;
        _app_push_event(event);
    }
    _keys[key] = 1;
}
- (void)keyUp:(NSEvent *)theEvent
{
    Key key = _convert_keycode([theEvent keyCode]);
    if(key == -1)
        return;
    _keys[key] = 0;
}
- (void)flagsChanged:(NSEvent *)theEvent
{
    if([theEvent modifierFlags] & NSCommandKeyMask)
        _keys[KEY_SYS] = 1;
    else
        _keys[KEY_SYS] = 0;
    
    if([theEvent modifierFlags] & NSShiftKeyMask)
        _keys[KEY_SHIFT] = 1;
    else
        _keys[KEY_SHIFT] = 0;

    
    if([theEvent modifierFlags] & NSControlKeyMask)
        _keys[KEY_CTRL] = 1;
    else
        _keys[KEY_CTRL] = 0;
    
    if([theEvent modifierFlags] & NSAlternateKeyMask)
        _keys[KEY_ALT] = 1;
    else
        _keys[KEY_ALT] = 0;
}
- (void)mouseDown:(NSEvent *)theEvent
{
    if(_mouse_buttons[MOUSE_LEFT] == 0) {
        CGPoint pt = [theEvent locationInWindow];
        SystemEvent event;
        pt = [[self contentView] convertPointToBacking:pt];
        event.type = kEventMouseDown;
        event.data.mouse.x = (float)pt.x;
        event.data.mouse.y = (float)pt.y;
        event.data.mouse.button = MOUSE_LEFT;
        _app_push_event(event);
    }
    _mouse_buttons[MOUSE_LEFT] = 1;
}
- (void)mouseUp:(NSEvent *)theEvent
{
    _mouse_buttons[MOUSE_LEFT] = 0;
    (void)(sizeof(theEvent));
}
- (void)mouseMoved:(NSEvent *)theEvent
{
    SystemEvent event;
    event.type = kEventMouseMove;
    event.data.mouse.x = (float)[theEvent deltaX];
    event.data.mouse.y = (float)[theEvent deltaY];
    _app_push_event(event);    
}
- (void)mouseDragged:(NSEvent *)theEvent { [self mouseMoved:theEvent]; }
- (void)rightMouseDown:(NSEvent *)theEvent
{
    if(_mouse_buttons[MOUSE_RIGHT] == 0) {
        CGPoint pt = [theEvent locationInWindow];
        SystemEvent event;
        pt = [[self contentView] convertPointToBacking:pt];
        event.type = kEventMouseDown;
        event.data.mouse.x = (float)pt.x;
        event.data.mouse.y = (float)pt.y;
        event.data.mouse.button = MOUSE_RIGHT;
        _app_push_event(event);
    }
    _mouse_buttons[MOUSE_RIGHT] = 1;
}
- (void)rightMouseUp:(NSEvent *)theEvent
{
    _mouse_buttons[MOUSE_RIGHT] = 0;
    (void)(sizeof(theEvent));
}
- (void)rightMouseDragged:(NSEvent *)theEvent { [self mouseMoved:theEvent]; }

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
    [[self window] setAcceptsMouseMovedEvents:YES];
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
