/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup         cpu_max78000
 * @{
 *
 * @file
 * @brief           MAX78000 specific definitions for handling peripherals
 *
 * @author          Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#include <stdint.h>

#include "cpu.h"
#include "macros/units.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   System core clock: the internal primary oscillator (IPO), selected
 *          by cpu_init()
 */
#define CLOCK_CORECLOCK     MHZ(100)

/**
 * @brief   Peripheral clock (PCLK), half of the system core clock
 */
#define PERIPH_CLOCK        (CLOCK_CORECLOCK / 2)

/**
 * @brief   Length of the CPU_ID in bytes: the unique serial number (USN) of
 *          the information block
 */
#define CPUID_LEN           (11U)

/**
 * @name    GPIO configuration
 *
 * Ports 0, 1 and 2 are supported. Port 3 (two pins of the always-on power
 * domain, controlled through the MCR registers) is not.
 * @{
 */
#define HAVE_GPIO_T
typedef uint32_t gpio_t;                    /**< GPIO type identifier */

#define GPIO_UNDEF          (0xffffffff)    /**< Undefined GPIO pin */

/**
 * @brief   Define a GPIO pin from a port and a pin number
 */
#define GPIO_PIN(x, y)      (((x) << 5) | (y))

/**
 * @brief   Number of supported GPIO ports
 */
#define GPIO_PORT_NUMOF     (3U)

/**
 * @brief   Alternate functions of a GPIO pin
 */
typedef enum {
    GPIO_AF1 = 0,           /**< Alternate function 1 */
    GPIO_AF2,               /**< Alternate function 2 */
    GPIO_AF3,               /**< Alternate function 3 */
    GPIO_AF4,               /**< Alternate function 4 */
} gpio_af_t;

/**
 * @brief   I/O supply of a GPIO pin
 */
typedef enum {
    GPIO_SUPPLY_VDDIO = 0,  /**< VDDIO supply (reset value) */
    GPIO_SUPPLY_VDDIOH,     /**< VDDIOH supply */
} gpio_supply_t;

/**
 * @brief   Configure a pin for an alternate function
 *
 * @param[in] pin       pin to configure
 * @param[in] af        alternate function to use
 */
void gpio_init_af(gpio_t pin, gpio_af_t af);

/**
 * @brief   Select the I/O supply of a pin
 *
 * @param[in] pin       pin to configure
 * @param[in] supply    supply to use
 */
void gpio_set_supply(gpio_t pin, gpio_supply_t supply);
/** @} */

/**
 * @name    UART configuration
 * @{
 */
/**
 * @brief   UART configuration
 *
 * UART0 to UART2 are supported, clocked from the internal baud rate
 * oscillator (IBRO, 7.3728 MHz). The low power UART (UART3) is not supported.
 */
typedef struct {
    mxc_uart_regs_t *dev;   /**< UART device */
    gpio_t rx_pin;          /**< RX pin */
    gpio_t tx_pin;          /**< TX pin */
    gpio_af_t af;           /**< Alternate function of the RX and TX pins */
    IRQn_Type irqn;         /**< IRQ number */
} uart_conf_t;

#ifndef DOXYGEN
#define HAVE_UART_PARITY_T
typedef enum {
    UART_PARITY_NONE,       /**< no parity */
    UART_PARITY_EVEN,       /**< even parity */
    UART_PARITY_ODD,        /**< odd parity */
    UART_PARITY_MARK,       /**< mark parity (not supported) */
    UART_PARITY_SPACE,      /**< space parity (not supported) */
} uart_parity_t;

#define HAVE_UART_DATA_BITS_T
typedef enum {
    UART_DATA_BITS_5 = 0,   /**< 5 data bits */
    UART_DATA_BITS_6,       /**< 6 data bits */
    UART_DATA_BITS_7,       /**< 7 data bits */
    UART_DATA_BITS_8,       /**< 8 data bits */
} uart_data_bits_t;

#define HAVE_UART_STOP_BITS_T
typedef enum {
    UART_STOP_BITS_1 = 0,   /**< 1 stop bit */
    UART_STOP_BITS_2,       /**< 2 stop bits (1.5 with 5 data bits) */
} uart_stop_bits_t;
#endif /* ndef DOXYGEN */
/** @} */

/**
 * @name    Timer configuration
 * @{
 */
/**
 * @brief   Timer configuration
 *
 * TMR0 to TMR3 are supported, in 32-bit cascade mode, clocked from the
 * peripheral clock with a power of 2 prescaler (PERIPH_CLOCK / 2^n, n from 0
 * to 12). Each timer provides one channel. The low power timers (TMR4 and
 * TMR5) are not supported.
 */
typedef struct {
    mxc_tmr_regs_t *dev;    /**< Timer device */
    IRQn_Type irqn;         /**< IRQ number */
} timer_conf_t;

/**
 * @brief   Number of channels of a timer
 */
#define TIMER_CHANNEL_NUMOF (1U)
/** @} */

#ifdef __cplusplus
}
#endif

/** @} */
