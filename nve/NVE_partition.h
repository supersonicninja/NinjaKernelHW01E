/*
 *  NVE_partition.h
 *
 *  Copyright:	(C) Copyright 2009 Marvell International Ltd.
 *
 *  Author: Chen Yajia <>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  publishhed by the Free Software Foundation.
 *  use for CONFIG_BOARD_IS_HUAWEI_PXA920
 */
 
/******************************************************************************
  Copyright (c) 2011-2111 HUAWEI Incorporated. All Rights Reserved.
*******************************************************************************/

#ifndef __NVE_PARTITION_H
#define __NVE_PARTITION_H

#define NVE_PARTITION_SIZE (128 * 1024)

#define NV_NAME_LENGTH 8

struct NVE_partition_header {
	char nve_partition_name[32];
	unsigned int nve_version;
	unsigned int nve_age;
	unsigned int nve_id;
	unsigned int nve_count;
	unsigned int valid_items;
	unsigned int nv_checksum;
	unsigned char reserved[72];	
};

struct NV_struct {
	unsigned int nv_number;
	char nv_name[NV_NAME_LENGTH];
	unsigned int nv_property;  //bit 0:"0" for volatile;"1" for non-volatile.
	unsigned int nv_data_size;
	unsigned int reserved;
	char nv_data[104];
};

struct NVE_partittion {
	struct NVE_partition_header header;
	struct NV_struct NVs[1023];
};

#endif /* __NVE_PARTITION_H */
