/*! @file fps.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/13/12 8:45 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 *	@addtogroup fps fps
 *	@{
 */
#ifndef __fps_h__
#define __fps_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { kFPSFrameCount = 31 };

typedef struct {
    float times[kFPSFrameCount];
    uint32_t frame;
} FPSCounter;

void update_fps(FPSCounter* fps, float delta_time);
float get_fps(const FPSCounter* fps);
float get_frametime(const FPSCounter* fps);

#ifdef __cplusplus
} // extern "C" {
#endif

/* @} */
#endif /* include guard */
