/*! @file game.h
 *  @brief TODO: Add the purpose of this module
 *  @author kcweicht
 *  @date 9/14/2012 2:16:40 PM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#ifndef __game_h__
#define __game_h__

#include "timer.h"
#include "vec_math.h"
#include "fps.h"
#include "render.h"

class Render;

struct Object {
    Transform   transform;
    MeshID      mesh;
    TextureID   texture;
};

class Game {
public:
    Game();

    void initialize(void);
    void shutdown(void);
    int on_frame(void);
    
private:
    void _control_camera(void);
    void _add_object(const Object& o);

private:
    FPSCounter  _fps;
    Timer       _timer;
    Render*     _render;
    int         _frame_count;
    float       _delta_time;

    Transform   _camera;

    Object      _objects[1024];
    int         _num_objects;
    float4      _lights[MAX_LIGHTS];
    float4      _colors[MAX_LIGHTS];
};

#endif /* include guard */
