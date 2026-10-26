/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     cpu_max78000
 * @{
 *
 * @file
 * @brief       MAX78000 interrupt vector definitions
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <stdint.h>

#include "cpu.h"
#include "vectors_cortexm.h"

/* define a local dummy handler as it needs to be in the same compilation unit
 * as the alias definition */
void dummy_handler(void)
{
    dummy_handler_default();
}

/* MAX78000 specific interrupt vectors */
WEAK_DEFAULT void isr_pf(void);
WEAK_DEFAULT void isr_wdt0(void);
WEAK_DEFAULT void isr_rtc(void);
WEAK_DEFAULT void isr_trng(void);
WEAK_DEFAULT void isr_tmr0(void);
WEAK_DEFAULT void isr_tmr1(void);
WEAK_DEFAULT void isr_tmr2(void);
WEAK_DEFAULT void isr_tmr3(void);
WEAK_DEFAULT void isr_tmr4(void);
WEAK_DEFAULT void isr_tmr5(void);
WEAK_DEFAULT void isr_i2c0(void);
WEAK_DEFAULT void isr_uart0(void);
WEAK_DEFAULT void isr_uart1(void);
WEAK_DEFAULT void isr_spi1(void);
WEAK_DEFAULT void isr_adc(void);
WEAK_DEFAULT void isr_flc0(void);
WEAK_DEFAULT void isr_gpio0(void);
WEAK_DEFAULT void isr_gpio1(void);
WEAK_DEFAULT void isr_gpio2(void);
WEAK_DEFAULT void isr_dma0(void);
WEAK_DEFAULT void isr_dma1(void);
WEAK_DEFAULT void isr_dma2(void);
WEAK_DEFAULT void isr_dma3(void);
WEAK_DEFAULT void isr_uart2(void);
WEAK_DEFAULT void isr_i2c1(void);
WEAK_DEFAULT void isr_wut(void);
WEAK_DEFAULT void isr_gpiowake(void);
WEAK_DEFAULT void isr_spi0(void);
WEAK_DEFAULT void isr_wdt1(void);
WEAK_DEFAULT void isr_pt(void);
WEAK_DEFAULT void isr_i2c2(void);
WEAK_DEFAULT void isr_riscv(void);
WEAK_DEFAULT void isr_owm(void);
WEAK_DEFAULT void isr_ecc(void);
WEAK_DEFAULT void isr_dvs(void);
WEAK_DEFAULT void isr_simo(void);
WEAK_DEFAULT void isr_uart3(void);
WEAK_DEFAULT void isr_pcif(void);
WEAK_DEFAULT void isr_aes(void);
WEAK_DEFAULT void isr_i2s(void);
WEAK_DEFAULT void isr_cnn_fifo(void);
WEAK_DEFAULT void isr_cnn(void);
WEAK_DEFAULT void isr_lpcmp(void);

/* CPU specific interrupt vector table */
ISR_VECTOR(1) const isr_t vector_cpu[CPU_IRQ_NUMOF] = {
    [PF_IRQn]       = isr_pf,
    [WDT0_IRQn]     = isr_wdt0,
    [RTC_IRQn]      = isr_rtc,
    [TRNG_IRQn]     = isr_trng,
    [TMR0_IRQn]     = isr_tmr0,
    [TMR1_IRQn]     = isr_tmr1,
    [TMR2_IRQn]     = isr_tmr2,
    [TMR3_IRQn]     = isr_tmr3,
    [TMR4_IRQn]     = isr_tmr4,
    [TMR5_IRQn]     = isr_tmr5,
    [I2C0_IRQn]     = isr_i2c0,
    [UART0_IRQn]    = isr_uart0,
    [UART1_IRQn]    = isr_uart1,
    [SPI1_IRQn]     = isr_spi1,
    [ADC_IRQn]      = isr_adc,
    [FLC0_IRQn]     = isr_flc0,
    [GPIO0_IRQn]    = isr_gpio0,
    [GPIO1_IRQn]    = isr_gpio1,
    [GPIO2_IRQn]    = isr_gpio2,
    [DMA0_IRQn]     = isr_dma0,
    [DMA1_IRQn]     = isr_dma1,
    [DMA2_IRQn]     = isr_dma2,
    [DMA3_IRQn]     = isr_dma3,
    [UART2_IRQn]    = isr_uart2,
    [I2C1_IRQn]     = isr_i2c1,
    [WUT_IRQn]      = isr_wut,
    [GPIOWAKE_IRQn] = isr_gpiowake,
    [SPI0_IRQn]     = isr_spi0,
    [WDT1_IRQn]     = isr_wdt1,
    [PT_IRQn]       = isr_pt,
    [I2C2_IRQn]     = isr_i2c2,
    [RISCV_IRQn]    = isr_riscv,
    [OWM_IRQn]      = isr_owm,
    [ECC_IRQn]      = isr_ecc,
    [DVS_IRQn]      = isr_dvs,
    [SIMO_IRQn]     = isr_simo,
    [UART3_IRQn]    = isr_uart3,
    [PCIF_IRQn]     = isr_pcif,
    [AES_IRQn]      = isr_aes,
    [I2S_IRQn]      = isr_i2s,
    [CNN_FIFO_IRQn] = isr_cnn_fifo,
    [CNN_IRQn]      = isr_cnn,
    [LPCMP_IRQn]    = isr_lpcmp,
};
