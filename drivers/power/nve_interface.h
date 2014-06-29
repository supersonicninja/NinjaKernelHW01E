/******************************************************************************
  Copyright (c) 2011-2111 HUAWEI Incorporated. All Rights Reserved.
*******************************************************************************/


#ifndef __NVE_INTERFACE_H__
#define __NVE_INTERFACE_H__

//#if defined(CONFIG_BOARD_IS_HUAWEI_PXA920)
#include <linux/types.h>
#define NV_NAME_LENGTH 			                        8
#define NVE_NV_DATA_SIZE			                    104

#define TEL_HUAWEI_NV_WRITE			                    0
#define TEL_HUAWEI_NV_READ				                1
#define TEL_HUAWEI_NV_DEVICE				            "/dev/nve1"

#define TEL_HUAWEI_NV_BATTERYPARAMETER_NAME             "BATPARA"
#define TEL_HUAWEI_NV_BATTERYPARAMETER_NUMBER           199
#define TEL_HUAWEI_NV_BATTERYPARAMETER_LENGTH           10

typedef enum
{
	HWEI_NV_OP_OK,
	HWEI_NV_INVALID_OP,
	HWEI_NV_OP_NOT_ACTIVE,
}hwei_nv_op_ret_type;

struct nve_info_user {
	uint32_t nv_read;
	uint32_t nv_number;
	char nv_name[NV_NAME_LENGTH];
	uint32_t valid_size;
	u_char nv_data[NVE_NV_DATA_SIZE];
};
extern int nve_direct_access(struct nve_info_user * pinfo);
extern int access_nve_in_kernel(int nv_read, int nv_number, char* nv_name, int nv_len, void* nvbuf);

//#endif
#endif /* __NVE_INTERFACE_H__ */

