/*
 * Copyright (C) 2018 Dream Chip Technologies GmbH, Thomas Metzler <thomas.metzler@dreamchip.de>
 *
 * Based on board/altera/socfpga_arria10/socfpga_common.c
 * Copyright (C) 2014 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <watchdog.h>
#include <asm/arch/system_manager.h>
#include <asm/arch/reset_manager.h>
#include <asm/arch/misc.h>
#include <phy.h>
#include <micrel.h>
#include <miiphy.h>
#include <netdev.h>
#include <i2c.h>
#include <fdt_support.h>
#include "../../../drivers/net/designware.h"

DECLARE_GLOBAL_DATA_PTR;

/*DCT::TM GPIO defines*/
#define GPIO_0_BASE 0xffc02900
#define GPIO_1_BASE 0xffc02a00
#define GPIO_2_BASE 0xffc02b00

#define GPIO_PORTA_DR(base)  (base + 0x0)
#define GPIO_PORTA_DDR(base) (base + 0x4)

#define GPIO(pin) 1 << pin

/*
 * Initialization function which happen at early stage of c code
 */
int board_early_init_f(void)
{
	if (is_regular_boot())
		gd->flags |= (GD_FLG_SILENT | GD_FLG_DISABLE_CONSOLE);

	WATCHDOG_RESET();
	return 0;
}

/*
 * Miscellaneous platform dependent initialisations
 */
int board_init(void)
{
	/* adress of boot parameters for ATAG (if ATAG is used) */
	gd->bd->bi_boot_params = 0x00000100;
	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	/*set GPIO 2.10 to high (release reset USB1)*/
	setbits_le32(GPIO_PORTA_DR(GPIO_2_BASE), GPIO(10));
	/*set GPIO 2.10 as output (release reset USB1)*/
	setbits_le32(GPIO_PORTA_DDR(GPIO_2_BASE), GPIO(10));

	/*set GPIO 2.11 to high (release reset DP)*/
	setbits_le32(GPIO_PORTA_DR(GPIO_2_BASE), GPIO(11));
	/*set GPIO 2.11 as output (release reset DP)*/
	setbits_le32(GPIO_PORTA_DDR(GPIO_2_BASE), GPIO(11));

	/*set GPIO 1.22 to high (release reset GEN)*/
	setbits_le32(GPIO_PORTA_DR(GPIO_1_BASE), GPIO(22));
	/*set GPIO 1.22 as output (release reset GEN)*/
	setbits_le32(GPIO_PORTA_DDR(GPIO_1_BASE), GPIO(22));

	return 0;
}
#endif

ulong
socfpga_get_emac_control(unsigned long emacbase)
{
	ulong base = 0;
	switch (emacbase) {
		case SOCFPGA_EMAC0_ADDRESS:
			base = CONFIG_SYSMGR_EMAC0_CTRL;
			break;
		case SOCFPGA_EMAC1_ADDRESS:
			base = CONFIG_SYSMGR_EMAC1_CTRL;
			break;
		case SOCFPGA_EMAC2_ADDRESS:
			base = CONFIG_SYSMGR_EMAC2_CTRL;
			break;
		default:
			error("bad emacbase %lx\n", emacbase);
			hang();
			break;
	}
	return base;
}

ulong
socfpga_get_phy_mode(ulong phymode)
{
	ulong val;
	switch (phymode) {
		case PHY_INTERFACE_MODE_GMII:
			val = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_GMII_MII;
			break;
		case PHY_INTERFACE_MODE_MII:
			val = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_GMII_MII;
			break;
		case PHY_INTERFACE_MODE_RGMII:
			val = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII;
			break;
		case PHY_INTERFACE_MODE_RMII:
			val = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RMII;
			break;
		default:
			error("bad phymode %lx\n", phymode);
			hang();
			break;
	}
	return val;
}

int is_ksz9031(struct phy_device *phydev)
{
	unsigned short phyid1;
	unsigned short phyid2;

	phyid1 = phy_read(phydev, MDIO_DEVAD_NONE, MII_PHYSID1);
	phyid2 = phy_read(phydev, MDIO_DEVAD_NONE, MII_PHYSID2);

	phyid2 = phyid2 & MICREL_KSZ9031_PHYID2_REVISION_MASK;

	debug("phyid1 %04x, phyid2 %04x\n", phyid1, phyid2);

	if ((phyid1 == MICREL_KSZ9031_PHYID1) &&
	    (phyid2 == MICREL_KSZ9031_PHYID2))
		return 1;
	return 0;
}



