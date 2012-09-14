/*! @file game.cpp
 *  @author kcweicht
 *  @date 9/14/2012 2:16:40 PM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#include "game.h"

#include "render.h"
#include "assert.h"
#include "application.h"

/*
 * Internal 
 */
/* TODO: Add private static definitions here */


/*
 * External
 */
Game::Game() {
    _fps.frame = 0;
}
void Game::initialize(void) {
    timer_init(&_timer);
    _frame_count = 0;
    _render = Render::create();
    _render->initialize(app_get_window());
}
void Game::shutdown(void) {
    _render->shutdown();
    Render::destroy(_render);
}
int Game::on_frame(void) {
    // Beginning of frame stuff
    float delta_time = (float)timer_delta_time(&_timer);
    update_fps(&_fps, delta_time);

    // Frame
    _render->render();
    if(_frame_count % 1024 == 0)
        debug_output("%.0f FPS\n", get_fps(&_fps));

    // End of frame stuff
    _frame_count++;
    return 0;
}
