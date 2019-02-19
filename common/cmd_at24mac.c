/*
 * Copyright 2017 Dream Chip Technologies
 * Thomas Metzler (thomas.metzler@dreamchip.de)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

DECLARE_GLOBAL_DATA_PTR;

extern int set_ethaddr_from_at24mac(int at24mac_addr);

static int do_at24mac(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int retval = CMD_RET_SUCCESS;
	retval = set_ethaddr_from_at24mac(CONFIG_AT24MAC_EXT_ADDR);
	return retval;
}


U_BOOT_CMD(
	at24mac, 1, 1,  do_at24mac,
	"read MAC from at24 EEPROM and set it to the environment",
);
