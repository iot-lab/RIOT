/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     pkg_zenoh-pico
 * @{
 *
 * @file
 * @brief       zenoh-pico system layer for RIOT
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>

#include "container.h"
#include "irq.h"
#include "list.h"
#include "random.h"
#include "sched.h"
#include "thread.h"
#include "timex.h"
#include "ztimer.h"
#include "ztimer64.h"

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

/**
 * @brief   Stack size of the tasks started by zenoh-pico (read and lease
 *          tasks), the subscriber and queryable callbacks run in the read task
 */
#ifndef CONFIG_ZENOH_PICO_TASK_STACKSIZE
#define CONFIG_ZENOH_PICO_TASK_STACKSIZE    (THREAD_STACKSIZE_DEFAULT * 4)
#endif

/*------------------ Random ------------------*/
uint8_t z_random_u8(void)
{
    return random_uint32();
}

uint16_t z_random_u16(void)
{
    return random_uint32();
}

uint32_t z_random_u32(void)
{
    return random_uint32();
}

uint64_t z_random_u64(void)
{
    return ((uint64_t)random_uint32() << 32) | random_uint32();
}

void z_random_fill(void *buf, size_t len)
{
    random_bytes(buf, len);
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size)
{
    return malloc(size);
}

void *z_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void z_free(void *ptr)
{
    free(ptr);
}

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Task ------------------*/
/**
 * @brief   Priority of the tasks started by zenoh-pico
 */
#ifndef CONFIG_ZENOH_PICO_TASK_PRIORITY
#define CONFIG_ZENOH_PICO_TASK_PRIORITY     (THREAD_PRIORITY_MAIN - 1)
#endif

struct _z_riot_task {
    kernel_pid_t pid;
    void *stack;
    void *(*fun)(void *);
    void *arg;
};

static void *_task_trampoline(void *arg)
{
    struct _z_riot_task *task = arg;

    task->fun(task->arg);
    /* keep the stack until the task is joined */
    thread_zombify();
    return NULL;
}

z_result_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg)
{
    size_t stack_size = attr ? attr->stack_size : CONFIG_ZENOH_PICO_TASK_STACKSIZE;
    uint8_t priority = attr ? attr->priority : CONFIG_ZENOH_PICO_TASK_PRIORITY;
    struct _z_riot_task *t = z_malloc(sizeof(*t));

    if (t == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    t->stack = z_malloc(stack_size);
    if (t->stack == NULL) {
        z_free(t);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    t->fun = fun;
    t->arg = arg;
    t->pid = thread_create(t->stack, stack_size, priority, 0, _task_trampoline, t, "zenoh");
    if (t->pid < 0) {
        z_free(t->stack);
        z_free(t);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_TASK_FAILED);
    }
    *task = t;
    return _Z_RES_OK;
}

z_result_t _z_task_join(_z_task_t *task)
{
    struct _z_riot_task *t = *task;

    /* wait for the task to return */
    while (thread_kill_zombie(t->pid) != 1) {
        ztimer_sleep(ZTIMER_MSEC, 1);
    }
    z_free(t->stack);
    z_free(t);
    *task = NULL;
    return _Z_RES_OK;
}

z_result_t _z_task_detach(_z_task_t *task)
{
    /* a detached task stays a zombie with its stack allocated once it returns,
     * zenoh-pico only joins its tasks */
    *task = NULL;
    return _Z_RES_OK;
}

z_result_t _z_task_cancel(_z_task_t *task)
{
    (void)task;
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

void _z_task_exit(void)
{
    thread_zombify();
}

void _z_task_free(_z_task_t **task)
{
    z_free(*task);
    *task = NULL;
}

_z_task_id_t _z_task_get_id(const _z_task_t *task)
{
    return (*task)->pid;
}

_z_task_id_t _z_task_current_id(void)
{
    return thread_getpid();
}

bool _z_task_id_equal(const _z_task_id_t *l, const _z_task_id_t *r)
{
    return *l == *r;
}

/*------------------ Mutex ------------------*/
z_result_t _z_mutex_init(_z_mutex_t *m)
{
    mutex_init(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_drop(_z_mutex_t *m)
{
    (void)m;
    return _Z_RES_OK;
}

z_result_t _z_mutex_lock(_z_mutex_t *m)
{
    mutex_lock(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_try_lock(_z_mutex_t *m)
{
    return mutex_trylock(m) ? _Z_RES_OK : _Z_ERR_SYSTEM_GENERIC;
}

z_result_t _z_mutex_unlock(_z_mutex_t *m)
{
    mutex_unlock(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_init(_z_mutex_rec_t *m)
{
    rmutex_init(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_drop(_z_mutex_rec_t *m)
{
    (void)m;
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_lock(_z_mutex_rec_t *m)
{
    rmutex_lock(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_try_lock(_z_mutex_rec_t *m)
{
    return rmutex_trylock(m) ? _Z_RES_OK : _Z_ERR_SYSTEM_GENERIC;
}

z_result_t _z_mutex_rec_unlock(_z_mutex_rec_t *m)
{
    rmutex_unlock(m);
    return _Z_RES_OK;
}

/*------------------ Condvar ------------------*/
/* same implementation as the RIOT core cond_t, with a timed wait */
typedef struct {
    _z_condvar_t *cv;
    thread_t *thread;
    bool timed_out;
} _condvar_timeout_t;

static void _condvar_wake(_z_condvar_t *cv, bool all)
{
    unsigned state = irq_disable();
    uint16_t min_prio = THREAD_PRIORITY_MIN + 1;
    list_node_t *next;

    while ((next = list_remove_head(&cv->queue)) != NULL) {
        thread_t *thread = container_of((clist_node_t *)next, thread_t, rq_entry);

        sched_set_status(thread, STATUS_PENDING);
        if (thread->priority < min_prio) {
            min_prio = thread->priority;
        }
        if (!all) {
            break;
        }
    }
    irq_restore(state);
    if (min_prio <= THREAD_PRIORITY_MIN) {
        sched_switch(min_prio);
    }
}

static void _condvar_timeout(void *arg)
{
    _condvar_timeout_t *timeout = arg;
    unsigned state = irq_disable();

    /* the thread is still waiting if it was not signaled */
    if (list_remove(&timeout->cv->queue, (list_node_t *)&timeout->thread->rq_entry)) {
        sched_set_status(timeout->thread, STATUS_PENDING);
        timeout->timed_out = true;
    }
    irq_restore(state);
    if (timeout->timed_out) {
        sched_switch(timeout->thread->priority);
    }
}

static void _condvar_wait(_z_condvar_t *cv, _z_mutex_t *m, ztimer64_t *timer, uint64_t duration)
{
    unsigned state = irq_disable();
    thread_t *me = thread_get_active();

    /* the thread is queued before the timer is started, no wake up is lost */
    mutex_unlock(m);
    sched_set_status(me, STATUS_COND_BLOCKED);
    thread_add_to_list(&cv->queue, me);
    if (timer) {
        ztimer64_set(ZTIMER64_USEC, timer, duration);
    }
    irq_restore(state);
    thread_yield_higher();

    if (timer) {
        ztimer64_remove(ZTIMER64_USEC, timer);
    }
    mutex_lock(m);
}

z_result_t _z_condvar_init(_z_condvar_t *cv)
{
    cv->queue.next = NULL;
    return _Z_RES_OK;
}

z_result_t _z_condvar_drop(_z_condvar_t *cv)
{
    (void)cv;
    return _Z_RES_OK;
}

z_result_t _z_condvar_signal(_z_condvar_t *cv)
{
    _condvar_wake(cv, false);
    return _Z_RES_OK;
}

z_result_t _z_condvar_signal_all(_z_condvar_t *cv)
{
    _condvar_wake(cv, true);
    return _Z_RES_OK;
}

z_result_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m)
{
    _condvar_wait(cv, m, NULL, 0);
    return _Z_RES_OK;
}

z_result_t _z_condvar_wait_until(_z_condvar_t *cv, _z_mutex_t *m, const z_clock_t *abstime)
{
    z_clock_t now = z_clock_now();

    if (now >= *abstime) {
        return Z_ETIMEDOUT;
    }

    _condvar_timeout_t timeout = { .cv = cv, .thread = thread_get_active() };
    ztimer64_t timer = { .callback = _condvar_timeout, .arg = &timeout };

    _condvar_wait(cv, m, &timer, *abstime - now);
    return timeout.timed_out ? Z_ETIMEDOUT : _Z_RES_OK;
}

#endif /* Z_FEATURE_MULTI_THREAD == 1 */

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time)
{
    ztimer64_sleep(ZTIMER64_USEC, time);
    return _Z_RES_OK;
}

z_result_t z_sleep_ms(size_t time)
{
    ztimer_sleep(ZTIMER_MSEC, time);
    return _Z_RES_OK;
}

z_result_t z_sleep_s(size_t time)
{
    ztimer_sleep(ZTIMER_MSEC, time * MS_PER_SEC);
    return _Z_RES_OK;
}

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void)
{
    return ztimer64_now(ZTIMER64_USEC);
}

unsigned long zp_clock_elapsed_us_since(z_clock_t *instant, z_clock_t *epoch)
{
    return (*instant > *epoch) ? (unsigned long)(*instant - *epoch) : 0;
}

unsigned long zp_clock_elapsed_ms_since(z_clock_t *instant, z_clock_t *epoch)
{
    return zp_clock_elapsed_us_since(instant, epoch) / US_PER_MS;
}

unsigned long zp_clock_elapsed_s_since(z_clock_t *instant, z_clock_t *epoch)
{
    return zp_clock_elapsed_us_since(instant, epoch) / US_PER_SEC;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant)
{
    z_clock_t now = z_clock_now();

    return zp_clock_elapsed_us_since(&now, instant);
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant)
{
    return z_clock_elapsed_us(instant) / US_PER_MS;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant)
{
    return z_clock_elapsed_us(instant) / US_PER_SEC;
}

void z_clock_advance_us(z_clock_t *clock, unsigned long duration)
{
    *clock += duration;
}

void z_clock_advance_ms(z_clock_t *clock, unsigned long duration)
{
    *clock += (uint64_t)duration * US_PER_MS;
}

void z_clock_advance_s(z_clock_t *clock, unsigned long duration)
{
    *clock += (uint64_t)duration * US_PER_SEC;
}

/*------------------ Time ------------------*/
z_time_t z_time_now(void)
{
    return ztimer64_now(ZTIMER64_USEC);
}

const char *z_time_now_as_str(char *const buf, unsigned long buflen)
{
    snprintf(buf, buflen, "%lu", (unsigned long)(z_time_now() / US_PER_MS));
    return buf;
}

unsigned long z_time_elapsed_us(z_time_t *time)
{
    z_time_t now = z_time_now();

    return (now > *time) ? (unsigned long)(now - *time) : 0;
}

unsigned long z_time_elapsed_ms(z_time_t *time)
{
    return z_time_elapsed_us(time) / US_PER_MS;
}

unsigned long z_time_elapsed_s(z_time_t *time)
{
    return z_time_elapsed_us(time) / US_PER_SEC;
}

z_result_t _z_get_time_since_epoch(_z_time_since_epoch *t)
{
    z_time_t now = z_time_now();

    t->secs = now / US_PER_SEC;
    t->nanos = (now % US_PER_SEC) * NS_PER_US;
    return _Z_RES_OK;
}
