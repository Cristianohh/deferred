/*! @file main.c
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "application.h"
#include <stdio.h>
#include "unit_test.h"

#include "render.h"

static Render* _render = NULL;

int on_init(int argc, const char* argv[]) {
    _render = Render::create();
    _render->initialize(app_get_window());
    return 0;
    (void)sizeof(argc);
    (void)sizeof(argv[0]);
}
int on_frame(void) {
    static int _frame_count = 0;
    if(_frame_count % 60 == 0)
        printf("%f\n", _frame_count);

    _render->render();
    return 0;
}
void on_shutdown(void) {
    _render->shutdown();
    Render::destroy(_render);
}

int main(int argc, const char* argv[])
{
    RUN_ALL_TESTS(argc, argv, "-t");
    return ApplicationMain(argc, (const char**)argv);
}
