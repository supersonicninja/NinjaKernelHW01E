/******************************************************************************
  Copyright (c) 2011-2111 HUAWEI Incorporated. All Rights Reserved.
*******************************************************************************/

/*
 * Character-device access to raw NVE devices.
 *
 */


#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
//#include <linux/smp_lock.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nve.h>
//#include <linux/mtd/compatmac.h>
//#include <linux/mfd/nve_interface.h>

#include <asm/uaccess.h>

#define NVE_PARTITION_MAX_SIZE 8

static struct class *nve_class;
static struct NVE_struct *my_nve;

static void nve_notify_add(struct mtd_info* mtd)
{
	if (!mtd)
		return;

	if (NULL == my_nve)
		return;

	device_create(nve_class, NULL, MKDEV(my_nve->nve_major_number, mtd->index*2),
		      NULL, "nve%d", mtd->index);
}

static void nve_notify_remove(struct mtd_info* mtd)
{
	if (!mtd)
		return;

	if (NULL == my_nve)
		return;

	device_destroy(nve_class, MKDEV(my_nve->nve_major_number, mtd->index*2));
}

static struct mtd_notifier nve_notifier = {
	.add	= nve_notify_add,
	.remove	= nve_notify_remove,
};

static void nve_increment(struct NVE_struct *nve)
{
	if (nve->nve_current_id > nve->nve_partition_count - 2)
		nve->nve_current_id = 1;
	else
		nve->nve_current_id++;

	return;
}

static int nve_read(struct mtd_info *mtd, loff_t from, size_t len, u_char *buf)
{
	int ret = 0;
	size_t retlen;
	
	ret = mtd->read(mtd, from, len, &retlen, buf);
	if ((ret && (ret != -EUCLEAN) && (ret != -EBADMSG)) || (retlen != len)) {
		printk("[NVE] nve_read: fatal error, read flash from = 0x%x, len = 0x%x, retlen = 0x%x, ret = 0x%x.\n", (uint32_t)from, len, retlen, ret);
		//try more times?
		ret = -ENODEV;
	}
	else {
		ret = 0;
	}

	return ret;
}

static int nve_erase(struct mtd_info *mtd, loff_t from, size_t len)
{
	int ret = 0;
	struct erase_info erase;
	
	erase.addr = from;
	erase.len = len;
	erase.mtd = mtd;
	erase.callback = NULL;
	erase.priv = 0;

	ret = mtd->erase(mtd, &erase);
	if (ret) {
		printk("[NVE] nve_erase: fatal error, erase flash from = 0x%x, len = 0x%x, ret = 0x%x.\n", (uint32_t)from, len, ret);
	}

	return ret;
}

static int nve_write(struct mtd_info *mtd, loff_t from, size_t len, u_char *buf)
{
	int ret = 0;
	size_t retlen;

	ret = nve_erase(mtd, from, len);
	if (!ret) {
		ret = mtd->write(mtd, from, len, &retlen, buf);
		if ((ret) || (retlen != len)) {
			printk("[NVE] nve_write: fatal error, write flash from = 0x%x, len = 0x%x, retlen = 0x%x, ret = 0x%x.\n", (uint32_t)from, len, retlen, ret);
			//try more times?
			ret = -ENODEV;
		}
		
		mtd->sync(mtd);
		
	}	

	return ret;
}
	
static int nve_get_NVM(struct NVE_struct *nve, uint32_t index)
{
	int ret = 0;
	struct mtd_info *mtd = nve->mtd;
	struct NVE_partition_header *nve_header = (struct NVE_partition_header *)(nve->nve_ramdisk);
	
	ret = nve_read(mtd, index * NVE_PARTITION_SIZE, sizeof(struct NVE_partition_header), (u_char *)nve_header);
	if (!ret) {
		//compare nve_header_t.nve_partition_name with const name
		ret = strncmp(NVE_HEADER_NAME, nve_header->nve_partition_name, strlen(NVE_HEADER_NAME));
	}
	
	return ret;
}

static int nve_findvalidNVM(struct NVE_struct *nve)
{
	int ret = 0;
	struct NVE_partition_header *nve_header = (struct NVE_partition_header *)(nve->nve_ramdisk);
	uint32_t i, age_t = 0;
	
	for (i = 1; i < nve->nve_partition_count; i++) {
		ret = nve_get_NVM(nve, i);
		if (ret) {
			continue;
		}
		
		if (nve_header->nve_age > age_t) {
			nve->nve_current_id = i;
			age_t = nve_header->nve_age;
		}
	}

	return 0;
}

