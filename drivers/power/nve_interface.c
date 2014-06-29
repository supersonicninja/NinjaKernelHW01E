/******************************************************************************
  Copyright (c) 2011-2111 HUAWEI Incorporated. All Rights Reserved.
*******************************************************************************/

#include "nve_interface.h"
#include <linux/string.h>

int access_nve_in_kernel(int nv_read, int nv_number, char* nv_name, int nv_len, void* nvbuf)
{
    int ret = 0;
	struct nve_info_user info;

    memset(&info, 0, sizeof(info));
	info.nv_read = nv_read;
	info.nv_number = nv_number;
	strcpy(info.nv_name, nv_name);
	info.valid_size = 0;
    
	if(TEL_HUAWEI_NV_WRITE==nv_read)
	{
		memcpy(&info.nv_data, nvbuf, nv_len);
	}
	ret = nve_direct_access(&info);
	if(ret)
	{
        //print something
	}
	else
	{
		if(TEL_HUAWEI_NV_READ==nv_read)
		{
			memcpy(nvbuf, &info.nv_data, nv_len);
		}
	}
    
	return ret;
}


