

/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include "board-8960.h"
#include <linux/wlan_plat.h>
#include <linux/random.h>

/* If bcm wifi need repeat insmod dhd.ko, dhd will pre alloc static buf.
 * because loading dhd need alloc memory, Repeat loading dhd.ko will bring fragment
 * The macro DHD_USE_STATIC_BUF in dhd Makefile control this feature of pre alloc static buf.
 * If Makefile in dhd open macro DHD_USE_STATIC_BUF, macro HUAWEI_BCMDHD_USE_STATIC_BUF must open.
 */
#define HUAWEI_BCMDHD_USE_STATIC_BUF
#ifdef HUAWEI_BCMDHD_USE_STATIC_BUF
#include <linux/skbuff.h>
#endif

#ifndef __NVE_INTERFACE_H__
#include "../../../drivers/power/nve_interface.h"
#include"../../../include/linux/uaccess.h"
#endif

extern void bcm_detect_card(int n);

#define WLAN_MAC_LEN            6
#define NV_WLAN_NUM             193
#define NV_WLAN_VALID_SIZE      12
#define MAC_ADDRESS_FILE  "/data/misc/wifi/macaddr"
unsigned char g_wifimac[WLAN_MAC_LEN] = {0};

#ifdef HUAWEI_BCMDHD_USE_STATIC_BUF
#define PREALLOC_WLAN_NUMBER_OF_SECTIONS	4
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS		160
#define PREALLOC_WLAN_SECTION_HEADER		24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)
#define WLAN_SKB_BUF_NUM	16

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

typedef struct wifi_mem_prealloc_struct {
	void *mem_ptr;
	unsigned long size;
} wifi_mem_prealloc_t;

static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_NUMBER_OF_SECTIONS] = {
	{ NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER) }
};
#endif

static void *bcm_wifi_mem_prealloc(int section, unsigned long size)
{
#ifdef HUAWEI_BCMDHD_USE_STATIC_BUF
    if (section == PREALLOC_WLAN_NUMBER_OF_SECTIONS)
        return wlan_static_skb;
    if ((section < 0) || (section > PREALLOC_WLAN_NUMBER_OF_SECTIONS))
        return NULL;
    if (wifi_mem_array[section].size < size)
        return NULL;
    return wifi_mem_array[section].mem_ptr;
#else
    return NULL;
#endif
}

static int bcm_wifi_set_power(int enable)
{
	int ret = 0;
	int pmic_gpio_wl_reg_on = 0;

	pmic_gpio_wl_reg_on = get_pm_gpio_num_by_name("BCM_WL_REG_ON"); //pmic gpio control the wifi power on
	if (pmic_gpio_wl_reg_on < 0)
	{
		printk(KERN_ERR "%s: get pmic gpio error: (%d)\n", __func__, pmic_gpio_wl_reg_on);
		BUG_ON(pmic_gpio_wl_reg_on < 0);
	}
	if (enable)
	{       
		ret = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(pmic_gpio_wl_reg_on), 1);
		if (ret)
		{
			printk(KERN_ERR "%s: WL_REG_ON  failed to pull up firstly when enable(%d)\n", __func__, ret);
			return -EIO;
		}   
		mdelay(10);
		ret = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(pmic_gpio_wl_reg_on), 0);
		if (ret)
		{
			printk(KERN_ERR "%s: WL_REG_ON  failed to pull down when enable(%d)\n", __func__, ret);
			return -EIO;
		}  
		mdelay(150); 
		ret = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(pmic_gpio_wl_reg_on), 1);
		if (ret)
		{
			printk(KERN_ERR "%s: WL_REG_ON  failed to pull up lastly when enable(%d)\n", __func__, ret);
			return -EIO;
		}       
		mdelay(200); 

		printk("%s: wifi power successed to pull up\n",__func__);
	}
	else
	{
		ret = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(pmic_gpio_wl_reg_on), 0); 
		if (ret) 
		{
			printk(KERN_ERR "%s:  WL_REG_ON  failed to pull down when disable(%d)\n", __func__, ret);
			return -EIO;
		}              
		mdelay(10);     

		printk("%s: wifi power successed to pull down\n",__func__);
	}

    return ret;
}