static int nve_restore(struct NVE_struct *nve)
{
	int ret = 0;
	uint32_t id = nve->nve_current_id;
	u_char *nve_ramdisk_temp = NULL;
	uint32_t i;
	struct mtd_info *mtd = nve->mtd;
	struct NVE_partition_header *nve_header;
	struct NVE_partition_header *nve_header_temp;
	struct NV_struct *nv;
	struct NV_struct *nv_temp;
	uint32_t valid_items;
	

	if (NVE_INVALID_NVM == id) {
		if (nve_read(mtd, 0, NVE_PARTITION_SIZE, (u_char *)nve->nve_ramdisk))
			return -1;
		id = 1;
	}
	else {
		nve_ramdisk_temp = (u_char *)kzalloc(NVE_PARTITION_SIZE, GFP_KERNEL);
		if (nve_ramdisk_temp == NULL) {
			printk("[NVE] failed to allocate ramdisk temp buffer.\n");
			return -1;
		}
		else {
 			if (nve_read(mtd, id * NVE_PARTITION_SIZE, NVE_PARTITION_SIZE, (u_char *)nve->nve_ramdisk)) {
				kfree(nve_ramdisk_temp);
				return -1;
			}
			
			if (nve_read(mtd, 0, NVE_PARTITION_SIZE, (u_char *)nve_ramdisk_temp)) {
				kfree(nve_ramdisk_temp);
				return -1;
			}

			nve_header = (struct NVE_partition_header *)nve->nve_ramdisk;
			nve_header_temp = (struct NVE_partition_header *)nve_ramdisk_temp;
			nv = (struct NV_struct *)(nve->nve_ramdisk + sizeof(struct NVE_partition_header));
			nv_temp = (struct NV_struct *)(nve_ramdisk_temp + sizeof(struct NVE_partition_header));
			
			valid_items = min(nve_header->valid_items, nve_header_temp->valid_items);
			
			for (i = 0; i < valid_items; i++, nv++, nv_temp++) {
				//skip non-volatile NV item
				if (nv->nv_header.nv_property) 
					continue;
				memcpy((void *)nv, (void *)nv_temp, sizeof(struct NV_struct));
			}
			
			if (nve_header_temp->valid_items > nve_header->valid_items) {
				memcpy((void *)nv, (void *)nv_temp, sizeof(struct NV_struct) * (nve_header_temp->valid_items - nve_header->valid_items));
				nve_header->valid_items = nve_header_temp->valid_items;
			}
			nve_header->nve_age++;
			nve_header->nve_version = nve_header_temp->nve_version;
		}

		if (id == nve->nve_partition_count - 1)
			id = 1;
		else
			id++;
	}
		
	ret = nve_write(mtd, id * NVE_PARTITION_SIZE, NVE_PARTITION_SIZE, (u_char *)nve->nve_ramdisk);
	if (!ret) {
		//erase block0
		nve_erase(mtd, 0, NVE_PARTITION_SIZE);
		nve_increment(nve);
	}

	return ret;
}

static int nve_do_index_table(struct NVE_struct *nve)
{
	u_char * p_nv;
	unsigned int i;
	unsigned int nv_offset;
	unsigned int nv_offset_next;
	struct NVE_index *nv_index;
	struct NV_header *nv;
	struct mtd_info *mtd = nve->mtd;
	struct NVE_partition_header *nve_header = (struct NVE_partition_header *)(nve->nve_ramdisk);

	if(nve_read(mtd, nve->nve_current_id * NVE_PARTITION_SIZE, NVE_PARTITION_SIZE, (u_char *)nve->nve_ramdisk)) {
		return -ENODEV;
	}
	
	printk("[NVE]nve_do_index_table:original valid_items: 0x%x\n", nve_header->valid_items);
	
	if (NULL != nve->nve_index_table) {
		kfree(nve->nve_index_table);
		nve->nve_index_table = NULL;
	}
	nve->nve_index_table = kzalloc(nve_header->valid_items * sizeof(struct NVE_index), GFP_KERNEL);
	if (nve->nve_index_table == NULL) {
		printk("[NVE] nve_do_index_table failed to allocate index table.\n");
		return -ENOMEM;
	}
	nv_offset = sizeof(struct NVE_partition_header);
	for (i = 0; i < nve_header->valid_items; i++) {
		nv_offset_next = nv_offset + sizeof(struct NV_header);
		if (nv_offset_next > NVE_PARTITION_SIZE -1) {
			//override, the end
			break;
		}
		p_nv = nve->nve_ramdisk + nv_offset;
		nv = (struct NV_header *)p_nv;
		if (i != nv->nv_number) {
			//invalid nv, the end
			break;
		}
		nv_offset_next += NVE_NV_DATA_SIZE;
		if (nv_offset_next > NVE_PARTITION_SIZE -1) {
			//override, the end
			break;
		}

		nv_index = nve->nve_index_table + i;
		nv_index->nv_offset = nv_offset;
		nv_index->nv_size = nv->valid_size;
		memcpy(nv_index->nv_name, nv->nv_name, NV_NAME_LENGTH);

		nv_offset = nv_offset_next;
	}
	nve_header->valid_items = i;
	
	printk("[NVE]nve_do_index_table:actual valid_items: 0x%x\n", nve_header->valid_items);

	return 0;
}

