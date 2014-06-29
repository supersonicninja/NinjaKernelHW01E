

#include "config_mgr.h"
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#define ATAG_PRODUCT_BOOTID 0x4D534D7B

typedef struct
{
	unsigned int boot_id0;
	unsigned int boot_id1;
	unsigned int reverse[6];
}hw_product_info_type;

static hw_product_info_type hw_product_info;
static bool product_info_initialised = false;

int __init parse_tag_product_info(const struct tag *tags)
{
	struct tag *t = (struct tag*)tags;
	hw_product_info.boot_id0 = t->u.revision.rev;
	hw_product_info.boot_id1 = *((unsigned int*)((void*)&t->u.revision.rev+sizeof(unsigned int))); /* get next int*/
	product_info_initialised = select_hw_config(hw_product_info.boot_id0, hw_product_info.boot_id1);
	return product_info_initialised;		
}
__tagtable(ATAG_PRODUCT_BOOTID, parse_tag_product_info);
