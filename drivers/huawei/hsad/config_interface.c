

#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <hsad/configdata.h>
#include "hwconfig_enum.h"
#include "config_mgr.h"
#include <hsad/config_general_struct.h>

#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/debugfs.h>

#define MAX_KEY_LENGTH 48

#define MAX_SDCARD_IN_VOLTAGE_LEN  10
#define MAX_SDCARD_DRIVE_CURRENT_LEN  20
#define MAX_LCD_PANEL_NAME_LEN  40
#define MAX_CAMERA_SEQ_NAME_LEN  80
/*Add sensor self adapt*/
#define MAX_SENSOR_LAYOUT_LEN 10
/*Add for 8960 audioparameters self adapt*/
#define MAX_ACDB_PRO_NAME_LEN  20
/*Add for 8960 pa value self adapt*/
#define PA_PRO_NAME_CMP_LEN  6
/*This file is interfacer between modules/devices and config data manager.*/

/* optmize lcd self adapt function */
/* add lcd id self adapt */
/*get lcd type according lcd id*/
extern int get_lcd_id(void);
int lcd_detect_panel(const char *pstring)
{
    /*the key is "lcd/idX, X may be 0, 1, 2*/
    char *psKey = NULL;
    int id;
    int ret;
    static bool detected = false;
	char panel_name[MAX_LCD_PANEL_NAME_LEN];

    if(detected == true)
    {
        return -EBUSY;
    }
    if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))  /*maybe we should use kmalloc instead?*/
    {
        ret = -ENOMEM;
        goto err_no_mem;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);
    id = get_lcd_id();
    sprintf(psKey, "lcd/id%d", id);
    ret = get_hw_config_string(psKey, panel_name, MAX_LCD_PANEL_NAME_LEN, NULL);
    if(ret == false)
    {
        ret = -ENODEV;
        goto err_get_config;
    }
    ret = strncmp(pstring,panel_name,MAX_LCD_PANEL_NAME_LEN);
    if(ret)
    {
        ret = -ENODEV;
        goto err_detect_panel;
    }

    detected = true;
    return 0;

err_detect_panel:
err_get_config:
    kfree(psKey);
err_no_mem:
    return ret;
}
bool get_lcd_name(char *pstring)
{
    char *psKey = NULL;
    int id;
    bool ret;
    if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))  /*maybe we should use kmalloc instead?*/
    {
		 kfree(psKey);
		return false;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);
    id = get_lcd_id();
    sprintf(psKey, "lcd/id%d", id);
    ret = get_hw_config_string(psKey, pstring, MAX_LCD_PANEL_NAME_LEN, NULL);
    kfree(psKey);
   return ret;
}
int camera_detect_power_seq(const char *pstring)
{

    char *psKey = NULL;
    int ret;
    static bool detected = false;
	char camera_seq_name[MAX_CAMERA_SEQ_NAME_LEN];
    if(true ==  detected)
    {
        return -EBUSY;
    }
    if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))  
    {
        ret = -ENOMEM;
        goto err_no_mem;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);
    sprintf(psKey, "camera/powerseq");
    ret = get_hw_config_string(psKey, camera_seq_name, MAX_CAMERA_SEQ_NAME_LEN, NULL);
    if(false ==  ret)
    {
        ret = -ENODEV;
        goto err_get_config;
    }
    printk("ESEN: %s, psKey=%s \n",__func__, camera_seq_name);
    ret = strncmp(pstring,camera_seq_name,MAX_CAMERA_SEQ_NAME_LEN);
    if(ret)
    {
        ret = -ENODEV;
        goto err_detect_panel;
    }

    detected = true;
    return 0;

err_detect_panel:
err_get_config:
    kfree(psKey);
err_no_mem:
    return ret;
}

/* get audio enhance_type type */
bool audio_get_enhance_type(char* pstring, size_t count)
{
    return get_hw_config_string("audio/enhance_type", pstring, count, NULL);
}
bool get_lightsensor_type(char *pstring, int count)
{
    return get_hw_config_string("lightsensor/apds99xx", pstring, count, NULL);
}
bool get_product_type(char *pname)
{
	return get_hw_config_string("product/name", pname, MAX_KEY_LENGTH, NULL);
}

#if 0
/*get pm charge limit current*/
bool PM_get_charger_limit_current(CHARGER_LIMIT_CURRENT *pcurrent)
{
    return get_hw_config("pm/current_limt", (unsigned int*)pcurrent, NULL);
}
#endif

