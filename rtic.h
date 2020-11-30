// Copyright (C) 2020-2020 Michael Kuyper. All rights reserved.
//
// This file is subject to the terms and conditions defined in file 'LICENSE',
// which is part of this source code package.

#ifndef _rtic_h_
#define _rtic_h_

#ifdef RTIC_HAL_IMPL_H
#include RTIC_HAL_IMPL_H
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct _rtic_task;
typedef struct _rtic_task rtic_task;
struct _rtic_loop;
typedef struct _rtic_loop rtic_loop;

typedef void (*rtic_task_func) (uint32_t now, rtic_task* task, void* context);

struct _rtic_loop {
    void* hal;
    rtic_task* queue;
};

struct _rtic_task {
    rtic_loop* loop;
    rtic_task* next;
    rtic_task_func func;
    void* context;
    uint32_t when;
    uint32_t flags;
};

bool rtic_schedule (rtic_task* task, uint32_t when, rtic_task_func func, void* context, uint32_t flags);
bool rtic_cancel (rtic_task* task);

uint32_t rtic_step (rtic_loop* loop, uint32_t now);


// Ticks comparison

#define rtic_tdiff(x,y)  ((int32_t)((uint32_t)(y) - (uint32_t)(x))) // how much time from x to y
#define rtic_after(x,y)  ( rtic_tdiff(x, y) < 0 )                   // true if x is after y
#define rtic_before(x,y) ( rtic_tdiff(x, y) > 0 )                   // true if x is before y


// Ticks/Time conversions

#ifndef RTIC_TICKS_PER_SEC
#define RTIC_TICKS_PER_SEC 32768
#elif RTIC_TICKS_PER_SEC < 10000 || RTIC_TICKS_PER_SEC > 64516
#error "Illegal RTIC_TICKS_PER_SEC - must be in range [10000:64516]. One tick must be 15.5us .. 100us long."
#endif

#define rtic_us2ticks(us)          ((uint32_t)(((uint64_t)(us)  * RTIC_TICKS_PER_SEC         ) / 1000000))
#define rtic_ms2ticks(ms)          ((uint32_t)(((uint64_t)(ms)  * RTIC_TICKS_PER_SEC         ) /    1000))
#define rtic_sec2ticks(sec)        ((uint32_t)(((uint64_t)(sec) * RTIC_TICKS_PER_SEC         )          ))

#define rtic_us2ticksCeil(us)      ((uint32_t)(((uint64_t)(us)  * RTIC_TICKS_PER_SEC + 999999) / 1000000))
#define rtic_ms2ticksCeil(ms)      ((uint32_t)(((uint64_t)(ms)  * RTIC_TICKS_PER_SEC +    999) /    1000))

#define rtic_us2ticksRound(us)     ((uint32_t)(((uint64_t)(us)  * RTIC_TICKS_PER_SEC + 500000) / 1000000))
#define rtic_ms2ticksRound(ms)     ((uint32_t)(((uint64_t)(ms)  * RTIC_TICKS_PER_SEC +    500) /    1000))

#define rtic_ticks2sec(ticks)      ((uint32_t)(((uint64_t)(ticks)                      ) / RTIC_TICKS_PER_SEC))
#define rtic_ticks2ms(ticks)       ((uint32_t)(((uint64_t)(ticks) *    1000            ) / RTIC_TICKS_PER_SEC))
#define rtic_ticks2us(ticks)       ((uint32_t)(((uint64_t)(ticks) * 1000000            ) / RTIC_TICKS_PER_SEC))

#define rtic_ticks2secCeil(ticks)  ((uint32_t)(((uint64_t)(ticks) + (OSTICKS_PER_SEC-1)) / RTIC_TICKS_PER_SEC))


// HAL - to be implemented externally

extern void rtic_hal_enter_critical (void* hal);
extern void rtic_hal_exit_critical (void* hal);
extern uint32_t rtic_hal_now (void);

#endif
