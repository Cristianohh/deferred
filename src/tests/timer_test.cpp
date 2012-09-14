/*! @file utility_test.cpp
 *  @author Kyle Weicht
 *  @date 8/9/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "unit_test.h"

extern "C" {
#include "timer.h"
}

namespace
{

TEST(TimerTest)
{
    Timer timer;
    timer_init(&timer);
    double time = 0.0f;
    for(int ii=0;ii<1024;++ii)
    {
        time += timer_delta_time(&timer);
    }
    CHECK_GREATER_THAN_FLOAT(time, 0.0);
}

} // anonymous namespace
