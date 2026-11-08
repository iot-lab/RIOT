/*
 * SPDX-FileCopyrightText: 2017 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @defgroup    pkg_openthread_cli   OpenThread
 * @ingroup     pkg_openthread
 * @brief       An open source implementation of Thread stack
 * @see         https://github.com/openthread/openthread
 *
 * Thread is a mesh oriented network stack running for IEEE802.15.4 networks.
 *
 * OpenThread runs in its own thread, on an event queue: the OpenThread API
 * must only be called from that thread. Post an event to the queue returned by
 * @ref openthread_get_evq to call it from the application:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.c}
 * #include "ot.h"
 * #include "openthread/thread.h"
 * static void _panid_handler(event_t *event);
 * static event_t event_panid = {
 *     .handler = _panid_handler
 * };
 * static void _panid_handler(event_t *event)
 * {
 *     (void) event;
 *     uint16_t panid = otLinkGetPanId(openthread_get_instance());
 *     do_something_with_panid(panid);
 * }
 * int main(void)
 * {
 *     event_post(openthread_get_evq(), &event_panid);
 *     return 0;
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The radio is driven through the IEEE 802.15.4 Radio HAL: every radio of
 * RIOT with a Radio HAL implementation can be used.
 *
 * When `OPENTHREAD_PANID` and `OPENTHREAD_CHANNEL` are defined, OpenThread
 * starts at boot with an active dataset built from them (and the defaults of
 * the other `OPENTHREAD_*` dataset macros): the nodes built with the same
 * values form one network.
 *
 * @{
 * @file
 * @author      Jose Ignacio Alamos <jialamos@uc.cl>
 * @author      Baptiste Clenet <bapclenet@gmail.com>
 */

#include "event.h"
#include "net/ieee802154/radio.h"
#include "openthread/instance.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    OpenThread configuration
 * @{
 */
#ifndef OPENTHREAD_STACKSIZE
/**
 * @brief   Stack size of the OpenThread thread
 */
#define OPENTHREAD_STACKSIZE        (THREAD_STACKSIZE_MAIN * 2)
#endif

#ifndef OPENTHREAD_PRIORITY
/**
 * @brief   Priority of the OpenThread thread
 */
#define OPENTHREAD_PRIORITY         (THREAD_PRIORITY_MAIN - 5)
#endif

#ifndef OPENTHREAD_RX_SENSITIVITY
/**
 * @brief   Receive sensitivity of the radio in dBm, reported to OpenThread
 *          (the Radio HAL doesn't provide it)
 */
#define OPENTHREAD_RX_SENSITIVITY   (-100)
#endif

#ifndef OPENTHREAD_NETWORK_NAME
/**
 * @brief   Network name of the dataset started at boot
 */
#define OPENTHREAD_NETWORK_NAME     "RIOT"
#endif

#ifndef OPENTHREAD_EXT_PANID
/**
 * @brief   Extended PAN ID of the dataset started at boot (8 bytes)
 */
#define OPENTHREAD_EXT_PANID        { 0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe }
#endif

#ifndef OPENTHREAD_NETWORK_KEY
/**
 * @brief   Network key of the dataset started at boot (16 bytes)
 */
#define OPENTHREAD_NETWORK_KEY      { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, \
                                      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff }
#endif

#ifndef OPENTHREAD_MESH_LOCAL_PREFIX
/**
 * @brief   Mesh local prefix of the dataset started at boot (8 bytes)
 */
#define OPENTHREAD_MESH_LOCAL_PREFIX { 0xfd, 0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0x00 }
#endif
/** @} */

/**
 * @brief   Get the OpenThread event queue
 *
 * @return  pointer to the event queue
 */
event_queue_t *openthread_get_evq(void);

/**
 * @brief   Get the OpenThread instance
 *
 * @return  pointer to the OpenThread instance
 */
otInstance *openthread_get_instance(void);

/**
 * @brief   Bootstrap OpenThread: initialize the radio and start the
 *          OpenThread thread (called by auto_init)
 */
void openthread_bootstrap(void);

/**
 * @brief   Initialize the OpenThread radio layer
 *
 * @internal Called from the OpenThread thread, before the OpenThread
 *           instance is initialized.
 *
 * @param[in]  dev  Radio HAL device of the radio
 *
 * @retval  0 on success
 * @retval  negative errno on error
 */
int openthread_radio_init(ieee802154_dev_t *dev);

/**
 * @brief   Initialize the OpenThread CLI on the RIOT shell (`ot` command)
 *
 * @internal Called from the OpenThread thread.
 *
 * @param[in]  instance  OpenThread instance
 */
void ot_shell_init(otInstance *instance);

#ifdef __cplusplus
}
#endif

/** @} */
