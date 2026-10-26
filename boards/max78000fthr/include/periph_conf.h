/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup     boards_max78000fthr
 * @{
 *
 * @file
 * @brief       Peripheral configuration for the MAX78000FTHR
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name   Timer configuration
 * @{
 */
static const timer_conf_t timer_config[] = {
    {
        .dev  = MXC_TMR0,
        .irqn = TMR0_IRQn,
    },
    {
        .dev  = MXC_TMR1,
        .irqn = TMR1_IRQn,
    },
};

#define TIMER_0_ISR         isr_tmr0 /**< Timer0 IRQ */
#define TIMER_1_ISR         isr_tmr1 /**< Timer1 IRQ */

#define TIMER_NUMOF         ARRAY_SIZE(timer_config) /**< Timer configuration NUMOF */
/** @} */

/**
 * @name    UART configuration
 *
 * UART0 is connected to the virtual COM port of the on-board DAPLink and is
 * used for stdio.
 * @{
 */
static const uart_conf_t uart_config[] = {
    {
        .dev    = MXC_UART0,
        .rx_pin = GPIO_PIN(0, 0),
        .tx_pin = GPIO_PIN(0, 1),
        .af     = GPIO_AF1,
        .irqn   = UART0_IRQn,
    },
};

#define UART_0_ISR          isr_uart0 /**< UART0 IRQ */

#define UART_NUMOF          ARRAY_SIZE(uart_config) /**< UART configuration NUMOF */
/** @} */

#ifdef __cplusplus
}
#endif

/** @} */
