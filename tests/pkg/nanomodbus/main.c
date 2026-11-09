/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       nanoMODBUS client and server over UDP on the loopback
 *
 * A server thread holds 8 holding registers, the client writes some of them,
 * reads them back and reads out of range.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <stdio.h>

#include "net/ipv6/addr.h"
#include "net/sock/udp.h"
#include "thread.h"

#include "nanomodbus_riot.h"

#define MODBUS_PORT         (5020U)
#define REGISTERS_NUMOF     (8U)
#define TIMEOUT_MS          (1000U)

static uint16_t _registers[REGISTERS_NUMOF];
static char _server_stack[THREAD_STACKSIZE_DEFAULT];

static nmbs_error _read_holding_registers(uint16_t address, uint16_t quantity,
                                          uint16_t *registers_out, uint8_t unit_id,
                                          void *arg)
{
    (void)unit_id;
    (void)arg;

    if ((address + quantity) > REGISTERS_NUMOF) {
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }
    for (unsigned i = 0; i < quantity; i++) {
        registers_out[i] = _registers[address + i];
    }
    return NMBS_ERROR_NONE;
}

static nmbs_error _write_multiple_registers(uint16_t address, uint16_t quantity,
                                            const uint16_t *registers, uint8_t unit_id,
                                            void *arg)
{
    (void)unit_id;
    (void)arg;

    if ((address + quantity) > REGISTERS_NUMOF) {
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }
    for (unsigned i = 0; i < quantity; i++) {
        _registers[address + i] = registers[i];
    }
    return NMBS_ERROR_NONE;
}

static void *_server(void *arg)
{
    (void)arg;

    static sock_udp_t sock;
    static nanomodbus_udp_t udp;
    static nmbs_t server;
    nmbs_platform_conf conf;
    nmbs_callbacks callbacks;
    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;

    local.port = MODBUS_PORT;
    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("cannot create the server sock");
        return NULL;
    }
    /* NULL remote: reply to the sender of each request */
    nanomodbus_udp_init(&udp, &sock, NULL, &conf);
    nmbs_callbacks_create(&callbacks);
    callbacks.read_holding_registers = _read_holding_registers;
    callbacks.write_multiple_registers = _write_multiple_registers;
    nmbs_server_create(&server, 0, &conf, &callbacks);

    while (1) {
        nmbs_server_poll(&server);
    }
    return NULL;
}

static int _check(const char *what, nmbs_error err, nmbs_error expected)
{
    printf("%s: %s\n", what, nmbs_strerror(err));
    return (err == expected) ? 0 : 1;
}

int main(void)
{
    static sock_udp_t sock;
    static nanomodbus_udp_t udp;
    static nmbs_t client;
    nmbs_platform_conf conf;
    sock_udp_ep_t server = { .family = AF_INET6, .port = MODBUS_PORT };
    const uint16_t written[] = { 0x1234, 0xbeef, 42 };
    uint16_t read[REGISTERS_NUMOF];
    int failures = 0;

    thread_create(_server_stack, sizeof(_server_stack), THREAD_PRIORITY_MAIN - 1,
                  0, _server, NULL, "modbus server");

    ipv6_addr_set_loopback((ipv6_addr_t *)&server.addr.ipv6);
    if (sock_udp_create(&sock, NULL, NULL, 0) < 0) {
        puts("cannot create the client sock");
        return 1;
    }
    nanomodbus_udp_init(&udp, &sock, &server, &conf);
    nmbs_client_create(&client, &conf);
    nmbs_set_read_timeout(&client, TIMEOUT_MS);
    nmbs_set_byte_timeout(&client, TIMEOUT_MS);

    failures += _check("write 3 registers at 2",
                       nmbs_write_multiple_registers(&client, 2, 3, written),
                       NMBS_ERROR_NONE);
    failures += _check("read 5 registers at 1",
                       nmbs_read_holding_registers(&client, 1, 5, read),
                       NMBS_ERROR_NONE);
    printf("registers 1 to 5: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
           read[0], read[1], read[2], read[3], read[4]);
    if ((read[0] != 0) || (read[1] != written[0]) || (read[2] != written[1])
        || (read[3] != written[2]) || (read[4] != 0)) {
        puts("unexpected register values");
        failures++;
    }
    failures += _check("read 2 registers at 7",
                       nmbs_read_holding_registers(&client, 7, 2, read),
                       NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS);

    puts(failures ? "FAILURE" : "SUCCESS");
    return 0;
}
