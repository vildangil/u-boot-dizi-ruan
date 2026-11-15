/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for Xiaomi Redmi Pad Pro (dizi)
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

/* Базовые настройки для плат Qualcomm */
#define CFG_SYS_BAUDRATE_TABLE { 115200, 230400, 460800, 921600 }

/* Дополнительные настройки можно добавить позже по мере необходимости */
#define CFG_SYS_SDRAM_BASE 0x80000000

#endif /* __CONFIG_H */