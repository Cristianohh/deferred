/*! @file main.c
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "application.h"
#include <stdio.h>
#include "unit_test.h"

#include "render.h"
#include "timer.h"
#include "fps.h"

static Render* _render = NULL;
static Timer _timer = {0};
static FPSCounter _fps = {0};

int on_init(int argc, const char* argv[]) {
    _render = Render::create();
    _render->initialize(app_get_window());
    timer_init(&_timer);
    for(int ii=0;ii<argc;++ii)
        printf("%s\n", argv[ii]);
    return 0;
    (void)sizeof(argc);
    (void)sizeof(argv[0]);
}
int on_frame(void) {
    float delta_time = (float)timer_delta_time(&_timer);
    static int frame_count = 0;
    if(++frame_count % kFPSFrameCount == 0)
        printf("%f\n", get_fps(&_fps));

    _render->render();
    update_fps(&_fps, delta_time);
    return 0;
}
void on_shutdown(void) {
    _render->shutdown();
    Render::destroy(_render);
}

int main(int argc, const char* argv[])
{
    RUN_ALL_TESTS(argc, argv, "-t");
    return ApplicationMain(argc, argv);
}
