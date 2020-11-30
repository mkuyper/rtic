// Copyright (C) 2020-2020 Michael Kuyper. All rights reserved.
//
// This file is subject to the terms and conditions defined in file 'LICENSE',
// which is part of this source code package.

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include "rtic.h"

static void runloop (rtic_loop* loop);

// Task function
static void say_hello (uint32_t now, rtic_task* task, void* context) {
    // say hello
    printf("Hello world, the time is %" PRIu32 " ticks (%d s)\n",
            now, rtic_ticks2sec(now));

    // re-schedule task
    rtic_schedule(task, now + rtic_sec2ticks(1), say_hello, context, 0);
}

int main (void) {
    rtic_loop loop = { .hal = NULL };
    rtic_task task = { .loop = &loop };

    // schedule first task
    rtic_schedule(&task, rtic_hal_now(), say_hello, NULL, 0);

    // run loop
    runloop(&loop);

    return 0;
}

// Time conversion functions
static uint64_t ticks_extend (uint32_t ticks, uint64_t context) {
    return context + rtic_tdiff(context, ticks);
}

static uint64_t ts2ticks (struct timespec* ts) {
    return ((((uint64_t) ts->tv_sec * 1000000) + (ts->tv_nsec / 1000)) * RTIC_TICKS_PER_SEC) / 1000000;
}

static void ticks2ts (struct timespec* ts, uint32_t ticks) {
    uint64_t xt = ticks_extend(ticks, ts2ticks(ts));
    ts->tv_sec = xt / RTIC_TICKS_PER_SEC;
    xt -= ((uint64_t) ts->tv_sec * RTIC_TICKS_PER_SEC);
    ts->tv_nsec = (xt * 1000000000) / RTIC_TICKS_PER_SEC;
}

// Main loop
static void runloop (rtic_loop* loop) {
    // runtime loop
    while( 1 ) {
        // get current time
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        // call RTiC loop
        uint32_t next = rtic_step(loop, ts2ticks(&ts));
        // sleep
        ticks2ts(&ts, next);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
    }
}

// Glue functions
uint32_t rtic_hal_now (void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return ts2ticks(&now);
}

void rtic_hal_enter_critical (void* hal) { /* no concurrency */ }
void rtic_hal_exit_critical (void* hal) { /* no concurrency */ }
