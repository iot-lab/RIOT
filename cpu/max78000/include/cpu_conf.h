/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup     cpu_max78000
 * @{
 *
 * @file
 * @brief       MAX78000 specific CPU configuration
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#ifdef CPU_MODEL_MAX78000
#  include "vendor/max78000.h"
#  include "vendor/flc_regs.h"
#  include "vendor/gpio_regs.h"
#  include "vendor/icc_regs.h"
#  include "vendor/lpgcr_regs.h"
#  include "vendor/tmr_regs.h"
#  include "vendor/uart_regs.h"
#else
#  error "The CPU_MODEL of your board is currently not supported"
#endif

#include "cpu_conf_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    ARM Cortex-M specific CPU configuration
 * @{
 */
#define CPU_DEFAULT_IRQ_PRIO            (1U)            /**< Default ARM IRQ priority */
#define CPU_FLASH_BASE                  (MXC_FLASH_MEM_BASE) /**< Flash base address */
#define CPU_IRQ_NUMOF                   (MXC_IRQ_EXT_COUNT)  /**< MAX78000 specific IRQ count */
/** @} */

#ifdef __cplusplus
}
#endif

/** @} */