bool is_sdcard_in_voltage_high(void)
{
    char sdcard_in_voltage_level[MAX_SDCARD_IN_VOLTAGE_LEN];
    memset(sdcard_in_voltage_level, 0 ,sizeof(sdcard_in_voltage_level));
    get_hw_config_string("sdcard/detect_voltage_level", sdcard_in_voltage_level, MAX_SDCARD_IN_VOLTAGE_LEN, NULL);
    if(!strncmp(sdcard_in_voltage_level, "high",MAX_SDCARD_IN_VOLTAGE_LEN))
    	     	return true;
    else
    	     	return false;
}
bool get_sdcard_drive_current_val(char* item_name, unsigned int *sdcard_current)
{
    char sdcard_drive_current[MAX_SDCARD_DRIVE_CURRENT_LEN];
    memset(sdcard_drive_current, 0 ,sizeof(sdcard_drive_current));
    if(false == get_hw_config_string(item_name, 
        sdcard_drive_current, sizeof(sdcard_drive_current), NULL))
    {
        return false;
    }
    if(!strncmp(sdcard_drive_current, "GPIO_CFG_16MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_16MA;
    }
    else if(!strncmp(sdcard_drive_current, "GPIO_CFG_14MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_14MA;
    }
    else if(!strncmp(sdcard_drive_current, "GPIO_CFG_12MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_12MA;
    }
    else if(!strncmp(sdcard_drive_current, "GPIO_CFG_10MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_10MA;
    }
    else if(!strncmp(sdcard_drive_current, "GPIO_CFG_8MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_8MA;
    }
    else if(!strncmp(sdcard_drive_current, "GPIO_CFG_6MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_6MA;
    }
    else if(!strncmp(sdcard_drive_current, "GPIO_CFG_4MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_4MA;
    }
    else if(!strncmp(sdcard_drive_current, "GPIO_CFG_2MA",MAX_SDCARD_DRIVE_CURRENT_LEN))
    {
        *sdcard_current = GPIO_CFG_2MA;
    }
    else
    {
        return false;
    }

    return true;
}

/*Add TP self adapt*/
void get_tp_resolution(char *version, int count)
{
    get_hw_config_string("tp/resolution", version, count, NULL);
}
void get_touchkey_keyorder(char *version, int count)
{
    get_hw_config_string("touchkey/keyorder", version, count, NULL);
}
#ifdef CONFIG_BATTERY_BQ275x0
bool is_use_fuel_gauge(void)
{
    bool bRet = false;

    get_hw_config_bool("pm/fuel_gauge", &bRet, NULL);

    return bRet;

}
#endif
bool is_leds_ctl_pwm(void)
{
	char bl_ctl_info[MAX_KEY_LENGTH]={0};
	get_hw_config_string("leds/bl_ctl", bl_ctl_info, MAX_KEY_LENGTH, NULL);
	if(!strncmp(bl_ctl_info,"pwm",MAX_KEY_LENGTH))
		return true;
	else
		return false;
}

bool is_leds_ctl_tca6507(void)
{
	char bl_ctl_info[MAX_KEY_LENGTH]={0};
	get_hw_config_string("leds/bl_ctl", bl_ctl_info, MAX_KEY_LENGTH, NULL);
	if(!strncmp(bl_ctl_info,"tca6507",MAX_KEY_LENGTH))
		return true;
	else
		return false;
}

bool is_leds_ctl_mpp(void)
{
	char bl_ctl_info[MAX_KEY_LENGTH]={0};
	get_hw_config_string("leds/bl_ctl", bl_ctl_info, MAX_KEY_LENGTH, NULL);
	if(!strncmp(bl_ctl_info,"mpp",MAX_KEY_LENGTH))
		return true;
	else
		return false;
}

int get_leds_max_brightness(void)
{
	static int max_brightness=0;
	if(max_brightness == 0)
	{
		if(get_hw_config_int("leds/max_brightness", &max_brightness, NULL))
		{
			printk(" %s, max brightness: %d.\n",__func__, max_brightness);
		}
		else
		{
			printk(" %s, fail to get leds max brightness.\n",__func__);
			max_brightness = -1;
		}
	}
	return max_brightness;
	
}

/*Add sensor self adapt*/
bool is_gsensor_layout_top(void)
{
	char sensor_layout[MAX_SENSOR_LAYOUT_LEN];
	memset(sensor_layout, 0, sizeof(sensor_layout));
	get_hw_config_string("sensors/gsensor", sensor_layout, MAX_SENSOR_LAYOUT_LEN, NULL);
	if(!strncmp(sensor_layout,"top",MAX_SENSOR_LAYOUT_LEN))
		return true;
	else
		return false;
}
bool is_compass_sensor_layout_top(void)
{
	char sensor_layout[MAX_SENSOR_LAYOUT_LEN];
	memset(sensor_layout, 0, sizeof(sensor_layout));
	get_hw_config_string("sensors/compass", sensor_layout, MAX_SENSOR_LAYOUT_LEN, NULL);
	if(!strncmp(sensor_layout,"top",MAX_SENSOR_LAYOUT_LEN))
		return true;
	else
		return false;
}
bool is_gyro_sensor_layout_top(void)
{
	char sensor_layout[MAX_SENSOR_LAYOUT_LEN];
	memset(sensor_layout, 0, sizeof(sensor_layout));
	get_hw_config_string("sensors/gyro", sensor_layout, MAX_SENSOR_LAYOUT_LEN, NULL);
	if(!strncmp(sensor_layout,"top",MAX_SENSOR_LAYOUT_LEN))
		return true;
	else
		return false;
}
bool product_type(char *pname)
{
	 char product_name[MAX_KEY_LENGTH];
	memset(product_name, 0, sizeof(product_name));
	get_hw_config_string("product/name", product_name, MAX_SENSOR_LAYOUT_LEN, NULL);
	if(strstr(product_name,pname))
		return true;
	else
		return false;
}
/* gpio get gpio struct */
#ifdef CONFIG_HUAWEI_GPIO_UNITE
struct gpio_config_type *get_gpio_config_table(void)
{
    
	struct board_id_general_struct *gpios_ptr = get_board_id_general_struct(GPIO_MODULE_NAME);
	struct gpio_config_type *gpio_ptr;
    
	if(NULL == gpios_ptr)
	{
		HW_CONFIG_DEBUG(" can not find  module:gpio\n");
		return NULL;
	}
	
	gpio_ptr =(struct gpio_config_type *) gpios_ptr->data_array.gpio_ptr;

    if(NULL != gpio_ptr)
	{
		return gpio_ptr;
	}
	else
	{
		HW_CONFIG_DEBUG(" return NULL\n");
		return NULL;
	}
}

/*gpio get number by name*/
int get_gpio_num_by_name(char *name)
{
    int min = 0;
    int max = NR_GPIO_IRQS - 1;
    int result;
    int new_cursor;
	struct gpio_config_type *gpio_ptr = get_gpio_config_table();

    if(NULL == gpio_ptr)
    {
       HW_CONFIG_DEBUG(" get gpio struct failed.\n");
		return -EFAULT; 
    }

    while(min <= max)
    {
        new_cursor = (min+max)/2;

        if(!(strcmp((gpio_ptr+new_cursor)->name,"")))
        {
            result = 1;
        }
        else
        {
            result = strcmp((gpio_ptr+new_cursor)->name,name);  
        }   
        
        if (0 == result)
        {
            /*found it, just return*/
    		return (gpio_ptr+new_cursor)->gpio_number;
        }
        else if (result > 0)
        {
            /* key is smaller, update max*/
            max = new_cursor-1;
        }
        else if (result < 0)
        {
            /* key is bigger, update min*/
            min = new_cursor+1;
        }        
    }

    return -EFAULT;
}
struct pm_gpio_cfg_t *get_pm_gpio_config_table(void)
{  
	struct board_id_general_struct *pm_gpios_ptr = get_board_id_general_struct(PM_GPIO_MODULE_NAME);
	struct pm_gpio_cfg_t *pm_gpio_ptr;
    
	if(NULL == pm_gpios_ptr)
	{
		HW_CONFIG_DEBUG(" can not find  module:pm gpio\n");
		return NULL;
	}
	
	pm_gpio_ptr =(struct pm_gpio_cfg_t *) pm_gpios_ptr->data_array.pm_gpio_ptr;

    if(NULL != pm_gpio_ptr)
	{
		return pm_gpio_ptr;
	}
	else
	{
		HW_CONFIG_DEBUG(" return NULL\n");
		return NULL;
	}
}

int get_pm_gpio_num_by_name(char *name)
{
   	int min = 0;
    int max = PM8921_GPIO_NUM -1;
    int result;
    int new_cursor;
    struct pm_gpio_cfg_t *pm_gpio_ptr = get_pm_gpio_config_table();

    if(NULL == pm_gpio_ptr)
    {
       HW_CONFIG_DEBUG(" get pm gpio config table failed.\n");
		return -EFAULT; 
    }

    while(min <= max)
    {
        new_cursor = (min+max)/2;

        if(!(strcmp((pm_gpio_ptr+new_cursor)->name,"")))
        {
            result = 1;
        }
        else
        {
            result = strcmp((pm_gpio_ptr+new_cursor)->name,name);  
        }   
        
        if (0 == result)
        {
            /*found it, just return*/
    		return (pm_gpio_ptr+new_cursor)->gpio_number;
        }
        else if (result > 0)
        {
            /* key is smaller, update max*/
            max = new_cursor-1;
        }
        else if (result < 0)
        {
            /* key is bigger, update min*/
            min = new_cursor+1;
        }        
    }

    return -EFAULT;
}
#endif

#ifdef CONFIG_HW_POWER_TREE 
struct hw_config_power_tree *get_power_config_table(void)
{    
	struct board_id_general_struct *power_general_struct = get_board_id_general_struct(POWER_MODULE_NAME);
	struct hw_config_power_tree *power_ptr = NULL;
    
	if(NULL == power_general_struct)
	{
		HW_CONFIG_DEBUG("Can not find module:regulator\n");
		return NULL;
	}
	
	power_ptr =(struct hw_config_power_tree*)power_general_struct->data_array.power_tree_ptr;

  if(NULL == power_ptr)
	{
		HW_CONFIG_DEBUG("hw_config_power_tree return  NULL\n");
	}
	return power_ptr;
}
#endif
bool is_usbswitch_exist(void)
{   
	char *psKey = NULL;    
	bool bRet = false;    
	if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))    
	{        
			return false;   
	}    
	memset(psKey, 0, MAX_KEY_LENGTH);    
	sprintf(psKey, "usbswitch/fsa9485");        
	get_hw_config_bool(psKey, &bRet, NULL);        
	kfree(psKey);    
	return bRet;
}
bool is_mhl_exist(void)
{
    char *psKey = NULL;
    bool bRet = false;

    if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))
    {
        return false;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);
    sprintf(psKey, "mhl/sii9244");
    
    get_hw_config_bool(psKey, &bRet, NULL);
    
    kfree(psKey);

    return bRet;
}

bool is_hdmi_exist(void)
{
    char *psKey = NULL;
    bool bRet = false;

    if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))
    {
        return false;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);
    sprintf(psKey, "hdmi/msm");
    
    get_hw_config_bool(psKey, &bRet, NULL);
    
    kfree(psKey);

    return bRet;
}

/*Add for 8960 audioparameters self adapt*/
extern ssize_t simple_read_from_buffer(void __user *to, size_t count, loff_t *ppos,
				const void *from, size_t available);

int acdb_type_read(struct file *filp,char __user *buffer,size_t count, loff_t *ppos)
{
    char idarray[MAX_ACDB_PRO_NAME_LEN];
    memset(idarray, 0, MAX_ACDB_PRO_NAME_LEN);
    get_hw_config_string("product/name", idarray, MAX_ACDB_PRO_NAME_LEN, NULL);
    return simple_read_from_buffer(buffer, count, ppos,(void *) idarray, MAX_ACDB_PRO_NAME_LEN);
}

struct file_operations acdb_type_fops = {
    .read = acdb_type_read,
};

int  acdb_debugfs(void)
{
    struct dentry *acdb_debugfs_dir = NULL;
    struct dentry *acdb_file = NULL;
    acdb_file = debugfs_create_file("acdbProName", 0444, acdb_debugfs_dir, NULL, &acdb_type_fops);
    return 0;
}

bool is_felica_exist(void)
{
    char *psKey = NULL;
    bool bRet = false;

    if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))
    {
        return false;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);
    sprintf(psKey, "nfc/felica");
    
    get_hw_config_bool(psKey, &bRet, NULL);
    
    kfree(psKey);

    return bRet;
}