#if 0
static int testnve(struct NVE_struct *nve)
{
	struct NVE_partition_header *nve_header;
	struct NV_struct *nv;
	uint32_t valid_items;
	uint32_t i;

	nve_header = (struct NVE_partition_header *)nve->nve_ramdisk;
	nv = (struct NV_struct *)(nve->nve_ramdisk + sizeof(struct NVE_partition_header));
	valid_items = nve_header->valid_items;

	printk("[NVE] testnve: version %d\n", nve_header->nve_version);
	printk("[NVE] testnve: age %d\n", nve_header->nve_age);
	printk("[NVE] testnve: valid_items %d\n", valid_items);

	for (i = 0; i < valid_items; i++) {
		printk("[NVE] testnve: number %d, name %s, NV %d, size %d\n", nv->nv_header.nv_number, nv->nv_header.nv_name, nv->nv_header.nv_property, nv->nv_header.valid_size);
		printk("[NVE] testnve: data 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n\n", nv->nv_data[0], nv->nv_data[1], nv->nv_data[2], nv->nv_data[3], nv->nv_data[4], nv->nv_data[5]);
		nv++;
	}
	
	return 0;
}
#endif


static int nve_open_ex(void)
{
	int ret = 0;
	struct mtd_info *mtd;
	struct NVE_struct *nve;
	
	printk("nve_open_ex in\n");

	//the driver is not initialized successfully, return error
	if (NULL == my_nve) {
		ret = -ENODEV;
		goto out;
	}
	else
		nve = my_nve;

	//print something to indicate more APP call me at the same time
	//just to debug
	if (nve->initialized > 0) {
		goto out;
	}
    
	//mtd = get_mtd_device_nm(NVE_NAME);	
	mtd = get_mtd_device_nm("block2mtd: /dev/mmcblk0p2");	
	if (IS_ERR(mtd)) {
		ret = PTR_ERR(mtd);
		goto out;
	}

	if (MTD_ABSENT == mtd->type) {
		put_mtd_device(mtd);
		ret = -ENODEV;
		goto out;
	}

	nve->mtd = mtd;
	//nve->nve_partition_count = mtd->size / NVE_PARTITION_SIZE;
	nve->nve_partition_count = NVE_PARTITION_MAX_SIZE;
	nve->nve_current_id = NVE_INVALID_NVM;
	printk("[NVE] nve_open_ex: nve->nve_partition_count = 0x%x.\n", nve->nve_partition_count);

	//the count must bigger than 3, one for updata, the other for runtime 
	if (nve->nve_partition_count < 4){
		ret = -ENODEV;
		goto out;
	}
	
	nve_findvalidNVM(nve);
	
	//check block0
	ret = nve_get_NVM(nve, 0);
	if (!ret) {
		//block0 is valid, restore it to NVM
		ret = nve_restore(nve);
	}
	
	if (ret) {
		if (NVE_INVALID_NVM == nve->nve_current_id) {
			printk("[NVE] nve_open_ex: no valid NVM.\n");
			ret = -ENODEV;
			goto out;
		}
		else {
			ret = 0;
		}
	}

	if (nve_do_index_table(nve)) {
		ret = -ENODEV;
		goto out;
	}
	nve->initialized = 1;

out:
	printk("[NVE] nve_open_ex: out.\n");

	return ret;
} /* nve_open_ex */

