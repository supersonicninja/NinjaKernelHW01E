/*
 *  linux/drivers/mmc/core/mmc_wifi.h
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright 2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _MMC_WIFI_H
#define _MMC_WIFI_H

/* three sdio buses are used in five,
 * and SDC4 is the third,
 * so index of SDC4 is 2*/
#define MMC_SDIO_INDEX 2

/* flag : whether wifi trigger detected card.
 * it is initialized by 0 in core.c .
 * it will be set to 1 when called the function bcm_detect_card();
 * and it will be set to 0 again when called the function mmc_rescan(); */
extern int wifi_detect_flag;

/* device id: SDC4, so it is 4. */
#define SDIO_WLAN_ID 4

#endif

