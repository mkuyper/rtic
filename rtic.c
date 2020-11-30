// Copyright (C) 2020-2020 Michael Kuyper. All rights reserved.
//
// This file is subject to the terms and conditions defined in file 'LICENSE',
// which is part of this source code package.

#include "rtic.h"

#ifndef ASSERT
#include <assert.h>
#define ASSERT(cond) assert(cond)
#endif

static bool remove_unsafe (rtic_task* task) {
    rtic_task** pnext;
    for( pnext = &task->loop->queue; *pnext; pnext = &(*pnext)->next ) {
        if( *pnext == task ) {
            *pnext = task->next;
            return true;
        }
    }
    return false;
}

bool rtic_cancel (rtic_task* task) {
    ASSERT(task->loop);

    rtic_hal_enter_critical(task->loop->hal);

    bool rv = remove_unsafe(task);
    
    rtic_hal_exit_critical(task->loop->hal);

    return rv;
}

bool rtic_schedule (rtic_task* task, uint32_t when, rtic_task_func func, void* context, uint32_t flags) {
    ASSERT(task->loop);

    rtic_hal_enter_critical(task->loop->hal);

    bool rv = remove_unsafe(task);

    task->when = when;
    task->func = func;
    task->context = context;
    task->flags = flags;

    rtic_task** pnext;
    for( pnext = &task->loop->queue; *pnext; pnext = &(*pnext)->next ) {
        if( rtic_before(when, (*pnext)->when) ) {
            break;
        }
    }
    task->next = *pnext;
    *pnext = task;

    rtic_hal_exit_critical(task->loop->hal);

    return rv;
}

uint32_t rtic_step (rtic_loop* loop, uint32_t now) {
    rtic_task* task = NULL;
    rtic_task_func func;
    void* context;
    uint32_t next;

    rtic_hal_enter_critical(loop->hal);

    if( loop->queue ) {
        if( rtic_after(loop->queue->when, now) ) {
            next = loop->queue->when;
        } else {
            task = loop->queue;
            remove_unsafe(task);
            // read function and context pointers within critical section
            func = task->func;
            context = task->context;
            next = now;
        }
    } else {
        next = now + 0x7fffffff;
    }

    rtic_hal_exit_critical(loop->hal);

    if( task ) {
        func(now, task, context);
    }

    return next;
}
