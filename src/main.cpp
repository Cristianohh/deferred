/*! @file main.c
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "application.h"
#include <stdio.h>
#include "unit_test.h"

#include "game.h"
#include "world.h"

#include "perlin_noise.h"

static Game*    _game = NULL;

#define TOTAL_NUM_NUMBERS (1024*1024*8)
#define CHUNK_SIZE (32)

int on_init(int argc, const char* argv[]) {
#if 0
    Timer timer;
    float x[CHUNK_SIZE], y[CHUNK_SIZE], z[CHUNK_SIZE], res[CHUNK_SIZE], res2[CHUNK_SIZE];
    for(int ii=0; ii<CHUNK_SIZE; ++ii) {
        x[ii] = rand()/(float)RAND_MAX;
        y[ii] = rand()/(float)RAND_MAX;
        z[ii] = rand()/(float)RAND_MAX;
    }
    
    timer_init(&timer);

    for(int ii=0; ii < TOTAL_NUM_NUMBERS; ii += CHUNK_SIZE) {
        for(int jj=0; jj < CHUNK_SIZE; ++jj) {
            res[jj] = noise(42, x[jj], y[jj], z[jj]);
        }
    }
    double delta_time = timer_delta_time(&timer);
    debug_output("Slow time: %f\n", delta_time);

    
    timer_init(&timer);

    for(int ii=0; ii < TOTAL_NUM_NUMBERS; ii += CHUNK_SIZE) {
        noisev(42, x, y, z, res2, CHUNK_SIZE);
    }
    delta_time = timer_delta_time(&timer);
    debug_output("Fast time: %f\n", delta_time);
    
    exit(0);
    return 1;
#endif
    _game = new Game();
    _game->initialize();
    return 0;
    (void)(argc);
    (void)(argv[0]);
}
int on_frame(void) {
    return _game->on_frame();
}
void on_shutdown(void) {
    _game->shutdown();
    delete _game;
}

int main(int argc, const char* argv[])
{
    RUN_ALL_TESTS(argc, argv, "-t");
    return ApplicationMain(argc, argv);
}
