/*! @file game.h
 *  @brief TODO: Add the purpose of this module
 *  @author kcweicht
 *  @date 9/14/2012 2:16:40 PM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#ifndef __game_h__
#define __game_h__

#include "timer.h"
#include "fps.h"

class Render;

class Game {
public:
    Game();

    void initialize(void);
    void shutdown(void);
    int on_frame(void);

private:
    FPSCounter  _fps;
    Timer       _timer;
    Render*     _render;
    int         _frame_count;
};

#endif /* include guard */
