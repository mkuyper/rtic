// Copyright (C) 2020-2020 Michael Kuyper. All rights reserved.
//
// This file is subject to the terms and conditions defined in file 'LICENSE',
// which is part of this source code package.

#include <stdarg.h>

#include <criterion/criterion.h>

#include "rtic.h"

void rtic_hal_enter_critical (void* hal) {
    (void) hal;
}

void rtic_hal_exit_critical (void* hal) {
    (void) hal;
}

typedef struct {
    rtic_task* task;
} runcontext;

static void func1 (uint32_t now, rtic_task* task, void* context) {
    runcontext* rc = context;
    cr_assert_eq(task, rc->task, "unexpected task actual=%p, expected=%p");
}

static void rtic_schedule_c (rtic_task* task, uint32_t when, rtic_task_func func, void* context, uint32_t flags) {
    rtic_schedule(task, when, func, context, flags);
    cr_assert_eq(task->func, func);
    cr_assert_eq(task->context, context);
    cr_assert_eq(task->when, when);
    cr_assert_eq(task->flags, flags);
}

static void check_queue (rtic_task* head, const char* explain, int count, ...) {
    va_list ap;
    va_start(ap, count);
    for( int i = 0 ; i < count; i++ ) {
        void* exp = va_arg(ap, void*);
        cr_assert_eq(head, exp, "queue check (%s): element=%d, actual=%p, expected=%p",
                explain, i, head, exp);
        head = head->next;
    }
    cr_assert_eq(head, NULL, "queue check (%s): last, actual=%p, expected=%p",
            explain, head, NULL);
}

Test(rtic, scheduletask) {
    runcontext rc;

    rtic_loop loop = { .hal = NULL };
    check_queue(loop.queue, "empty", 0);

    rtic_task t1 = { .loop = &loop };
    rtic_schedule_c(&t1, 10, func1, &rc, 0);
    check_queue(loop.queue, "t1", 1, &t1);

    rtic_task t2 = { .loop = &loop };
    rtic_schedule_c(&t2, 30, func1, &rc, 0);
    check_queue(loop.queue, "t1,t2", 2, &t1, &t2);

    rtic_task t3 = { .loop = &loop };
    rtic_schedule_c(&t3, 20, func1, &rc, 0);
    check_queue(loop.queue, "t1,t3,t2", 3, &t1, &t3, &t2);

    rtic_task t4 = { .loop = &loop };
    rtic_schedule_c(&t4, -10, func1, &rc, 0);
    check_queue(loop.queue, "t4,t1,t3,t2", 4, &t4, &t1, &t3, &t2);

    // remove middle
    rtic_cancel(&t1);
    check_queue(loop.queue, "t4,t3,t2", 3, &t4, &t3, &t2);

    rtic_schedule_c(&t1, 10, func1, &rc, 0);
    check_queue(loop.queue, "re-add 1, t4,t1,t3,t2", 4, &t4, &t1, &t3, &t2);

    // remove front
    rtic_cancel(&t4);
    check_queue(loop.queue, "t1,t3,t2", 3, &t1, &t3, &t2);

    rtic_schedule_c(&t4, -10, func1, &rc, 0);
    check_queue(loop.queue, "re-add 4, t4,t1,t3,t2", 4, &t4, &t1, &t3, &t2);

    // remove back
    rtic_cancel(&t2);
    check_queue(loop.queue, "t4,t1,t3", 3, &t4, &t1, &t3);

    // re-use in front
    rtic_schedule_c(&t2, -20, func1, &rc, 0);
    check_queue(loop.queue, "t2,t4,t1,t3", 4, &t2, &t4, &t1, &t3);

    // run
    uint32_t next;

    // early
    rc.task = NULL;
    next = rtic_step(&loop, -50);
    cr_assert_eq(next, -20);

    // on time
    rc.task = &t2;
    next = rtic_step(&loop, -20);
    cr_assert_eq(next, -20);
    check_queue(loop.queue, "t4,t1,t3", 3, &t4, &t1, &t3);

    // late
    rc.task = &t4;
    next = rtic_step(&loop, -5);
    cr_assert_eq(next, -5);
    check_queue(loop.queue, "t1,t3", 2, &t1, &t3);

    // late for 2
    rc.task = &t1;
    next = rtic_step(&loop, 50);
    cr_assert_eq(next, 50);
    check_queue(loop.queue, "t3", 1, &t3);
    rc.task = &t3;
    next = rtic_step(&loop, 50);
    cr_assert_eq(next, 50);
    check_queue(loop.queue, "-", 0);

    // no more jobs
    rc.task = NULL;
    next = rtic_step(&loop, 100);
    cr_assert_eq(next, 100 + 0x7fffffffu);
}
