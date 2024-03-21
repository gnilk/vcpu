//
// DurationTimer, imported from other project
//
#ifdef WIN32
#include <windows.h>
#include <winnt.h>
#endif
#include "DurationTimer.h"
#include <chrono>

using namespace gnilk;

//typedef std::chrono::steady_clock Clock;

DurationTimer::DurationTimer() {
    Reset();
}

//
// Restet starting point for clock
//
void DurationTimer::Reset() {
    time_start = Clock::now();

}

//
// Sample the time between last reset and now, return as double in seconds
// Note: This could be a one-liner but I had to debug and decided to leave it like this as it makes it easier to read..
//
double DurationTimer::Sample() {
    double ret = 0.0;

    auto elapsed = Clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed - time_start).count();

    ret = ms / 1000.0f;
  

    return ret;
}

