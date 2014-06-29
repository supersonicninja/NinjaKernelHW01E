//#if defined(CONFIG_BOARD_IS_HUAWEI_PXA920)
/*
 *  linux/drivers/NVE/NVE.h
 *
 *  Copyright:	(C) Copyright 2009 Marvell International Ltd.
 *
 *  Author: Chen Yajia <>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  publishhed by the Free Software Foundation.
 *
 */
 
/******************************************************************************
  Copyright (c) 2011-2111 HUAWEI Incorporated. All Rights Reserved.
*******************************************************************************/


#ifndef __NVE_H
#define __NVE_H

#define NVE_PARTITION_SIZE (128 * 1024)

#define NVE_PARTITION_NUMBER 7

#define NV_NAME_LENGTH 8

#define NVE_NV_DATA_SIZE	104


/*NVE_name named in uboot, nve driver get mtd info through this name*/
#define NVE_NAME	"nvme"		

#define NVE_INVALID_NVM 0xFFFFFFFF

struct NVE_index {
	char nv_name[NV_NAME_LENGTH];
	unsigned int nv_offset;
	unsigned int nv_size;
};

struct NVE_partition_header {
	char nve_partition_name[32];
	unsigned int nve_version;			//should be built in image with const value
	unsigned int nve_age;				//changed by run-time image
	unsigned int nve_block_id;			//should be built in image with const value
	unsigned int nve_block_count;		//should be built in image with const value
	unsigned int valid_items;			//should be built in image with const value
	unsigned int nv_checksum;
	unsigned char reserved[72];	
};

struct NV_header {
	unsigned int nv_number;
	char nv_name[NV_NAME_LENGTH];
	unsigned int nv_property;  //bit 0:"0" for volatile;"1" for non-volatile.
	unsigned int valid_size;
	unsigned char reserved[4];
};

struct NV_struct {
	struct NV_header nv_header;
	u_char nv_data[NVE_NV_DATA_SIZE];
};

struct NVE_struct {
	int nve_major_number;
	int initialized;
	unsigned int nve_partition_count;
	unsigned int nve_current_id;
	struct NVE_index *nve_index_table;
	struct mtd_info *mtd;
	u_char *nve_ramdisk;
};

#define NVE_HEADER_NAME            "TD-nvm-extension-partition"                                /* ReliableData area */
#endif /* __NVE_H */
//#endif
