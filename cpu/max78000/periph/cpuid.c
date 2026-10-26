/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     cpu_max78000
 * @ingroup     drivers_periph_cpuid
 * @{
 *
 * @file
 * @brief       Low-level CPUID driver implementation for the MAX78000
 *
 * The CPU ID is the unique serial number (USN) written in the information
 * block of the flash at production. The information block is read protected:
 * it is unlocked through the flash controller for the time of the read.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <stdint.h>

#include "cpu.h"
#include "irq.h"
#include "periph/cpuid.h"
#include "periph_cpu.h"

/**
 * @name    Information block access keys of the flash controller
 * @{
 */
#define FLC_ACTRL_LOCK      (0xdeadbeef)
#define FLC_ACTRL_RESET     (0x00001234)
#define FLC_ACTRL_KEY1      (0x3a7f5ca3)
#define FLC_ACTRL_KEY2      (0xa1e34f20)
#define FLC_ACTRL_KEY3      (0x9608b2c1)
/** @} */

void cpuid_get(void *id)
{
    const volatile uint32_t *info = (const volatile uint32_t *)MXC_INFO_MEM_BASE;
    uint32_t word[4];
    uint8_t *usn = id;

    unsigned state = irq_disable();

    MXC_FLC->actrl = FLC_ACTRL_RESET;
    MXC_FLC->actrl = FLC_ACTRL_KEY1;
    MXC_FLC->actrl = FLC_ACTRL_KEY2;
    MXC_FLC->actrl = FLC_ACTRL_KEY3;
    for (unsigned i = 0; i < 4; i++) {
        word[i] = info[i];
    }
    MXC_FLC->actrl = FLC_ACTRL_LOCK;
    irq_restore(state);

    /* the USN bytes are spread over the information block words, as decoded
     * by the vendor SDK (MXC_SYS_GetUSN) */
    usn[0] = (word[0] & 0x007f8000) >> 15;
    usn[1] = (word[0] & 0x7f800000) >> 23;
    usn[2] = ((word[1] & 0x0000007f) << 1) | ((word[0] & 0x80000000) >> 31);
    usn[3] = (word[1] & 0x00007f80) >> 7;
    usn[4] = (word[1] & 0x007f8000) >> 15;
    usn[5] = (word[1] & 0x7f800000) >> 23;
    usn[6] = (word[2] & 0x007f8000) >> 15;
    usn[7] = (word[2] & 0x7f800000) >> 23;
    usn[8] = ((word[3] & 0x0000007f) << 1) | ((word[2] & 0x80000000) >> 31);
    usn[9] = (word[3] & 0x00007f80) >> 7;
    usn[10] = (word[3] & 0x007f8000) >> 15;
}
