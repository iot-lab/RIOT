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
 * @brief       Board configuration for the MAX78000FTHR board
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#include "cpu.h"
#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    ztimer configuration
 *
 * The timer prescaler only divides the 50 MHz peripheral clock by powers of
 * 2: the closest frequency above 1 MHz is used (50 MHz / 32).
 * @{
 */
#define CONFIG_ZTIMER_USEC_BASE_FREQ    (PERIPH_CLOCK / 32) /**< 1.5625 MHz */
/** @} */

/**
 * @name    LED pin configuration
 *
 * The RGB LED is active LOW.
 * @{
 */
#define LED0_PIN            GPIO_PIN(2, 0)  /**< Red LED */
#define LED1_PIN            GPIO_PIN(2, 1)  /**< Green LED */
#define LED2_PIN            GPIO_PIN(2, 2)  /**< Blue LED */

#define LED0_MASK           (1 << 0)        /**< LED0 PORT bitmask */
#define LED1_MASK           (1 << 1)        /**< LED1 PORT bitmask */
#define LED2_MASK           (1 << 2)        /**< LED2 PORT bitmask */

#define LED0_IS_INVERTED    1               /**< LED0 is active LOW */
#define LED1_IS_INVERTED    1               /**< LED1 is active LOW */
#define LED2_IS_INVERTED    1               /**< LED2 is active LOW */

#define LED0_ON             (MXC_GPIO2->out_clr = LED0_MASK) /**< LED0 ON macro */
#define LED0_OFF            (MXC_GPIO2->out_set = LED0_MASK) /**< LED0 OFF macro */
#define LED0_TOGGLE         (MXC_GPIO2->out    ^= LED0_MASK) /**< LED0 toggle macro */

#define LED1_ON             (MXC_GPIO2->out_clr = LED1_MASK) /**< LED1 ON macro */
#define LED1_OFF            (MXC_GPIO2->out_set = LED1_MASK) /**< LED1 OFF macro */
#define LED1_TOGGLE         (MXC_GPIO2->out    ^= LED1_MASK) /**< LED1 toggle macro */

#define LED2_ON             (MXC_GPIO2->out_clr = LED2_MASK) /**< LED2 ON macro */
#define LED2_OFF            (MXC_GPIO2->out_set = LED2_MASK) /**< LED2 OFF macro */
#define LED2_TOGGLE         (MXC_GPIO2->out    ^= LED2_MASK) /**< LED2 toggle macro */
/** @} */

/**
 * @name    Button pin configuration
 *
 * The buttons are active LOW.
 * @{
 */
#define BTN0_PIN            GPIO_PIN(0, 2)  /**< SW1 pin definition */
#define BTN0_MODE           GPIO_IN_PU      /**< SW1 default mode */
#define BTN1_PIN            GPIO_PIN(1, 7)  /**< SW2 pin definition */
#define BTN1_MODE           GPIO_IN_PU      /**< SW2 default mode */
/** @} */

#ifdef __cplusplus
}
#endif

/** @} */
