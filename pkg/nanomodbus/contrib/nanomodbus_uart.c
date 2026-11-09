/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     pkg_nanomodbus
 * @{
 *
 * @file
 * @brief       nanoMODBUS RTU over UART transport
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <errno.h>

#include "isrpipe/read_timeout.h"
#include "tsrb.h"

#include "nanomodbus_riot.h"

static void _rx_cb(void *arg, uint8_t data)
{
    nanomodbus_uart_t *dev = arg;

    isrpipe_write_one(&dev->isrpipe, data);
}

static int32_t _read(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    nanomodbus_uart_t *dev = arg;
    int res;

    if (byte_timeout_ms == 0) {
        /* non blocking */
        return tsrb_get(&dev->isrpipe.tsrb, buf, count);
    }

    if (byte_timeout_ms < 0) {
        uint16_t pos = 0;

        while (pos < count) {
            pos += isrpipe_read(&dev->isrpipe, &buf[pos], count - pos);
        }
        return count;
    }

    res = isrpipe_read_all_timeout(&dev->isrpipe, buf, count, (uint32_t)byte_timeout_ms * 1000);
    /* a timeout is reported to nanoMODBUS as a short read */
    return (res == -ETIMEDOUT) ? 0 : res;
}

static void _set_de(const nanomodbus_uart_params_t *params, bool enable)
{
    if (gpio_is_valid(params->de_pin)) {
        gpio_write(params->de_pin, enable != params->de_active_low);
    }
}

static int32_t _write(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    nanomodbus_uart_t *dev = arg;

    (void)byte_timeout_ms;

    _set_de(&dev->params, true);
    uart_write(dev->params.uart, buf, count);
    _set_de(&dev->params, false);

    return count;
}

static void _flush(nmbs_t *nmbs, void *arg)
{
    nanomodbus_uart_t *dev = arg;

    (void)nmbs;
    isrpipe_clear(&dev->isrpipe);
}

int nanomodbus_uart_init(nanomodbus_uart_t *dev, const nanomodbus_uart_params_t *params,
                         nmbs_platform_conf *conf)
{
    dev->params = *params;
    isrpipe_init(&dev->isrpipe, dev->buf, sizeof(dev->buf));

    if (gpio_is_valid(params->de_pin)) {
        if (gpio_init(params->de_pin, GPIO_OUT) != 0) {
            return -EIO;
        }
        _set_de(params, false);
    }

    int res = uart_init(params->uart, params->baudrate, _rx_cb, dev);

    if (res != UART_OK) {
        return -EIO;
    }

    nmbs_platform_conf_create(conf);
    conf->transport = NMBS_TRANSPORT_RTU;
    conf->read = _read;
    conf->write = _write;
    conf->flush = _flush;
    conf->arg = dev;

    return 0;
}
