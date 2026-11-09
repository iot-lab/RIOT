/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     pkg_nanomodbus
 * @{
 *
 * @file
 * @brief       nanoMODBUS TCP transport
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <errno.h>

#include "nanomodbus_riot.h"

static uint32_t _timeout_us(int32_t timeout_ms)
{
    return (timeout_ms < 0) ? SOCK_NO_TIMEOUT : (uint32_t)timeout_ms * 1000;
}

static int32_t _read(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    sock_tcp_t *sock = arg;
    uint16_t pos = 0;

    while (pos < count) {
        ssize_t res = sock_tcp_read(sock, &buf[pos], count - pos, _timeout_us(byte_timeout_ms));

        if ((res == -ETIMEDOUT) || (res == -EAGAIN)) {
            /* a timeout is reported to nanoMODBUS as a short read */
            break;
        }
        if (res <= 0) {
            /* error or connection closed */
            return -1;
        }
        pos += res;
    }

    return pos;
}

static int32_t _write(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    sock_tcp_t *sock = arg;
    uint16_t pos = 0;

    (void)byte_timeout_ms;

    while (pos < count) {
        ssize_t res = sock_tcp_write(sock, &buf[pos], count - pos);

        if (res <= 0) {
            return -1;
        }
        pos += res;
    }

    return pos;
}

void nanomodbus_tcp_init(sock_tcp_t *sock, nmbs_platform_conf *conf)
{
    nmbs_platform_conf_create(conf);
    conf->transport = NMBS_TRANSPORT_TCP;
    conf->read = _read;
    conf->write = _write;
    conf->arg = sock;
}