static int bcm_wifi_set_carddetect(int val)
{
    printk("%s: %d\n", __func__, val);

    bcm_detect_card(val);

    return 0;
}

static int bcm_wifi_reset(int on)
{
    printk("%s: %d\n", __func__, on);
    return 0;
}

static int reverse_byte(unsigned char *inbuf, unsigned char *outbuf )
{
    int i = 0;
    int sum = 0;

    printk("reverse_byte: in mac=%02x:%02x:%02x:%02x:%02x:%02x\n",inbuf[0],inbuf[1],inbuf[2],inbuf[3],inbuf[4],inbuf[5]);
    for(i = 0; i < WLAN_MAC_LEN; i++)
    {
        outbuf[i] = inbuf[WLAN_MAC_LEN - i -1];
        sum += outbuf[i];
    }
    return sum;
}

static void read_from_g_var(unsigned char * buf)
{
    memcpy(buf,g_wifimac,WLAN_MAC_LEN);
    printk("get mac from g_wifimac: mac=%02x:%02x:%02x:%02x:%02x:%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
    return;    
}

static int read_from_file(unsigned char * buf)
{
    struct file* filp = NULL;
    mm_segment_t old_fs;
    int result = 0;

    filp = filp_open(MAC_ADDRESS_FILE, O_CREAT|O_RDWR, 0666);
    if(IS_ERR(filp))
    {
        printk("open mac address file error\n");
        return -1;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    filp->f_pos = 0; 
    result = filp->f_op->read(filp,buf,WLAN_MAC_LEN,&filp->f_pos); 
    if(WLAN_MAC_LEN == result)
    {
        printk("get MAC from the file!\n");
        memcpy(g_wifimac,buf,WLAN_MAC_LEN);
        
        set_fs(old_fs);
        filp_close(filp,NULL);

        return 0;
    }
    //random mac
    get_random_bytes(buf,WLAN_MAC_LEN);
    buf[0] = 0x0;
    printk("get MAC from Random: mac=%02x:%02x:%02x:%02x:%02x:%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
    memcpy(g_wifimac,buf,WLAN_MAC_LEN);

    //update mac -file
    filp->f_pos = 0; 
    result = filp->f_op->write(filp,buf,WLAN_MAC_LEN,&filp->f_pos); 

    if(WLAN_MAC_LEN != result )
    {
        printk("update NV mac to file error\n");
        set_fs(old_fs);
        filp_close(filp,NULL);
        return -1;
    }

    set_fs(old_fs);
    filp_close(filp,NULL);

    return 0;
}

static int bcm_wifi_get_mac_addr(unsigned char * buf)
{
    int ret = 1;
    int sum = 0;
    struct nve_info_user  info;
    
    printk("%s\n", __func__);     
    if(NULL == buf)
    {
        printk(KERN_ERR "bcm_wifi_get_mac_addr input buf is NULL.\n");
        return -1;
    }

    memset(buf, 0, WLAN_MAC_LEN);
    memset( &info, 0, sizeof(info) );
    info.nv_read    = TEL_HUAWEI_NV_READ;
    info.nv_number  = NV_WLAN_NUM;
    strcpy( info.nv_name, "MACWLAN" );
    info.valid_size = NV_WLAN_VALID_SIZE;

    if (0 != g_wifimac[0] || 0 != g_wifimac[1] || 0 != g_wifimac[2] || 0 != g_wifimac[3]
        || 0 != g_wifimac[4] || 0 != g_wifimac[5])
    {
        read_from_g_var(buf);
        return 0;
    }

    ret = nve_direct_access( &info );
    if (0 == ret)
    {
        sum = reverse_byte(info.nv_data, buf);
        if (0 != sum)
        {
            printk("get MAC from NV: mac=%02x:%02x:%02x:%02x:%02x:%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
            memcpy(g_wifimac,buf,WLAN_MAC_LEN);
            return 0;
        }
    }

    return read_from_file(buf);
}
static struct wifi_platform_data bcm_wifi_control = {
    .mem_prealloc   = bcm_wifi_mem_prealloc,
    .set_power      = bcm_wifi_set_power,
    .set_carddetect = bcm_wifi_set_carddetect,
    .get_mac_addr   = bcm_wifi_get_mac_addr,
    .set_reset      = bcm_wifi_reset,
};

static struct resource bcm_wifi_resources[] = {
    [0] = {
              .name  = "bcm4329_wlan_irq",
              .start = -EINVAL,
              .end   = -EINVAL,
              .flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
    },
};

static struct platform_device bcm_wifi_device = {
        /* bcmdhd_wlan device */
        .name           = "bcmdhd_wlan",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(bcm_wifi_resources),
        .resource       = bcm_wifi_resources,
        .dev            = {
                .platform_data = &bcm_wifi_control,
        },
};

int __init bcm_wifi_init_gpio_mem(void)
{
#ifdef HUAWEI_BCMDHD_USE_STATIC_BUF
	int i;
#endif
	int wifi_oob_gpio_num = 0;
	int pmic_gpio_wl_reg_on = 0;	
	       
	pmic_gpio_wl_reg_on = get_pm_gpio_num_by_name("BCM_WL_REG_ON");
	if (pmic_gpio_wl_reg_on < 0)
	{
		printk(KERN_ERR "%s: init pmic gpio error: (%d)\n", __func__, pmic_gpio_wl_reg_on);
		BUG_ON(pmic_gpio_wl_reg_on < 0);
	}
	else
	{
		if (gpio_request(PM8921_GPIO_PM_TO_SYS(pmic_gpio_wl_reg_on), "WL_REG_ON"))
		{
			printk(KERN_ERR "%s: Failed to request gpio WL_REG_ON for WL_REG_ON\n", __func__);
		}
		if (gpio_direction_output(PM8921_GPIO_PM_TO_SYS(pmic_gpio_wl_reg_on), 0)) 
		{
			printk(KERN_ERR "%s: WL_REG_ON  failed to pull down \n", __func__);
		}		
	}

    wifi_oob_gpio_num = get_gpio_num_by_name("BCM_WL2MSM_WAKE");
    if(wifi_oob_gpio_num < 0)
    {
        printk(KERN_ERR "%s: can't get wifi oob gpio num (%d)\n",
                   __func__, wifi_oob_gpio_num);
    }
    else
    {
        if (gpio_request(wifi_oob_gpio_num, "wlan_wakes_msm"))  
        {
            printk(KERN_ERR "%s: Failed to request wifi oob gpio for wlan_wakes_msm\n", __func__);     
        }
        ((bcm_wifi_device.resource)[0]).start = MSM_GPIO_TO_INT(wifi_oob_gpio_num);
        ((bcm_wifi_device.resource)[0]).end = MSM_GPIO_TO_INT(wifi_oob_gpio_num);
    }

#ifdef HUAWEI_BCMDHD_USE_STATIC_BUF
    printk("%s pre alloc static buf\n", __func__);
    for(i=0;( i < WLAN_SKB_BUF_NUM );i++) {
        if (i < (WLAN_SKB_BUF_NUM/2))
            wlan_static_skb[i] = dev_alloc_skb(4096); //malloc skb 4k buffer
        else
            wlan_static_skb[i] = dev_alloc_skb(8192); //malloc skb 8k buffer
    }
    for(i=0;( i < PREALLOC_WLAN_NUMBER_OF_SECTIONS );i++) {
        wifi_mem_array[i].mem_ptr = kmalloc(wifi_mem_array[i].size,
                            GFP_KERNEL);
        if (wifi_mem_array[i].mem_ptr == NULL)
            return -ENOMEM;
    }
#endif
    return 0;
}

void __init msm8960_init_wifi(void)
{
    bcm_wifi_init_gpio_mem();
    platform_device_register(&bcm_wifi_device);
}

