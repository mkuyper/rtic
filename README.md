# RTiC – Run-Time in C

RTiC is a lightweight library and API to schedule tasks in a resource
constrained environment.  It can serve as an abstraction layer for any other
library or application that has a need to schedule tasks to run
non-concurrently and at a specific point in time.


## Concepts

### Task

A task is a unit of work that can be scheduled to run at a specific point in
time. 

### Loop

A loop is a collection of scheduled tasks that can be executed. A task can
never be interrupted by another task that is scheduled in the same loop.


## Example

Using the run-time is very straight-forward: Simply allocate a loop and one or
more tasks and schedule them. Tasks can reschedule themselves or others when
they run.

```c
#include <inttypes.h>
#include <stdio.h>

#include "rtic.h"

static void runloop (rtic_loop* loop);

// Task function
static void say_hello (uint32_t now, rtic_task* task, void* context) {
    // say hello
    printf("Hello world, the time is %" PRIu32 " ticks (%d s)\n",
            now, rtic_ticks2sec(now));

    // re-schedule ourselves to run again in 1 sec
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
```

Running the loop in similarly easy: Simply call `rtic_step()` with the current
time, and the loop will execute the next task that is scheduled to run at that
time (or before). This function will return the time at which to call it again,
which might be immediately.

A simple implementation on POSIX might look like this:

```c
#include <time.h>

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
```
