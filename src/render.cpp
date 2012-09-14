/*! @file render.cpp
 *  @author kcweicht
 *  @date 9/14/2012 3:41:28 PM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#include "render.h"

/*
 * Internal 
 */
/* TODO: Add private static definitions here */


/*
 * External
 */
Render* Render::create() {
    return _create_ogl();
    // return _create_dx();
}
void Render::destroy(Render* render) {
    delete render;
}
