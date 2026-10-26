/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     cpu_max78000
 * @ingroup     drivers_periph_uart
 * @{
 *
 * @file
 * @brief       Low-level UART driver implementation for the MAX78000
 *
 * The UARTs are clocked from the internal baud rate oscillator (IBRO,
 * 7.3728 MHz): the common baud rates are exact and independent of the system
 * clock. Transmission is blocking, reception is interrupt driven (one
 * interrupt per received byte).
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <assert.h>

#include "cpu.h"
#include "periph/gpio.h"
#include "periph/uart.h"
#include "periph_conf.h"
#include "periph_cpu.h"

/**
 * @brief   Frequency of the internal baud rate oscillator
 */
#define UART_CLOCK          (IBRO_FREQ)

/**
 * @brief   Over-sampling rate, value used by the vendor SDK
 */
#define UART_OSR            (5U)

/**
 * @brief   Allocate memory to store the callback functions
 */
static uart_isr_ctx_t _isr_ctx[UART_NUMOF];

static inline mxc_uart_regs_t *dev(uart_t uart)
{
    return uart_config[uart].dev;
}

static void _clock_enable(uart_t uart, bool enable)
{
    volatile uint32_t *reg;
    uint32_t mask;

    switch (MXC_UART_GET_IDX(dev(uart))) {
    case 0:
        reg = &MXC_GCR->pclkdis0;
        mask = MXC_F_GCR_PCLKDIS0_UART0;
        break;
    case 1:
        reg = &MXC_GCR->pclkdis0;
        mask = MXC_F_GCR_PCLKDIS0_UART1;
        break;
    default:
        reg = &MXC_GCR->pclkdis1;
        mask = MXC_F_GCR_PCLKDIS1_UART2;
        break;
    }

    if (enable) {
        *reg &= ~mask;
    }
    else {
        *reg |= mask;
    }
}

static void _reset(uart_t uart)
{
    uint32_t mask;

    switch (MXC_UART_GET_IDX(dev(uart))) {
    case 0:
        mask = MXC_F_GCR_RST0_UART0;
        break;
    case 1:
        mask = MXC_F_GCR_RST0_UART1;
        break;
    default:
        mask = MXC_F_GCR_RST0_UART2;
        break;
    }

    MXC_GCR->rst0 |= mask;
    while (MXC_GCR->rst0 & mask) {}
}

int uart_init(uart_t uart, uint32_t baud, uart_rx_cb_t rx_cb, void *arg)
{
    assert(uart < UART_NUMOF);

    int idx = MXC_UART_GET_IDX(dev(uart));
    if ((idx < 0) || (idx > 2)) {
        /* UART3 (low power UART) is not supported */
        return UART_NODEV;
    }

    /* the clock divider is rounded to the nearest value, as the vendor SDK
     * does */
    uint32_t clkdiv = UART_CLOCK / baud;
    if ((clkdiv == 0) || ((UART_CLOCK % baud) > (baud / 2))) {
        clkdiv++;
    }
    if (clkdiv > MXC_F_UART_CLKDIV_CLKDIV) {
        return UART_NOBAUD;
    }

    _isr_ctx[uart].rx_cb = rx_cb;
    _isr_ctx[uart].arg = arg;

    uart_poweron(uart);
    _reset(uart);

    gpio_init_af(uart_config[uart].tx_pin, uart_config[uart].af);
    if (rx_cb) {
        gpio_init_af(uart_config[uart].rx_pin, uart_config[uart].af);
    }

    /* 8N1, an RX interrupt per received byte, baud clock from the IBRO */
    dev(uart)->ctrl = MXC_S_UART_CTRL_CHAR_SIZE_8BITS
                      | (1 << MXC_F_UART_CTRL_RX_THD_VAL_POS)
                      | MXC_S_UART_CTRL_BCLKSRC_CLK2;
    dev(uart)->ctrl |= MXC_F_UART_CTRL_TX_FLUSH | MXC_F_UART_CTRL_RX_FLUSH;

    dev(uart)->osr = UART_OSR;
    dev(uart)->clkdiv = clkdiv;
    dev(uart)->ctrl |= MXC_F_UART_CTRL_BCLKEN;
    while (!(dev(uart)->ctrl & MXC_F_UART_CTRL_BCLKRDY)) {}

    dev(uart)->int_fl = dev(uart)->int_fl;
    if (rx_cb) {
        dev(uart)->int_en = MXC_F_UART_INT_EN_RX_THD;
        NVIC_EnableIRQ(uart_config[uart].irqn);
    }
    else {
        dev(uart)->int_en = 0;
    }

    return UART_OK;
}

#ifdef MODULE_PERIPH_UART_MODECFG
int uart_mode(uart_t uart, uart_data_bits_t data_bits, uart_parity_t parity,
              uart_stop_bits_t stop_bits)
{
    assert(uart < UART_NUMOF);

    uint32_t ctrl = dev(uart)->ctrl & ~(MXC_F_UART_CTRL_CHAR_SIZE |
                                        MXC_F_UART_CTRL_PAR_EN |
                                        MXC_F_UART_CTRL_PAR_EO |
                                        MXC_F_UART_CTRL_PAR_MD |
                                        MXC_F_UART_CTRL_STOPBITS);

    ctrl |= ((uint32_t)data_bits << MXC_F_UART_CTRL_CHAR_SIZE_POS);

    switch (parity) {
    case UART_PARITY_NONE:
        break;
    case UART_PARITY_EVEN:
        ctrl |= MXC_F_UART_CTRL_PAR_EN;
        break;
    case UART_PARITY_ODD:
        ctrl |= MXC_F_UART_CTRL_PAR_EN | MXC_F_UART_CTRL_PAR_EO;
        break;
    default:
        return UART_NOMODE;
    }

    if (stop_bits == UART_STOP_BITS_2) {
        ctrl |= MXC_F_UART_CTRL_STOPBITS;
    }

    dev(uart)->ctrl = ctrl;

    return UART_OK;
}
#endif /* MODULE_PERIPH_UART_MODECFG */

void uart_write(uart_t uart, const uint8_t *data, size_t len)
{
    assert(uart < UART_NUMOF);

    for (size_t i = 0; i < len; i++) {
        while (dev(uart)->status & MXC_F_UART_STATUS_TX_FULL) {}
        dev(uart)->fifo = data[i];
    }

    /* wait for the end of the transmission */
    while (dev(uart)->status & MXC_F_UART_STATUS_TX_BUSY) {}
}

void uart_poweron(uart_t uart)
{
    assert(uart < UART_NUMOF);

    _clock_enable(uart, true);
}

void uart_poweroff(uart_t uart)
{
    assert(uart < UART_NUMOF);

    _clock_enable(uart, false);
}

static inline void _irq_handler(uart_t uart)
{
    uint32_t flags = dev(uart)->int_fl;

    dev(uart)->int_fl = flags;
    while (!(dev(uart)->status & MXC_F_UART_STATUS_RX_EM)) {
        uint8_t data = dev(uart)->fifo & MXC_F_UART_FIFO_DATA;
        _isr_ctx[uart].rx_cb(_isr_ctx[uart].arg, data);
    }

    cortexm_isr_end();
}

#ifdef UART_0_ISR
void UART_0_ISR(void)
{
    _irq_handler(UART_DEV(0));
}
#endif

#ifdef UART_1_ISR
void UART_1_ISR(void)
{
    _irq_handler(UART_DEV(1));
}
#endif

#ifdef UART_2_ISR
void UART_2_ISR(void)
{
    _irq_handler(UART_DEV(2));
}
#endif
