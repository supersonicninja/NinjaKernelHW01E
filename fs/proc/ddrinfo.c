/*============================================================================

                 DDR Information FS

   DESCRIPTION
     This file contains the function to create /proc/ddrinfo for lpDDR Information.
            
   Copyright (c) 2007-2011 by HUAWEI Technologies Co., Ltd.  All Rights Reserved.
============================================================================*/

/*============================================================================

                      EDIT HISTORY FOR FILE

 This section contains comments describing changes made to this file.
 Notice that changes are listed in reverse chronological order.

 $Header: ... ... /DDRinfo.c
 $DateTime: 2011/12/30 
 $Author: chenlide

=============================================================================*/




/*============================================================================
                        INCLUDE FILES 
============================================================================*/

#include <linux/io.h>
#include <linux/fs.h>
#include <linux/hugetlb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/quicklist.h>
#include <linux/seq_file.h>
#include <mach/msm_iomap.h>
#include <asm/mach-types.h>
#include <mach/msm_smd.h>
#include <mach/msm_smsm.h>


/*
  share struct for HUAWEI who want to pass paramater from sbl to aboot
  use SMEM ID: SMEM_ID_VENDOR0
*/
#define SBL3_VERSION_LEN 64
typedef struct
{
  //battery connect status
  uint battery_status;

  //sbl3 version
  char  sbl3_version[SBL3_VERSION_LEN];

  //add here for HUAWEI extensional paramater
  //int sample;
  uint lpddr_info;

  //logctl
    char logctl;

}smem_exten_huawei_paramater;


/*
    Read SMEM to get the lpDDR Inform and print out
*/
static int ddrinfo_proc_show(struct seq_file *m, void *v)
{
        unsigned int ddr_id = 0;

	 smem_exten_huawei_paramater * param = NULL;

        param = smem_alloc(SMEM_ID_VENDOR0, sizeof(smem_exten_huawei_paramater));

        if(param == NULL)
		return -1;

        ddr_id = param->lpddr_info;

	if(ddr_id&0x000000FF)
	{
	    seq_printf(m, "SDRAM Interface0 : %d\n", (ddr_id&0x000000FF));   
	}
	else
	{
	    seq_printf(m, "SDRAM Interface0 : N/A\n");   
	}


	if(ddr_id&0x00FF0000)
	{
	    seq_printf(m, "SDRAM Interface1 : %d\n", ((ddr_id&0x00FF0000)>>16));   
	}
	else
	{
	    seq_printf(m, "SDRAM Interface1 : N/A\n");   
	}

        return 0;
}

static int ddrinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ddrinfo_proc_show, NULL);
}

static const struct file_operations ddrinfo_proc_fops = {
	.open		= ddrinfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_ddrinfo_init(void)
{
	proc_create("ddrinfo", 0, NULL, &ddrinfo_proc_fops);
	return 0;
}
module_init(proc_ddrinfo_init);

