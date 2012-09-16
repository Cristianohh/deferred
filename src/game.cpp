/*! @file game.cpp
 *  @author kcweicht
 *  @date 9/14/2012 2:16:40 PM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#include "game.h"

#include <stdio.h>
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
    // Handle OS events
    const SystemEvent* event = app_pop_event();
    while (event) {
        switch(event->type) {
        case kEventResize:
            _render->resize(event->data.resize.width, event->data.resize.height);
            printf("W: %d  H: %d\n", event->data.resize.width, event->data.resize.height);
            break;
        default:
            break;
        }
        event = app_pop_event();
    }

    // Beginning of frame stuff
    float delta_time = (float)timer_delta_time(&_timer);
    update_fps(&_fps, delta_time);

    // Frame
    _render->render();

    // End of frame stuff
    if(++_frame_count % 1024 == 0)
        debug_output("%.0f FPS\n", get_fps(&_fps));
    return 0;
}
