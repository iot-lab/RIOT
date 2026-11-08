/*
 * SPDX-FileCopyrightText: 2025 TU Dresden
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     sys_shell_commands
 * @{
 *
 * @file
 * @brief       Shell command implementation for OpenThread
 *
 * The command line is run by the OpenThread CLI in the OpenThread thread:
 * the shell waits until it's done.
 *
 * @author      Moritz Voigt <moritz.voigt@mailbox.tu-dresden.de>
 *
 * @}
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "event.h"
#include "mutex.h"
#include "shell.h"
#include "openthread/cli.h"
#include "ot.h"

static char *_line;
static mutex_t _done = MUTEX_INIT_LOCKED;

static int ot_console_cb(void *context, const char *format, va_list args)
{
    (void)context;
    return vprintf(format, args);
}

static void _input_handler(event_t *event)
{
    (void)event;
    otCliInputLine(_line);
    mutex_unlock(&_done);
}

static event_t _ev_input = { .handler = _input_handler };

static int ot_cmd(int argc, char **argv)
{
    /* Join all arguments after "ot" into a single space-separated string */
    for (int i = 1; i < argc - 1; i++) {
        char *arg = argv[i];
        arg += strlen(arg);
        *arg = ' ';
    }

    static char help[] = "help";
    _line = (argc > 1) && (strlen(argv[1]) != 0) ? argv[1] : help;
    event_post(openthread_get_evq(), &_ev_input);
    mutex_lock(&_done);
    return 0;
}

SHELL_COMMAND(ot, "Use commands from OpenThread CLI", ot_cmd);

void ot_shell_init(otInstance *aInstance)
{
    otCliInit(aInstance, ot_console_cb, NULL);
}