int board_phy_config(struct phy_device *phydev)
{
	int reg;
	int devad = MDIO_DEVAD_NONE;

	reg = phy_read(phydev, devad, MII_BMCR);
	if (reg < 0) {
		debug("PHY status read failed\n");
		return -1;
	}

	if (reg & BMCR_PDOWN) {
		reg &= ~BMCR_PDOWN;
		if (phy_write(phydev, devad, MII_BMCR, reg) < 0) {
			debug("PHY power up failed\n");
			return -1;
		}
		udelay(1500);
	}


	if (is_ksz9031(phydev)) {
		unsigned short reg4;
		unsigned short reg5;
		unsigned short reg6;
		unsigned short reg8;

		reg4 = getenv_ulong("ksz9031-rgmii-ctrl-skew", 16, 0x77);
		reg5 = getenv_ulong("ksz9031-rgmii-rxd-skew", 16, 0x7777);
		reg6 = getenv_ulong("ksz9031-rgmii-txd-skew", 16, 0x7777);
		reg8 = getenv_ulong("ksz9031-rgmii-clock-skew", 16, 0x1ef);

		ksz9031_phy_extended_write(phydev, 2, 4,
					   MII_KSZ9031_MOD_DATA_NO_POST_INC,
					   reg4);
		ksz9031_phy_extended_write(phydev, 2, 5,
					   MII_KSZ9031_MOD_DATA_NO_POST_INC,
					   reg5);
		ksz9031_phy_extended_write(phydev, 2, 6,
					   MII_KSZ9031_MOD_DATA_NO_POST_INC,
					   reg6);
		ksz9031_phy_extended_write(phydev, 2, 8,
					   MII_KSZ9031_MOD_DATA_NO_POST_INC,
					   reg8);

		ksz9031_phy_extended_write(phydev, 0,
			MII_KSZ9031RN_FLP_BURST_TX_HI,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0006);

		ksz9031_phy_extended_write(phydev, 0,
			MII_KSZ9031RN_FLP_BURST_TX_LO,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x1A80);
	}
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

/*DCT:TM add support for at24mac */
#ifdef CONFIG_AT24MAC
int set_ethaddr_from_at24mac(int at24mac_addr)
{
	unsigned char ethaddr[6];
	int ret = 0;

	/*read ethaddr from environment*/
	ret = eth_getenv_enetaddr("ethaddr", ethaddr);
	if (ret) {
		printf("Ethernet MAC is set via environment to %pM\n",ethaddr);
		return 0;
	}

	/* Set I2C bus to 1 */
	if (i2c_set_bus_num(1)) {
		error("i2c bus 1 is not valid\n");
		return -1;
	}
	/*read eui form at24mac starting at 0x9A*/
	if (i2c_read(at24mac_addr, 0x9A, 1, ethaddr, 6)) {
		error("EUI address read failed on device 0x%02x at i2c bus 1\n",at24mac_addr);
		return -1;
	}

	if (!is_valid_ether_addr(ethaddr)) {
		error("EUI address read from at24mac(device0x%02x, i2c-bus 1) is not valid!\n",at24mac_addr);
		return -1;
	}
	printf("Set ethaddr to %pM\n",ethaddr);
	return eth_setenv_enetaddr("ethaddr", ethaddr);
}
#endif
/*DCT::TM provide mac to linux via DT*/
#ifdef CONFIG_OF_BOARD_SETUP
void ft_board_setup(void *blob, bd_t *bd)
{
	uint8_t enetaddr[6];

	/* MAC addr */
	if (eth_getenv_enetaddr("ethaddr", enetaddr)) {
		int ret = 0;
		ret = fdt_find_and_setprop(blob, "ethernet1", "local-mac-address",
				     enetaddr, 6, 1);
		if(ret)
			error("Could not set Ethernet MAC in device tree\n");
	}
}
#endif



#ifdef CONFIG_DESIGNWARE_ETH
/* We know all the init functions have been run now */
int board_eth_init(bd_t *bis)
{
	ulong emacctrlreg;
	ulong reg32;

	emacctrlreg = socfpga_get_emac_control(CONFIG_EMAC_BASE);

	/* Put the emac we're using into reset. 
	 * This is required before configuring the PHY interface
	 */
	emac_manage_reset(CONFIG_EMAC_BASE, 1);

	reg32 = readl(emacctrlreg);
	reg32 &= ~SYSMGR_EMACGRP_CTRL_PHYSEL_MASK;

	reg32 |= socfpga_get_phy_mode(CONFIG_PHY_INTERFACE_MODE);

	writel(reg32, emacctrlreg);

	/* Now that the PHY interface is configured, release
	 * the EMAC from reset. Delay a little bit afterwards
	 * just to make sure reset is completed before first access
	 * to EMAC CSRs. 
	 */
	emac_manage_reset(CONFIG_EMAC_BASE, 0);

#ifdef CONFIG_AT24MAC
	//DCT::TM i2c is on FPGA pins so this is called via bootcmd set_ethaddr_from_at24mac(CONFIG_AT24MAC_EXT_ADDR);
#endif
	/* initialize and register the emac */
	return designware_initialize(CONFIG_EMAC_BASE,
					CONFIG_PHY_INTERFACE_MODE);
}
#endif

