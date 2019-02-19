/*
 * Copyright (C) 2018 Dream Chip Technologies GmbH, Thomas Metzler <thomas.metzler@dreamchip.de>
 *
 * Based on board/altera/socfpga_arria10/socfpga_arria10.c
 * Copyright (C) 2014 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <watchdog.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Print Board information
 */
int checkboard(void)
{
	WATCHDOG_RESET();
	puts("BOARD : Dream Chip Arria 10 SoM base\n");
	return 0;
}

