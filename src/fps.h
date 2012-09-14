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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int frame_count;
    float time;
} FPSCounter;

void fps_update(float delta_time);
float get_fps(FPSCounter* fps);

#ifdef __cplusplus
} // extern "C" {
#endif

/* @} */
#endif /* include guard */
