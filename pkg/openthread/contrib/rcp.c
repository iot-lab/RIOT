/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       OpenThread Radio Co-Processor: spinel over HDLC on a UART
 *
 * The bytes received on the UART are queued in a ring buffer by the UART
 * interrupt and given to OpenThread in the OpenThread thread. The frames
 * of OpenThread are written synchronously on the UART.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @}
 */

#include <stdint.h>

#include "event.h"
#include "periph/uart.h"
#include "tsrb.h"

#include "openthread/ncp.h"
#include "ot.h"

#define ENABLE_DEBUG 0
#include "debug.h"

static uint8_t _rx_mem[OPENTHREAD_RCP_RX_BUFSIZE];
static tsrb_t _rx_buf;

static void _rx_handler(event_t *event)
{
    (void)event;
    uint8_t buf[64];
    int len;

    while ((len = tsrb_get(&_rx_buf, buf, sizeof(buf))) > 0) {
        otNcpHdlcReceive(buf, len);
    }
}

static void _send_done_handler(event_t *event)
{
    (void)event;
    otNcpHdlcSendDone();
}

static event_t _ev_rx = { .handler = _rx_handler };
static event_t _ev_send_done = { .handler = _send_done_handler };

static void _uart_cb(void *arg, uint8_t data)
{
    (void)arg;
    if (tsrb_add_one(&_rx_buf, data) < 0) {
        /* lost: the HDLC frame is dropped by its FCS, the host retries */
        DEBUG("openthread_rcp: RX buffer full\n");
    }
    event_post(openthread_get_evq(), &_ev_rx);
}

static int _send(const uint8_t *buf, uint16_t len)
{
    uart_write(OPENTHREAD_RCP_UART_DEV, buf, len);
    /* the send done callback must not be called from the send callback */
    event_post(openthread_get_evq(), &_ev_send_done);
    return len;
}

void openthread_rcp_init(otInstance *instance)
{
    tsrb_init(&_rx_buf, _rx_mem, sizeof(_rx_mem));
    if (uart_init(OPENTHREAD_RCP_UART_DEV, OPENTHREAD_RCP_UART_BAUDRATE, _uart_cb, NULL)
        != UART_OK) {
        DEBUG("openthread_rcp: cannot initialize the UART\n");
        return;
    }
    otNcpHdlcInit(instance, _send);
}
