/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       zenoh-pico client: session, publication and subscription from
 *              the shell
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "msg.h"
#include "mutex.h"
#include "shell.h"

#include "zenoh-pico.h"

#define MAIN_QUEUE_SIZE     (8U)

static z_owned_session_t _session;
static z_owned_subscriber_t _subscriber;
static bool _opened;
static bool _subscribed;
static mutex_t _lock = MUTEX_INIT;

static void _on_sample(z_loaned_sample_t *sample, void *ctx)
{
    (void)ctx;

    z_view_string_t key;
    z_owned_string_t payload;

    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key);
    z_bytes_to_string(z_sample_payload(sample), &payload);
    printf("zenoh: received '%.*s' on %.*s\n",
           (int)z_string_len(z_loan(payload)), z_string_data(z_loan(payload)),
           (int)z_string_len(z_loan(key)), z_string_data(z_loan(key)));
    z_drop(z_move(payload));
}

static int _open(const char *locator)
{
    z_owned_config_t config;

    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_CLIENT);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, locator);
    if (z_open(&_session, z_move(config), NULL) < 0) {
        puts("zenoh: cannot open the session");
        return -1;
    }
    _opened = true;
    printf("zenoh: connected to %s\n", locator);
    return 0;
}

static void _close(void)
{
    if (_subscribed) {
        z_drop(z_move(_subscriber));
        _subscribed = false;
    }
    z_drop(z_move(_session));
    _opened = false;
    puts("zenoh: closed");
}

static int _put(const char *key, const char *value)
{
    z_view_keyexpr_t ke;
    z_owned_bytes_t payload;

    if (z_view_keyexpr_from_str(&ke, key) < 0) {
        puts("zenoh: invalid key expression");
        return -1;
    }
    z_bytes_copy_from_str(&payload, value);
    if (z_put(z_loan(_session), z_loan(ke), z_move(payload), NULL) < 0) {
        puts("zenoh: put failed");
        return -1;
    }
    printf("zenoh: put '%s' on %s\n", value, key);
    return 0;
}

static int _subscribe(const char *key)
{
    z_view_keyexpr_t ke;
    z_owned_closure_sample_t callback;

    if (_subscribed) {
        puts("zenoh: already subscribed");
        return -1;
    }
    if (z_view_keyexpr_from_str(&ke, key) < 0) {
        puts("zenoh: invalid key expression");
        return -1;
    }
    z_closure(&callback, _on_sample, NULL, NULL);
    if (z_declare_subscriber(z_loan(_session), &_subscriber, z_loan(ke),
                             z_move(callback), NULL) < 0) {
        puts("zenoh: cannot declare the subscriber");
        return -1;
    }
    _subscribed = true;
    printf("zenoh: subscribed to %s\n", key);
    return 0;
}

static void _usage(const char *cmd)
{
    printf("usage: %s open <locator>     e.g. tcp/[2001:db8::1]:7447\n"
           "       %s put <key> <value>\n"
           "       %s sub <key>          e.g. demo/**\n"
           "       %s close\n", cmd, cmd, cmd, cmd);
}

static int _zenoh_cmd(int argc, char **argv)
{
    int res = -1;

    if (argc < 2) {
        _usage(argv[0]);
        return 1;
    }

    mutex_lock(&_lock);
    if ((argc == 3) && !strcmp(argv[1], "open")) {
        if (_opened) {
            puts("zenoh: already open");
        }
        else {
            res = _open(argv[2]);
        }
    }
    else if (!_opened && strcmp(argv[1], "help")) {
        puts("zenoh: no session, zenoh open <locator> first");
    }
    else if ((argc == 4) && !strcmp(argv[1], "put")) {
        res = _put(argv[2], argv[3]);
    }
    else if ((argc == 3) && !strcmp(argv[1], "sub")) {
        res = _subscribe(argv[2]);
    }
    else if ((argc == 2) && !strcmp(argv[1], "close")) {
        _close();
        res = 0;
    }
    else {
        _usage(argv[0]);
    }
    mutex_unlock(&_lock);

    return (res < 0) ? 1 : 0;
}

SHELL_COMMAND(zenoh, "zenoh-pico client", _zenoh_cmd);

int main(void)
{
    /* the shell commands of GNRC (e.g. ping) receive messages in this thread */
    static msg_t main_queue[MAIN_QUEUE_SIZE];
    msg_init_queue(main_queue, MAIN_QUEUE_SIZE);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
