/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     cpu_max78000
 * @ingroup     drivers_periph_gpio
 * @{
 *
 * @file
 * @brief       Low-level GPIO driver implementation for the MAX78000
 *
 * Ports 0 and 1 are in the main power domain, port 2 in the low power domain
 * (clock gated by the LPGCR). Each port has its own interrupt line, with an
 * edge detection per pin.
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <assert.h>

#include "cpu.h"
#include "irq.h"
#include "periph/gpio.h"
#include "periph_cpu.h"
#include "periph_conf.h"

#define PORT_POS            (5U)
#define PIN_MASK            (0x1f)

/**
 * @brief   GPIO port base addresses, indexed by port number
 */
static mxc_gpio_regs_t *const _port[GPIO_PORT_NUMOF] = {
    MXC_GPIO0,
    MXC_GPIO1,
    MXC_GPIO2,
};

/**
 * @brief   Get the port's base address
 */
static inline mxc_gpio_regs_t *port(gpio_t pin)
{
    return _port[pin >> PORT_POS];
}

/**
 * @brief   Get the port number of a pin
 */
static inline unsigned port_num(gpio_t pin)
{
    return pin >> PORT_POS;
}

/**
 * @brief   Get the bit mask of a pin in its port
 */
static inline uint32_t pin_mask(gpio_t pin)
{
    return (1UL << (pin & PIN_MASK));
}

static void _port_clock_enable(unsigned num)
{
    switch (num) {
    case 0:
        MXC_GCR->pclkdis0 &= ~MXC_F_GCR_PCLKDIS0_GPIO0;
        break;
    case 1:
        MXC_GCR->pclkdis0 &= ~MXC_F_GCR_PCLKDIS0_GPIO1;
        break;
    default:
        MXC_LPGCR->pclkdis &= ~MXC_F_LPGCR_PCLKDIS_GPIO2;
        break;
    }
}

int gpio_init(gpio_t pin, gpio_mode_t mode)
{
    if (port_num(pin) >= GPIO_PORT_NUMOF) {
        return -1;
    }

    switch (mode) {
    case GPIO_IN:
    case GPIO_IN_PD:
    case GPIO_IN_PU:
    case GPIO_OUT:
        break;
    default:
        /* open drain is not supported */
        return -1;
    }

    _port_clock_enable(port_num(pin));

    mxc_gpio_regs_t *dev = port(pin);
    uint32_t mask = pin_mask(pin);

    /* GPIO function, input buffer always enabled so the pin can be read back */
    dev->inen |= mask;
    dev->en0_set = mask;
    dev->en1_clr = mask;
    dev->en2_clr = mask;

    if (mode == GPIO_OUT) {
        dev->outen_set = mask;
    }
    else {
        dev->outen_clr = mask;
    }

    switch (mode) {
    case GPIO_IN_PU:
        dev->padctrl0 |= mask;
        dev->padctrl1 &= ~mask;
        dev->ps &= ~mask;
        break;
    case GPIO_IN_PD:
        dev->padctrl0 &= ~mask;
        dev->padctrl1 |= mask;
        dev->ps &= ~mask;
        break;
    default:
        dev->padctrl0 &= ~mask;
        dev->padctrl1 &= ~mask;
        break;
    }

    return 0;
}

void gpio_init_af(gpio_t pin, gpio_af_t af)
{
    assert(port_num(pin) < GPIO_PORT_NUMOF);

    _port_clock_enable(port_num(pin));

    mxc_gpio_regs_t *dev = port(pin);
    uint32_t mask = pin_mask(pin);

    /* the function is selected by the en2:en1 bits with en0 cleared: switch
     * to GPIO function first to avoid a glitch on another function */
    dev->inen |= mask;
    dev->en0_set = mask;
    if (af & 0x1) {
        dev->en1_set = mask;
    }
    else {
        dev->en1_clr = mask;
    }
    if (af & 0x2) {
        dev->en2_set = mask;
    }
    else {
        dev->en2_clr = mask;
    }
    dev->en0_clr = mask;
}

void gpio_set_supply(gpio_t pin, gpio_supply_t supply)
{
    assert(port_num(pin) < GPIO_PORT_NUMOF);

    _port_clock_enable(port_num(pin));

    if (supply == GPIO_SUPPLY_VDDIOH) {
        port(pin)->vssel |= pin_mask(pin);
    }
    else {
        port(pin)->vssel &= ~pin_mask(pin);
    }
}

bool gpio_read(gpio_t pin)
{
    return port(pin)->in & pin_mask(pin);
}

void gpio_set(gpio_t pin)
{
    port(pin)->out_set = pin_mask(pin);
}

void gpio_clear(gpio_t pin)
{
    port(pin)->out_clr = pin_mask(pin);
}

void gpio_toggle(gpio_t pin)
{
    unsigned state = irq_disable();

    port(pin)->out ^= pin_mask(pin);
    irq_restore(state);
}

void gpio_write(gpio_t pin, bool value)
{
    if (value) {
        gpio_set(pin);
    }
    else {
        gpio_clear(pin);
    }
}

#ifdef MODULE_PERIPH_GPIO_IRQ

/**
 * @brief   Interrupt context of each pin
 */
static gpio_isr_ctx_t _isr_ctx[GPIO_PORT_NUMOF][32];

/**
 * @brief   Interrupt line of each port
 */
static const IRQn_Type _port_irqn[GPIO_PORT_NUMOF] = {
    GPIO0_IRQn,
    GPIO1_IRQn,
    GPIO2_IRQn,
};

int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg)
{
    if (gpio_init(pin, mode) != 0) {
        return -1;
    }

    mxc_gpio_regs_t *dev = port(pin);
    uint32_t mask = pin_mask(pin);

    _isr_ctx[port_num(pin)][pin & PIN_MASK].cb = cb;
    _isr_ctx[port_num(pin)][pin & PIN_MASK].arg = arg;

    /* edge triggered, polarity selected by intpol (1: rising) */
    dev->inten_clr = mask;
    dev->intmode |= mask;
    switch (flank) {
    case GPIO_FALLING:
        dev->intpol &= ~mask;
        dev->dualedge &= ~mask;
        break;
    case GPIO_RISING:
        dev->intpol |= mask;
        dev->dualedge &= ~mask;
        break;
    case GPIO_BOTH:
        dev->dualedge |= mask;
        break;
    default:
        return -1;
    }

    dev->intfl_clr = mask;
    dev->inten_set = mask;
    NVIC_EnableIRQ(_port_irqn[port_num(pin)]);

    return 0;
}

void gpio_irq_enable(gpio_t pin)
{
    port(pin)->intfl_clr = pin_mask(pin);
    port(pin)->inten_set = pin_mask(pin);
}

void gpio_irq_disable(gpio_t pin)
{
    port(pin)->inten_clr = pin_mask(pin);
}

static void _isr_port(unsigned num)
{
    mxc_gpio_regs_t *dev = _port[num];
    uint32_t flags = dev->intfl & dev->inten;

    dev->intfl_clr = flags;
    while (flags) {
        unsigned pin = 31 - __builtin_clz(flags);
        flags &= ~(1UL << pin);
        _isr_ctx[num][pin].cb(_isr_ctx[num][pin].arg);
    }

    cortexm_isr_end();
}

void isr_gpio0(void)
{
    _isr_port(0);
}

void isr_gpio1(void)
{
    _isr_port(1);
}

void isr_gpio2(void)
{
    _isr_port(2);
}

#endif /* MODULE_PERIPH_GPIO_IRQ */
