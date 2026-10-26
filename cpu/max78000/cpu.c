/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     cpu_max78000
 * @{
 *
 * @file
 * @brief       Implementation of the CPU initialization
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include "cpu.h"
#include "kernel_init.h"
#include "periph/init.h"
#include "periph_cpu.h"
#include "stdio_base.h"

static void _icc_enable(void)
{
    /* invalidate the instruction cache of the Cortex-M4, then enable it */
    MXC_ICC0->ctrl &= ~MXC_F_ICC_CTRL_EN;
    MXC_ICC0->invalidate = 1;
    while (!(MXC_ICC0->ctrl & MXC_F_ICC_CTRL_RDY)) {}
    MXC_ICC0->ctrl |= MXC_F_ICC_CTRL_EN;
    while (!(MXC_ICC0->ctrl & MXC_F_ICC_CTRL_RDY)) {}
}

static void _clock_init(void)
{
    /* the IBRO clocks the UARTs */
    MXC_GCR->clkctrl |= MXC_F_GCR_CLKCTRL_IBRO_EN;
    while (!(MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_IBRO_RDY)) {}

    /* switch the system clock from the ISO (60 MHz, reset value) to the
     * IPO (100 MHz), without divider */
    MXC_GCR->clkctrl |= MXC_F_GCR_CLKCTRL_IPO_EN;
    while (!(MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_IPO_RDY)) {}
    MXC_GCR->clkctrl = (MXC_GCR->clkctrl & ~(MXC_F_GCR_CLKCTRL_SYSCLK_SEL |
                                             MXC_F_GCR_CLKCTRL_SYSCLK_DIV))
                       | MXC_S_GCR_CLKCTRL_SYSCLK_SEL_IPO;
    while (!(MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_SYSCLK_RDY)) {}
}

/**
 * @brief   Initialize the CPU
 *
 * Only the Cortex-M4 is used: the RISC-V core and the CNN accelerator stay
 * in their reset state (off).
 */
void cpu_init(void)
{
    /* call cortexm default initialization */
    cortexm_init();

    _icc_enable();
    _clock_init();

    /* initialize stdio prior to periph_init() to allow use of DEBUG() there */
    early_init();

    /* trigger static peripheral initialization */
    periph_init();
}
