/*! @file fps.c
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "fps.h"

/*
 * Internal 
 */

/*
 * External
 */
void update_fps(FPSCounter* fps, float delta_time) {
    int index = fps->frame++ % kFPSFrameCount;
    fps->times[index] = delta_time;
}
float get_fps(const FPSCounter* fps) {
    int ii;
    float total = 0.0f;
    for(ii=0;ii<kFPSFrameCount;++ii) {
        total += fps->times[ii];
    }
    return kFPSFrameCount/total;
}
float get_frametime(const FPSCounter* fps) {
    int ii;
    float total = 0.0f;
    for(ii=0;ii<kFPSFrameCount;++ii) {
        total += fps->times[ii];
    }
    return total/kFPSFrameCount * 1000.0f;
}
