/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup     pkg_zenoh-pico
 * @{
 *
 * @file
 * @brief       zenoh-pico platform types for RIOT
 *
 * Tasks are RIOT threads, mutexes are the RIOT core ones and the links use
 * the sock API. Condition variables are implemented on the RIOT scheduler as
 * the core cond_t has no timed wait.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/config.h"

#if Z_FEATURE_MULTI_THREAD == 1
#include "list.h"
#include "mutex.h"
#include "rmutex.h"
#include "sched.h"
#endif

#if Z_FEATURE_LINK_TCP == 1
#include "net/sock/tcp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_MULTI_THREAD == 1
/**
 * @brief   Task attributes
 */
typedef struct {
    size_t stack_size;      /**< stack size, allocated with malloc() */
    uint8_t priority;       /**< RIOT thread priority */
} z_task_attr_t;

typedef struct _z_riot_task *_z_task_t;     /**< task */
typedef kernel_pid_t _z_task_id_t;          /**< task identifier */
typedef mutex_t _z_mutex_t;                 /**< mutex */
typedef rmutex_t _z_mutex_rec_t;            /**< recursive mutex */

/**
 * @brief   Condition variable
 */
typedef struct {
    list_node_t queue;      /**< waiting threads */
} _z_condvar_t;
#endif

/**
 * @brief   Monotonic clock, in microseconds since boot (ZTIMER64_USEC)
 */
typedef uint64_t z_clock_t;

/**
 * @brief   System time, in microseconds since boot as RIOT has no wall clock
 *          by default
 */
typedef uint64_t z_time_t;

/**
 * @brief   Socket of a link
 */
typedef struct {
#if Z_FEATURE_LINK_TCP == 1
    sock_tcp_t *_tcp;               /**< connected TCP sock */
    sock_tcp_queue_t *_tcp_queue;   /**< TCP listening queue */
    uint32_t _timeout_us;           /**< read timeout */
    bool _accepted;                 /**< the sock belongs to a listening queue */
#endif
} _z_sys_net_socket_t;

/**
 * @brief   Endpoint of a link
 */
typedef struct {
#if Z_FEATURE_LINK_TCP == 1
    sock_tcp_ep_t _ep;              /**< IP address and port */
#endif
} _z_sys_net_endpoint_t;

#ifdef __cplusplus
}
#endif

/** @} */
