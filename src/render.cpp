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
Resource Render::load_texture(const char* filename, void* ud) {
    Render* render = (Render*)ud;
    return render->_load_texture(filename);
}
void Render::unload_texture(Resource resource, void* ud) {
    Render* render = (Render*)ud;
    render->_unload_texture(resource);
}
Resource Render::load_mesh(const char* filename, void* ud) {
    Render* render = (Render*)ud;
    return render->_load_mesh(filename);
}
void Render::unload_mesh(Resource resource, void* ud) {
    Render* render = (Render*)ud;
    render->_unload_mesh(resource);
}