/*Add for 8960 pa value self adapt*/
bool get_pa_val_by_name(char *pstring)
{
        char product_name[MAX_KEY_LENGTH];
        memset(product_name, 0, sizeof(product_name));
        get_hw_config_string("product/name", product_name, MAX_KEY_LENGTH, NULL);
        if(!strncmp(product_name,pstring,PA_PRO_NAME_CMP_LEN))
                return true;
        else
                return false;
}
bool is_use_wireless_charger(void)
{   
	char *psKey = NULL;
	bool bRet = false;
	if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))
	{
			return false;
	}
	memset(psKey, 0, MAX_KEY_LENGTH);
	sprintf(psKey, "charger/wireless");
	get_hw_config_bool(psKey, &bRet, NULL);
	kfree(psKey);
	return bRet;
}

//added for using new tz end
bool is_use_new_tz(void)
{
    char *psKey = NULL;
    bool bRet = false;
               
    if(NULL == (psKey = kmalloc(MAX_KEY_LENGTH,GFP_KERNEL)))
    {
        return false;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);      
    sprintf(psKey, "tz/new_version");
            
    if(false == get_hw_config_bool(psKey, &bRet, NULL))
    {
        bRet = false;
    }
               
    kfree(psKey);
    return bRet;
}
//added for using new tz end

/* added for cradle charge begin */
bool is_use_cradle_adapt(void)
{   
	char *psKey = NULL;
	bool bRet = false;
	
	if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))
	{
		return false;
	}
	memset(psKey, 0, MAX_KEY_LENGTH);
	sprintf(psKey, "cradle/adapt");
	
	if (false == get_hw_config_bool(psKey, &bRet, NULL))
	{
		bRet = false;
	}
	
	kfree(psKey);
	return bRet;
}
/* added for cradle charge end */
