/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     boards_max78000fthr
 * @{
 *
 * @file
 * @brief       Board initialization for the MAX78000FTHR
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include "board.h"
#include "periph/gpio.h"

void board_init(void)
{
    /* the RGB LED and the push buttons are supplied from VDDIOH (3.3 V): with
     * the default VDDIO supply (1.8 V), an LED switched off would still be
     * lit faintly */
    gpio_set_supply(LED0_PIN, GPIO_SUPPLY_VDDIOH);
    gpio_set_supply(LED1_PIN, GPIO_SUPPLY_VDDIOH);
    gpio_set_supply(LED2_PIN, GPIO_SUPPLY_VDDIOH);
    gpio_set_supply(BTN0_PIN, GPIO_SUPPLY_VDDIOH);
    gpio_set_supply(BTN1_PIN, GPIO_SUPPLY_VDDIOH);
}
