// SPDX-License-Identifier: GPL-2.0+
/*
 * dizi bring-up: красим ВЕСЬ cont_splash, потом уходим в ребут.
 * ВРЕМЕННЫЙ отладочный файл.
 */

#include <init.h>
#include <linux/types.h>
#include <linux/arm-smccc.h>
#include <linux/psci.h>

#define FB_BASE		0xb8000000UL
#define FB_SIZE		0x02b00000UL	/* весь cont_splash из ABL DTB */
#define PASSES		8		/* держим цвет пару секунд */

int board_early_init_f(void)
{
	volatile u32 *fb = (volatile u32 *)FB_BASE;
	unsigned long n = FB_SIZE / sizeof(u32);
	struct arm_smccc_res res;
	unsigned long i;
	int pass;

	for (pass = 0; pass < PASSES; pass++)
		for (i = 0; i < n; i++)
			fb[i] = 0x00ff00ff;	/* малиновый */

	/* наблюдаемый маркер, не зависящий от экрана */
	arm_smccc_smc(PSCI_0_2_FN_SYSTEM_RESET, 0, 0, 0, 0, 0, 0, &res);

	while (1)
		;

	return 0;
}
