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
#include "resource_manager.h"
#include "world.h"

class Render;

class Game {
public:
    Game();

    void initialize(void);
    void shutdown(void);
    int on_frame(void);
    
private:
    void _control_camera(float mouse_x, float mouse_y);
    
private:
    FPSCounter  _fps;
    Timer       _timer;
    Render*     _render;
    int         _frame_count;
    float       _delta_time;

    EntityID    _sun_id;

    Transform   _camera;

    ResourceManager _resource_manager;

    World       _world;
};

#endif /* include guard */
