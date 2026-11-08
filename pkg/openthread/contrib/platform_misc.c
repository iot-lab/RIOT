/*
 * SPDX-FileCopyrightText: 2017-2021 Baptiste Clenet
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       Implementation of OpenThread misc platform abstraction
 *
 * @author      Baptiste Clenet <bapclenet@gmail.com>
 * @}
 */

#include <stdio.h>

#include "openthread/platform/misc.h"
#include "panic.h"
#include "periph/pm.h"

#define ENABLE_DEBUG 0
#include "debug.h"

void otPlatReset(otInstance *aInstance)
{
    (void)aInstance;
    DEBUG("reboot...\n");
    pm_reboot();
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    (void)aInstance;
    return OT_PLAT_RESET_REASON_POWER_ON;
}

void otPlatAssertFail(const char *aFilename, int aLineNumber)
{
    printf("openthread: assertion failed in %s:%d\n", aFilename, aLineNumber);
    core_panic(PANIC_ASSERT_FAIL, "OpenThread assertion");
}

void otPlatWakeHost(void)
{
}
