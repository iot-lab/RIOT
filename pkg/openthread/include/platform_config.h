/*
 * SPDX-FileCopyrightText: 2017-2021 Baptiste Clenet
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       Implementation of OpenThread platform config
 *
 * @author      Baptiste Clenet <bapclenet@gmail.com>
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in buffer pool
 */
#if OPENTHREAD_MTD
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS       (20U)
#else
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS       (10U)
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * Set OpenThread log level
 *
 * @see https://openthread.io/releases/thread-reference-20170716/group/plat-logging
 */
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_NONE

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
 *
 * The assertions of OpenThread call otPlatAssertFail(), implemented with the
 * assert() of RIOT
 */
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT 1

/**
 * @def OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
 *
 * The CLI prints on the RIOT shell: don't copy its input and output to the
 * logs (enabled by default on the native boards)
 */
#define OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE 0

#if OPENTHREAD_RADIO
/**
 * @name    SubMac of the Radio Co-Processor
 *
 * In a radio only build, the SubMac does in software only the
 * capabilities enabled here, when the radio doesn't have them. The hosts
 * (ot-cli, otbr-agent) require ACK timeout, retransmissions and transmit
 * security from the RCP.
 * @{
 */
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE   1
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE    1
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE  1
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE   1
/** @} */
#endif

#ifdef __cplusplus
}
#endif
