/*============================================================================

                 DDR Information FS

   DESCRIPTION
     This file contains the function to create /proc/hwfuse for lpDDR Information.
            
   Copyright (c) 2007-2011 by HUAWEI Technologies Co., Ltd.  All Rights Reserved.
============================================================================*/

/*============================================================================

                      EDIT HISTORY FOR FILE

 This section contains comments describing changes made to this file.
 Notice that changes are listed in reverse chronological order.

 $Header: ... ... /hwfuse.c
 $DateTime: 2011/12/30 
 $Author: chenlide

=============================================================================*/

/*=============================================================================
Add Histrory Here
=============================================================================*/


/*============================================================================
                        INCLUDE FILES 
============================================================================*/

#include <linux/io.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <mach/msm_iomap.h>
#include <linux/module.h>
#include <asm/uaccess.h>

static int hw_fuse_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int hw_fuse_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t hw_fuse_read(struct file *filp, char* __user buf_user, size_t size, loff_t* offset)
{
	/* n=[1..28] */
	#define SECURE_BOOT(n) (0x706034+4*n)
	void __iomem *psecboot = NULL;
	ssize_t ret = 0;

	psecboot = ioremap_nocache(SECURE_BOOT(1), 4*28);

	if(!psecboot)
	{
		pr_err("ioremap failed.\n");
		return ret;
	}

	if(size >= 4){
		unsigned int secvalue = *(unsigned int*)psecboot;
		if(copy_to_user(buf_user, &secvalue, sizeof(unsigned int)))
		{
			/* err */
			pr_err("copy to user space from kernel failed.\n");
			goto err;
		}
		return sizeof(unsigned int);
	}else{
		pr_err("the user buf is smaller than 4 bytes\n");
		goto err;
	}

err:
	iounmap(psecboot);
	return ret;
}

static const struct file_operations hw_fuse_fops = {
	.owner = THIS_MODULE,
	.read = hw_fuse_read,
	.open = hw_fuse_open,
	.release = hw_fuse_release
};

static int __init proc_hwfuse_init(void)
{
	proc_create("hwfuse", 0, NULL, &hw_fuse_fops);
	return 0;
}
module_init(proc_hwfuse_init);
