// SPDX-License-Identifier: GPL-2.0+
/*
 * Xiaomi Redmi Pad Pro (dizi) — bring-up маркеры на экране.
 *
 * ВРЕМЕННЫЙ отладочный файл. Рисует цветные полосы прямо в cont_splash,
 * чтобы видеть, докуда дошёл U-Boot, без UART, USB и консоли.
 *
 * Убрать потом: удалить board.c и Makefile (dizi.env оставить!).
 */

#include <init.h>
#include <linux/types.h>

#define FB_BASE		0xb8000000UL	/* cont_splash из ABL DTB dizi */
#define FB_WIDTH	1600
#define FB_HEIGHT	2560

/* формат a8r8g8b8 -> 0x00RRGGBB */
#define COLOR_MAGENTA	0x00ff00ff
#define COLOR_BLUE	0x000000ff
#define COLOR_GREEN	0x0000ff00

/* определены как __weak в arch/arm/mach-snapdragon/board.c */
void qcom_board_init(void);
void qcom_late_init(void);

static void paint_band(unsigned int y0, unsigned int y1, u32 color)
{
	volatile u32 *fb = (volatile u32 *)
		(FB_BASE + (unsigned long)y0 * FB_WIDTH * 4);
	unsigned int n = (y1 - y0) * FB_WIDTH;
	unsigned int i;

	for (i = 0; i < n; i++)
		fb[i] = color;
}

/* 1. Очень рано: до serial_init и консоли. Малиновая полоса сверху. */
int board_early_init_f(void)
{
	paint_band(0, FB_HEIGHT / 3, COLOR_MAGENTA);
	return 0;
}

/* 2. board_init(), уже после релокации. Синяя полоса посередине. */
void qcom_board_init(void)
{
	paint_band(FB_HEIGHT / 3, 2 * FB_HEIGHT / 3, COLOR_BLUE);
}

/* 3. board_late_init(), прямо перед main_loop/preboot. Зелёная снизу. */
void qcom_late_init(void)
{
	paint_band(2 * FB_HEIGHT / 3, FB_HEIGHT, COLOR_GREEN);
}
