/*! @file MacApplication.m
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#import "MacApplication.h"


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

@implementation MacApplication

- (void)run
{
    _running = 1;
    [self finishLaunching];
    do {
        NSEvent* event = NULL;
        do {
            event = [self nextEventMatchingMask:NSAnyEventMask
                                      untilDate:[NSDate distantPast]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES];
            [self sendEvent:event];
            [self updateWindows];
        } while(event);

        /* Application per-frame code */
        if(on_frame())
            [self performSelector:@selector(terminate:) withObject:self];
        
    } while (_running);
}

@end
