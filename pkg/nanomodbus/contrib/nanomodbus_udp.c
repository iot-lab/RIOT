/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     pkg_nanomodbus
 * @{
 *
 * @file
 * @brief       nanoMODBUS over UDP transport
 *
 * nanoMODBUS reads a frame in several steps (header, then the rest), so the
 * received datagram is buffered and handed out on each read. A frame is
 * written with a single write, which is sent as one datagram.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <errno.h>
#include <string.h>

#include "nanomodbus_riot.h"

static uint32_t _timeout_us(int32_t timeout_ms)
{
    return (timeout_ms < 0) ? SOCK_NO_TIMEOUT : (uint32_t)timeout_ms * 1000;
}

static int32_t _read(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    nanomodbus_udp_t *dev = arg;

    if (dev->pos == dev->len) {
        sock_udp_ep_t remote;
        ssize_t res = sock_udp_recv(dev->sock, dev->buf, sizeof(dev->buf),
                                    _timeout_us(byte_timeout_ms), &remote);

        if ((res == -ETIMEDOUT) || (res == -EAGAIN)) {
            /* a timeout is reported to nanoMODBUS as a short read */
            return 0;
        }
        if (res < 0) {
            return -1;
        }
        if (dev->server) {
            dev->remote = remote;
        }
        dev->len = res;
        dev->pos = 0;
    }

    /* a frame never spans several datagrams, a short read means the
     * datagram is shorter than expected */
    uint16_t len = dev->len - dev->pos;

    if (len > count) {
        len = count;
    }
    memcpy(buf, &dev->buf[dev->pos], len);
    dev->pos += len;

    return len;
}

static int32_t _write(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    nanomodbus_udp_t *dev = arg;

    (void)byte_timeout_ms;

    ssize_t res = sock_udp_send(dev->sock, buf, count, &dev->remote);

    return (res == (ssize_t)count) ? (int32_t)count : -1;
}

static void _flush(nmbs_t *nmbs, void *arg)
{
    nanomodbus_udp_t *dev = arg;

    (void)nmbs;

    /* drop the buffered datagram and the ones already queued */
    dev->len = 0;
    dev->pos = 0;
    while (sock_udp_recv(dev->sock, dev->buf, sizeof(dev->buf), 0, NULL) > 0) {}
}

void nanomodbus_udp_init(nanomodbus_udp_t *dev, sock_udp_t *sock, const sock_udp_ep_t *remote,
                         nmbs_platform_conf *conf)
{
    memset(dev, 0, sizeof(*dev));
    dev->sock = sock;
    dev->server = (remote == NULL);
    if (remote) {
        dev->remote = *remote;
    }

    nmbs_platform_conf_create(conf);
    conf->transport = NMBS_TRANSPORT_TCP;
    conf->read = _read;
    conf->write = _write;
    conf->flush = _flush;
    conf->arg = dev;
}
