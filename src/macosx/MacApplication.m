/*! @file MacApplication.m
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#import "MacApplication.h"


@implementation MacApplication

- (void)run
{
    int ii;
    _running = 1;
    [self finishLaunching];
    do {
        for(ii=0;ii<16;++ii) {
            NSEvent* event = [self nextEventMatchingMask:NSAnyEventMask
                                      untilDate:[NSDate distantPast]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES];
            if(event == NULL)
                break;
            [self sendEvent:event];
            [self updateWindows];
        }

        /* Application per-frame code */
        if(on_frame())
            [self performSelector:@selector(terminate:) withObject:self];
        
    } while (_running);
}

@end
