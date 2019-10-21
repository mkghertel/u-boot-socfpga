// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015 Altera Corporation <www.altera.com>
 */

#include <common.h>

int mac_read_from_eeprom(void)
{
	const int ETH_ADDR_LEN = 6;
	unsigned char ethaddr[ETH_ADDR_LEN];
	const char *ETHADDR_NAME = "ethaddr";
	struct udevice *dev;
	char *envaddr;
	int ret;
	
	envaddr = env_get(ETHADDR_NAME);
	if (envaddr) {
		printf("Ethernet MAC is set via environment to %s\n", envaddr);
		return 0;
	}
	
	ret = uclass_first_device_err(UCLASS_I2C_EEPROM, &dev);
	if (ret) {
		printf("Could not find at24mac device.\n");
		return ret;
	}
	
	ret = i2c_eeprom_read(dev, 0x9A, ethaddr, 6);
	if (ret) {
		printf("Could not read from at24mac.\n");
		return ret;
	}
	
	if (is_valid_ethaddr(ethaddr)) {
		eth_env_set_enetaddr(ETHADDR_NAME, ethaddr);
	}
	else {
		printf("Ethernet MAC %pM from at24mac is not valid.\n", ethaddr);
	}

	return 0;
}
