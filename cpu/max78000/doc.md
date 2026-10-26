@defgroup        cpu_max78000 Analog Devices MAX78000 MCU
@ingroup         cpu
@brief           Analog Devices (formerly Maxim Integrated) MAX78000 MCU

## Overview

The MAX78000 is an Analog Devices (formerly Maxim Integrated) MCU with an Arm
Cortex-M4F core running at 100 MHz, a RISC-V coprocessor and a convolutional
neural network (CNN) accelerator. It provides 512 KiB of flash and 128 KiB of
SRAM.

Only the Cortex-M4F is supported by RIOT: the RISC-V core and the CNN
accelerator are left in their reset state (off). The CPU is clocked from the
internal primary oscillator (IPO, 100 MHz), the peripheral clock is half of
it (50 MHz).

| Peripheral          | Support status                                    |
|:------------------- |:------------------------------------------------- |
| GPIO                | supported (ports 0, 1 and 2, no open drain)       |
| GPIO IRQ            | supported                                         |
| TIMER               | supported (TMR0 to TMR3, 32-bit, one channel)     |
| UART                | supported (UART0 to UART2, clocked from the IBRO) |
| CPUID               | supported (unique serial number, 11 bytes)        |
| I2C                 | not yet supported                                 |
| SPI                 | not yet supported                                 |
| RTC                 | not yet supported                                 |
| WDT                 | not yet supported                                 |
| Flash               | not yet supported (no periph_flashpage)           |
| TRNG                | not yet supported                                 |
| Low power modes     | not yet supported                                 |

The timers run at the peripheral clock divided by a power of 2
(50 MHz / 2^n, n from 0 to 12): no prescaler gives 1 MHz, the boards
configure `ztimer_usec` with the closest frequency.

## Vendor headers

The device and register headers in `include/vendor` are imported from the
Analog Devices MSDK, see `include/vendor/README.md`.

## Flashing and debugging

The boot ROM runs before the application at reset: halting the core right
after a reset needs a hardware breakpoint at the end of the boot ROM, see the
OpenOCD configuration of the boards.