int nve_direct_access(struct nve_info_user * pinfo)
{
	int ret = 0;
	struct mtd_info *mtd = NULL;
	struct NVE_struct *nve = NULL;
	struct NVE_index *nve_index = NULL;
	struct NV_header *nv = NULL;
	struct NVE_partition_header *nve_header = NULL;
	u_char *nv_data = NULL;

	
	printk("nve_direct_access in\n");

	//input parameter invalid, return.
    if (NULL == pinfo) {
		ret = -ENODEV;
		goto out;
    }
    
	//open nve.
    ret = nve_open_ex();
    if (ret) {
        goto out;
    }

	//the driver is not initialized successfully, return error
	if (NULL == my_nve) {
		ret = -ENODEV;
		goto out;
	}
	else
		nve = my_nve;

    mtd = nve->mtd;

    //find nv
	nve_header = (struct NVE_partition_header *)(nve->nve_ramdisk);

	if (pinfo->nv_number >= nve_header->valid_items)
	{
		printk("[NVE]nve_direct_access, nv[%d] is not defined.\n", pinfo->nv_number);
		return -EFAULT;
	}
	
	nve_index = nve->nve_index_table + pinfo->nv_number;
	
	if (pinfo->valid_size > NVE_NV_DATA_SIZE)
		pinfo->valid_size = NVE_NV_DATA_SIZE;
	else if (pinfo->valid_size == 0)
		pinfo->valid_size = nve_index->nv_size;
	
	nv_data = nve->nve_ramdisk + nve_index->nv_offset + sizeof(struct NV_header);

	if (pinfo->nv_read) {
		//read nv from ram
		memcpy(pinfo->nv_data, nv_data, pinfo->valid_size);
	}
	else
	{
		//write nv to ram
		nv = (struct NV_header *)(nve->nve_ramdisk + nve_index->nv_offset);
		nv->valid_size = pinfo->valid_size;
		nve_index->nv_size = pinfo->valid_size;
		memset(nv_data, 0x0, NVE_NV_DATA_SIZE);
		memcpy(nv_data, pinfo->nv_data, pinfo->valid_size);

		//updata nve_ramdisk
		nve_header->nve_age++;
		
		//write NVE to nand
		nve_increment(nve);
		ret = nve_write(mtd, nve->nve_current_id * NVE_PARTITION_SIZE, NVE_PARTITION_SIZE, (u_char *)nve->nve_ramdisk);
	}    

out:
	printk("[NVE] nve_direct_access: out.\n");

	return ret;
} /* nve_direct_access */

static int nve_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	
	printk("nve_open in\n");
	//lock_kernel();  /* remove by y00129977, need check */
    ret = nve_open_ex();
	//unlock_kernel();  /* remove by y00129977, need check */
	printk("[NVE] nve_open: out.\n");

	return ret;
} /* nve_open */

/*====================================================================*/

static int nve_close(struct inode *inode, struct file *file)
{
	return 0;
} /* nve_close */
#if 0
static int nve_ioctl(struct inode *inode, struct file *file,
		     u_int cmd, u_long arg)
#endif
static long nve_ioctl(struct file *file,
		     unsigned int cmd, u_long arg)
{
	int ret = 0;
	struct mtd_info *mtd = NULL;
	void __user *argp = (void __user *)arg;
	u_long size;
	struct nve_info_user info;
	struct NVE_struct *nve = NULL;
	struct NVE_index *nve_index = NULL;
	struct NV_header *nv = NULL;
	struct NVE_partition_header *nve_header = NULL;
	u_char *nv_data = NULL;
    struct nve_flag_user flag_info;
	if (NULL == my_nve) {
		return -EFAULT;
	}
	else
		nve = my_nve;

    mtd = nve->mtd;

	size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;
	printk("[NVE]nve_ioctl, size = 0x%x\n", (uint32_t)size);
	
	if (cmd & IOC_IN) {
		if (!access_ok(VERIFY_READ, argp, size))
			return -EFAULT;
	}
	if (cmd & IOC_OUT) {
		if (!access_ok(VERIFY_WRITE, argp, size))
			return -EFAULT;
	}

	switch (cmd) {
	case NVEACCESSDATA:
		if (copy_from_user(&info, argp, sizeof(struct nve_info_user)))
			return -EFAULT;
		
		//find nv
		nve_header = (struct NVE_partition_header *)(nve->nve_ramdisk);

		if (info.nv_number >= nve_header->valid_items)
		{
			printk("[NVE]nve_ioctl, nv[%d] is not defined.\n", info.nv_number);
			return -EFAULT;
		}
		
		nve_index = nve->nve_index_table + info.nv_number;
#if 0
		if (strncmp(info.nv_name, nve_index->nv_name, NV_NAME_LENGTH)) {
			printk("[NVE]nve_ioctl, nv[%d]'s name is not correct.\n", info.nv_number);			
			//find nv by name
			nve_index = nve->nve_index_table + nve_header->valid_items - 1;
			for (i = nve_header->valid_items; i  > 0; i--) {
				if (strncmp(info.nv_name, nve_index->nv_name, NV_NAME_LENGTH)) {
					info.nv_number = i - 1;
					break;
				}
				nve_index--;
			}
                  }
#endif
		
		if (info.valid_size > NVE_NV_DATA_SIZE)
			info.valid_size = NVE_NV_DATA_SIZE;
		else if (info.valid_size == 0)
			info.valid_size = nve_index->nv_size;
		
		nv_data = nve->nve_ramdisk + nve_index->nv_offset + sizeof(struct NV_header);

		if (info.nv_read) {
			//read nv from ram
			memcpy(info.nv_data, nv_data, info.valid_size);
			
			//send back to user
			if (copy_to_user(argp, &info, sizeof(struct nve_info_user)))
				return -EFAULT;
		}
		else
		{
			//write nv to ram
			nv = (struct NV_header *)(nve->nve_ramdisk + nve_index->nv_offset);
			nv->valid_size = info.valid_size;
			nve_index->nv_size = info.valid_size;
			memset(nv_data, 0x0, NVE_NV_DATA_SIZE);
			memcpy(nv_data, info.nv_data, info.valid_size);

			//updata nve_ramdisk
			nve_header->nve_age++;
			
			//write NVE to nand
			nve_increment(nve);
			ret = nve_write(mtd, nve->nve_current_id * NVE_PARTITION_SIZE, NVE_PARTITION_SIZE, (u_char *)nve->nve_ramdisk);
		}
		break;

	case NVWRITEFLAG:
		if (copy_from_user(&flag_info, argp, sizeof(struct nve_flag_user)))
		{
			return -EFAULT;
		}
		nve_write(mtd, flag_info.nvm_offset, sizeof(flag_info.nvm_flag), (u_char *)&flag_info.nvm_flag);
		break;

	default:
		ret = -ENOTTY;
		break;
	}
	
	return ret;
} /* memory_ioctl */

