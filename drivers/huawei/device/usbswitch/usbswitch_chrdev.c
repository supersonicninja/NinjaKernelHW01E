/*
* USBSWITCH_CHRDEV C File <driver>
*
* Copyright (C) 2012 Huawei Technology Inc. 
*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE.  See the
 * GNU General Public License for more details.
*/

/*========================================
* header file include
*========================================*/
#include "usbswitch_chrdev.h"
#include <linux/module.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#include <linux/gpio.h>
#include <hsad/config_interface.h>

/*========================================
* macro definition
*========================================*/
#define USWITCH_CHRDEV_NAME "uswitch_chrdev"

/*========================================
* function declaration
*========================================*/
static int usbswitch_open(struct inode *pinode, struct file *pfile);
static int usbswitch_release(struct inode *pinode, struct file *pfile);
static long usbswitch_ioctl(struct file *pfile, unsigned int uCommand, unsigned long uArgument);

/*========================================
* global data definition
*========================================*/
static const struct file_operations uswitch_fops = 
{
    .owner = THIS_MODULE,
    .open = usbswitch_open,
    .release = usbswitch_release,
    .unlocked_ioctl = usbswitch_ioctl,
};

static struct class *pUswitch_class;
static dev_t uswitch_dev;
static struct cdev cdev;
static struct device *pdevice;
static struct mutex uswitch_chrdev_mutex;

#if 0
static struct platform_device *uswitch_chr_device;
#endif

/*========================================
* function definition
*========================================*/
static int usbswitch_open(struct inode *pinode, struct file *pfile)
{
    INFO_PRINT("[cxh-kernel] usbswitch_open -E\n");

    //@add codes here

    return 0;
}

static int usbswitch_release(struct inode *pinode, struct file *pfile)
{
    INFO_PRINT("[cxh-kernel] usbswitch_release -E\n");

    //@add codes here

    return 0;
}

static long usbswitch_ioctl(struct file *pfile, unsigned int uCommand, unsigned long uArgument)
{
    int rc;
    int ndata;
    unsigned long copy_ret;

    INFO_PRINT("[cxh-kernel] usbswitch_ioctl -E; uCommand=%d\n", uCommand);

    if(uCommand == 0)
    {
        ERROR_PRINT("[cxh-kernel] usbswitch_ioctl uCommand invalid\n");
        return -EINVAL;
    }

    mutex_lock(&uswitch_chrdev_mutex);

    switch(uCommand)
    {
        case USWITCH_IOCTL_GET_CRADLE_STATUS:
            INFO_PRINT("[cxh-kernel] case USWITCH_IOCTL_GET_CRADLE_STATUS\n");
            ndata = usbswitch_is_cradle_attached();

            copy_ret = copy_to_user( &((*(int *)uArgument)), &ndata, sizeof(int));
            if(copy_ret != 0)
            {
                mutex_unlock(&uswitch_chrdev_mutex);
                ERROR_PRINT("[cxh-kernel] Error: usbswitch_ioctl copy_to_user\n");
                return -EINVAL;
            }
            break;

        default:
            break;
    }

    mutex_unlock(&uswitch_chrdev_mutex);
    
    INFO_PRINT("[cxh-kernel] usbswitch_ioctl -X\n");
    return rc;
}

static int m_cradle_gpio = 0;
int usbswitch_cradle_gpio_init(void)
{
    int rc;

    m_cradle_gpio = get_gpio_num_by_name(CRADLE_DETECT_GPIO);
    INFO_PRINT("usbswitch_get_cradle_state -E; m_cradle_gpio=%d\n", m_cradle_gpio);
    rc = gpio_request(m_cradle_gpio, CRADLE_DETECT_GPIO);
    if(rc)
    {
        gpio_free(m_cradle_gpio);
        rc = gpio_request(m_cradle_gpio, CRADLE_DETECT_GPIO);
        if(rc)
        {
            ERROR_PRINT("Error usbswitch: gpio_request\n");
            return -EIO;
        }
    }
    return 0;
}

/*
* brief: judge whether cradle attached
*
* return: 0: detached
*           1: attached
*/

int usbswitch_is_cradle_attached()
{
    int nRet = 0;
    int cradle_gpio_value;
    cradle_gpio_value = gpio_get_value(m_cradle_gpio);
    
    INFO_PRINT("usbswitch cradle_gpio_value=%d\n", cradle_gpio_value);
    if (cradle_gpio_value > 0)
    {
        //cradle detached
        INFO_PRINT("[cxh-kernel] usbswitch cradle detached\n");
        nRet = 0;
    }
    else if(cradle_gpio_value == 0)
    {
        //cradle attached
        INFO_PRINT("[cxh-kernel] usbswitch cradle attached\n");
        nRet = 1;
    }
    INFO_PRINT("usbswitch_get_cradle_state -X; nRet=%d\n", nRet);
    return nRet;
}
EXPORT_SYMBOL(usbswitch_is_cradle_attached);

static int __init usbswitch_chrdev_init_module(void)
{
    int rc;

    INFO_PRINT("usbswitch_chrdev_init_module -E\n");

    rc = alloc_chrdev_region(&uswitch_dev, 0, 1, USWITCH_CHRDEV_NAME);
    if(rc)
    {
        pr_err("Error usbswitch: alloc_chrdev_region rc=%d\n", rc);
        goto err_alloc_chrdev;
    }

    pUswitch_class = class_create(THIS_MODULE, USWITCH_CHRDEV_NAME);    
    if (IS_ERR(pUswitch_class)) 
    {
        rc = PTR_ERR(pUswitch_class);
        pr_err("Error usbswitch: class_create rc=%d\n", rc);
        goto err_create_class;
    }

    cdev_init(&cdev, &uswitch_fops);
    rc = cdev_add(&cdev, MKDEV(MAJOR(uswitch_dev), 0), 1);
    if(rc)
    {
        pr_err("Error usbswitch: cdev_add rc=%d\n", rc);
        goto err_cdev_add;
    }

    pdevice = device_create(pUswitch_class, NULL, uswitch_dev, NULL, USWITCH_CHRDEV_NAME);
    if (IS_ERR(pdevice)) 
    {
        rc = PTR_ERR(pdevice);
        pr_err("Error usbswitch: device_create rc=%d\n", rc);
        goto err_device_create;
    }
    usbswitch_cradle_gpio_init();
    mutex_init(&uswitch_chrdev_mutex);

    INFO_PRINT("usbswitch_chrdev_init_module case1 -X\n");
    return 0;

err_device_create:
    cdev_del(&cdev);
err_cdev_add:
    class_destroy(pUswitch_class);
err_create_class:
    unregister_chrdev_region(uswitch_dev, 1);
err_alloc_chrdev:
    INFO_PRINT("usbswitch_chrdev_init_module case2 -X\n");
    return rc;
}

static void __exit usbswitch_chrdev_exit_module(void)
{
    INFO_PRINT("usbswitch_chrdev_exit_module -E\n");
    device_destroy(pUswitch_class, uswitch_dev);
    cdev_del(&cdev);
    class_destroy(pUswitch_class);
    unregister_chrdev_region(uswitch_dev, 1);
    INFO_PRINT("usbswitch_chrdev_exit_module -X\n");
    
    return;
}

/*========================================
* module entrance
*========================================*/
module_init(usbswitch_chrdev_init_module);
module_exit(usbswitch_chrdev_exit_module);
MODULE_DESCRIPTION("usbswitch driver  char device ");
MODULE_LICENSE("GPL v2");

