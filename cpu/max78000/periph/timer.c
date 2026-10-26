/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     cpu_max78000
 * @ingroup     drivers_periph_timer
 * @{
 *
 * @file
 * @brief       Low-level timer driver implementation for the MAX78000
 *
 * The timers are used in 32-bit cascade mode and in compare mode: the counter
 * runs freely from 0 to 0xffffffff and wraps around, the interrupt is raised
 * when it matches the compare register. This gives one channel per timer.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <assert.h>

#include "cpu.h"
#include "irq.h"
#include "periph/timer.h"
#include "periph_conf.h"
#include "periph_cpu.h"

/**
 * @brief   Number of prescaler values: PERIPH_CLOCK / 2^n, n from 0 to 12
 */
#define PRESCALER_NUMOF     (13U)

/**
 * @brief   Clock source of TMR0 to TMR3 selecting the peripheral clock
 */
#define CLKSEL_PCLK         (0U)

/**
 * @brief   Interrupt context for each configured timer
 */
static timer_isr_ctx_t _isr_ctx[TIMER_NUMOF];

static inline mxc_tmr_regs_t *dev(tim_t tim)
{
    return timer_config[tim].dev;
}

static void _clock_enable(tim_t tim)
{
    uint32_t mask;

    switch (MXC_TMR_GET_IDX(dev(tim))) {
    case 0:
        mask = MXC_F_GCR_PCLKDIS0_TMR0;
        break;
    case 1:
        mask = MXC_F_GCR_PCLKDIS0_TMR1;
        break;
    case 2:
        mask = MXC_F_GCR_PCLKDIS0_TMR2;
        break;
    default:
        mask = MXC_F_GCR_PCLKDIS0_TMR3;
        break;
    }

    MXC_GCR->pclkdis0 &= ~mask;
}

static void _reset(tim_t tim)
{
    uint32_t mask;

    switch (MXC_TMR_GET_IDX(dev(tim))) {
    case 0:
        mask = MXC_F_GCR_RST0_TMR0;
        break;
    case 1:
        mask = MXC_F_GCR_RST0_TMR1;
        break;
    case 2:
        mask = MXC_F_GCR_RST0_TMR2;
        break;
    default:
        mask = MXC_F_GCR_RST0_TMR3;
        break;
    }

    MXC_GCR->rst0 |= mask;
    while (MXC_GCR->rst0 & mask) {}
}

uword_t timer_query_freqs_numof(tim_t tim)
{
    assert(tim < TIMER_NUMOF);
    (void)tim;

    return PRESCALER_NUMOF;
}

uint32_t timer_query_freqs(tim_t tim, uword_t index)
{
    assert(tim < TIMER_NUMOF);
    (void)tim;

    if (index >= PRESCALER_NUMOF) {
        return 0;
    }

    return PERIPH_CLOCK >> index;
}

int timer_init(tim_t tim, uint32_t freq, timer_cb_t cb, void *arg)
{
    assert(tim < TIMER_NUMOF);

    int idx = MXC_TMR_GET_IDX(dev(tim));
    if ((idx < 0) || (idx > 3)) {
        /* the low power timers TMR4 and TMR5 are not supported */
        return -1;
    }

    uint32_t prescaler = 0;
    while ((prescaler < PRESCALER_NUMOF) && ((PERIPH_CLOCK >> prescaler) != freq)) {
        prescaler++;
    }
    if (prescaler == PRESCALER_NUMOF) {
        return -1;
    }

    _isr_ctx[tim].cb = cb;
    _isr_ctx[tim].arg = arg;

    _clock_enable(tim);
    _reset(tim);

    /* 32-bit cascade mode, both halves clocked from the peripheral clock with
     * the same prescaler */
    dev(tim)->ctrl1 = MXC_F_TMR_CTRL1_CASCADE
                      | (CLKSEL_PCLK << MXC_F_TMR_CTRL1_CLKSEL_A_POS)
                      | (CLKSEL_PCLK << MXC_F_TMR_CTRL1_CLKSEL_B_POS);
    dev(tim)->ctrl0 = MXC_S_TMR_CTRL0_MODE_A_COMPARE
                      | (prescaler << MXC_F_TMR_CTRL0_CLKDIV_A_POS)
                      | (prescaler << MXC_F_TMR_CTRL0_CLKDIV_B_POS);

    dev(tim)->ctrl0 |= MXC_F_TMR_CTRL0_CLKEN_A;
    while (!(dev(tim)->ctrl1 & MXC_F_TMR_CTRL1_CLKRDY_A)) {}

    dev(tim)->cmp = UINT32_MAX;
    dev(tim)->cnt = 0;
    while (!(dev(tim)->intfl & MXC_F_TMR_INTFL_WRDONE_A)) {}
    dev(tim)->intfl = MXC_F_TMR_INTFL_IRQ_A;

    NVIC_EnableIRQ(timer_config[tim].irqn);

    timer_start(tim);

    return 0;
}

int timer_set_absolute(tim_t tim, int channel, unsigned int value)
{
    assert(tim < TIMER_NUMOF);

    if (channel != 0) {
        return -1;
    }

    unsigned state = irq_disable();
    dev(tim)->cmp = value;
    dev(tim)->intfl = MXC_F_TMR_INTFL_IRQ_A;
    dev(tim)->ctrl1 |= MXC_F_TMR_CTRL1_IE_A;
    irq_restore(state);

    return 0;
}

int timer_clear(tim_t tim, int channel)
{
    assert(tim < TIMER_NUMOF);

    if (channel != 0) {
        return -1;
    }

    dev(tim)->ctrl1 &= ~MXC_F_TMR_CTRL1_IE_A;

    return 0;
}

unsigned int timer_read(tim_t tim)
{
    assert(tim < TIMER_NUMOF);

    return dev(tim)->cnt;
}

void timer_start(tim_t tim)
{
    assert(tim < TIMER_NUMOF);

    dev(tim)->ctrl0 |= MXC_F_TMR_CTRL0_EN_A;
    while (!(dev(tim)->ctrl1 & MXC_F_TMR_CTRL1_CLKEN_A)) {}
}

void timer_stop(tim_t tim)
{
    assert(tim < TIMER_NUMOF);

    dev(tim)->ctrl0 &= ~MXC_F_TMR_CTRL0_EN_A;
}

static inline void _irq_handler(tim_t tim)
{
    if (dev(tim)->intfl & MXC_F_TMR_INTFL_IRQ_A) {
        dev(tim)->intfl = MXC_F_TMR_INTFL_IRQ_A;
        if (dev(tim)->ctrl1 & MXC_F_TMR_CTRL1_IE_A) {
            /* one shot: the channel is cleared before calling back */
            dev(tim)->ctrl1 &= ~MXC_F_TMR_CTRL1_IE_A;
            _isr_ctx[tim].cb(_isr_ctx[tim].arg, 0);
        }
    }

    cortexm_isr_end();
}

#ifdef TIMER_0_ISR
void TIMER_0_ISR(void)
{
    _irq_handler(TIMER_DEV(0));
}
#endif

#ifdef TIMER_1_ISR
void TIMER_1_ISR(void)
{
    _irq_handler(TIMER_DEV(1));
}
#endif

#ifdef TIMER_2_ISR
void TIMER_2_ISR(void)
{
    _irq_handler(TIMER_DEV(2));
}
#endif

#ifdef TIMER_3_ISR
void TIMER_3_ISR(void)
{
    _irq_handler(TIMER_DEV(3));
}
#endif