static const struct file_operations nve_fops = {
	.owner		                     = THIS_MODULE,
	.unlocked_ioctl			= nve_ioctl,  //modify from ioctl
	.open		                     = nve_open,
	.release		                     = nve_close,
};

static int __init init_nve(void)
{
	struct NVE_struct *nve = NULL;
	int error;

	printk("[NVE] init_nve in123.\n");
	my_nve = nve = kzalloc(sizeof(struct NVE_struct), GFP_KERNEL);
	if (nve == NULL) {
		printk("[NVE] failed to allocate driver data.\n");
		return -ENOMEM;
	}
	memset (nve, 0x0, sizeof(struct NVE_struct));
	
	nve->nve_ramdisk= (u_char *)kzalloc(NVE_PARTITION_SIZE, GFP_KERNEL);
	if (nve->nve_ramdisk == NULL) {
		printk("[NVE] failed to allocate ramdisk buffer.\n");
		error = -ENOMEM;
		goto failed_free_driver_data;
	}

	nve->nve_major_number = register_chrdev(0, "nve", &nve_fops);
	if (nve->nve_major_number < 0) {
		printk(KERN_NOTICE "Can't allocate major number for Non-Volatile memory Extension device.\n");
		error = -EAGAIN;
		goto failed_free_ramdisk;
	}

	nve_class = class_create(THIS_MODULE, "nve");

	if (IS_ERR(nve_class)) {
		printk(KERN_ERR "Error creating nve class.\n");
		unregister_chrdev(nve->nve_major_number, "nve");
		error = PTR_ERR(nve_class);
		goto failed_free_ramdisk;
	}

	register_mtd_user(&nve_notifier);
	printk(KERN_NOTICE "init_nve nve_major_number = 0x%x.\n", nve->nve_major_number);
	return 0;
	
failed_free_ramdisk:
	kfree(nve->nve_ramdisk);
	nve->nve_ramdisk = NULL;
failed_free_driver_data:
	kfree(nve);
	my_nve = NULL;
	return error;
}

static void __exit cleanup_nve(void)
{
	unregister_mtd_user(&nve_notifier);
	class_destroy(nve_class);
	if (NULL != my_nve)
	{
		unregister_chrdev(my_nve->nve_major_number, "mtd");
		kfree(my_nve->nve_index_table);
		my_nve->nve_index_table = NULL;
		kfree(my_nve->nve_ramdisk);
		my_nve->nve_ramdisk = NULL;
		kfree(my_nve);
		my_nve = NULL;
	}	
	return;
}

module_init(init_nve);
module_exit(cleanup_nve);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Yajia");
MODULE_DESCRIPTION("Direct character-device access to NVE devices");

