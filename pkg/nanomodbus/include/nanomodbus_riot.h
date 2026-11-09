/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup     pkg_nanomodbus
 * @{
 *
 * @file
 * @brief       RIOT transports for nanoMODBUS
 *
 * Each transport fills a `nmbs_platform_conf` that is then passed to
 * `nmbs_client_create()` or `nmbs_server_create()`.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#include <stdbool.h>
#include <stdint.h>

#include "modules.h"
#include "nanomodbus.h"

#if IS_USED(MODULE_NANOMODBUS_UART)
#include "isrpipe.h"
#include "periph/gpio.h"
#include "periph/uart.h"
#endif

#if IS_USED(MODULE_NANOMODBUS_TCP)
#include "net/sock/tcp.h"
#endif

#if IS_USED(MODULE_NANOMODBUS_UDP)
#include "net/sock/udp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Maximum size of a Modbus frame (TCP ADU), in bytes
 */
#define NANOMODBUS_FRAME_SIZE_MAX       (260U)

/**
 * @brief   Size of the UART receive buffer, in bytes
 */
#ifndef CONFIG_NANOMODBUS_UART_BUF_SIZE
#define CONFIG_NANOMODBUS_UART_BUF_SIZE (256U)
#endif

#if IS_USED(MODULE_NANOMODBUS_UART) || DOXYGEN
/**
 * @brief   Parameters of the RTU over UART transport
 */
typedef struct {
    uart_t uart;            /**< UART device */
    uint32_t baudrate;      /**< baudrate */
    gpio_t de_pin;          /**< RS-485 driver enable pin, GPIO_UNDEF if not used */
    bool de_active_low;     /**< driver enable pin is active low */
} nanomodbus_uart_params_t;

/**
 * @brief   RTU over UART transport
 */
typedef struct {
    nanomodbus_uart_params_t params;                /**< parameters */
    isrpipe_t isrpipe;                              /**< received bytes */
    uint8_t buf[CONFIG_NANOMODBUS_UART_BUF_SIZE];   /**< isrpipe buffer */
} nanomodbus_uart_t;

/**
 * @brief   Initialize the RTU over UART transport
 *
 * The driver enable pin, if used, is set before writing a frame and cleared
 * once `uart_write()` returns. This requires the UART driver to return only
 * once the last byte has been sent, which is not the case for all CPUs.
 *
 * @param[out]  dev     transport state, must stay valid while it is used
 * @param[in]   params  transport parameters
 * @param[out]  conf    nanoMODBUS platform configuration to fill
 *
 * @return  0 on success
 * @return  negative errno on failure
 */
int nanomodbus_uart_init(nanomodbus_uart_t *dev, const nanomodbus_uart_params_t *params,
                         nmbs_platform_conf *conf);
#endif

#if IS_USED(MODULE_NANOMODBUS_TCP) || DOXYGEN
/**
 * @brief   Initialize the Modbus TCP transport over a connected TCP sock
 *
 * Connecting (client) or accepting the connection (server) is up to the
 * application.
 *
 * @param[in]   sock    connected TCP sock, must stay valid while it is used
 * @param[out]  conf    nanoMODBUS platform configuration to fill
 */
void nanomodbus_tcp_init(sock_tcp_t *sock, nmbs_platform_conf *conf);
#endif

#if IS_USED(MODULE_NANOMODBUS_UDP) || DOXYGEN
/**
 * @brief   Modbus over UDP transport
 *
 * Each UDP datagram carries one Modbus TCP frame (MBAP header and PDU).
 */
typedef struct {
    sock_udp_t *sock;                           /**< UDP sock */
    sock_udp_ep_t remote;                       /**< remote endpoint */
    bool server;                                /**< reply to the sender of the last request */
    uint16_t len;                               /**< length of the buffered datagram */
    uint16_t pos;                               /**< read position in the buffered datagram */
    uint8_t buf[NANOMODBUS_FRAME_SIZE_MAX];     /**< buffered datagram */
} nanomodbus_udp_t;

/**
 * @brief   Initialize the Modbus over UDP transport
 *
 * @param[out]  dev     transport state, must stay valid while it is used
 * @param[in]   sock    UDP sock, must stay valid while it is used
 * @param[in]   remote  server endpoint for a client, NULL for a server which
 *                      replies to the sender of each request
 * @param[out]  conf    nanoMODBUS platform configuration to fill
 */
void nanomodbus_udp_init(nanomodbus_udp_t *dev, sock_udp_t *sock, const sock_udp_ep_t *remote,
                         nmbs_platform_conf *conf);
#endif

#ifdef __cplusplus
}
#endif

/** @} */
