/*! @file main.c
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "application.h"
#include <stdio.h>
#include "unit_test.h"

#include "game.h"

static Game _game;

int on_init(int argc, const char* argv[]) {
    _game.initialize();
    return 0;
    (void)(argc);
    (void)(argv[0]);
}
int on_frame(void) {
    return _game.on_frame();
}
void on_shutdown(void) {
    _game.shutdown();
}

int main(int argc, const char* argv[])
{
    RUN_ALL_TESTS(argc, argv, "-t");
    return ApplicationMain(argc, argv);
}
