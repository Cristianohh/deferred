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
