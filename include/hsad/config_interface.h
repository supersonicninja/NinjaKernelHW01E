

#ifndef CONFIG_INTERFACE_H
#define CONFIG_INTERFACE_H

#include <linux/types.h>
#include <linux/fs.h>
/* optmize lcd self adapt function */
extern int lcd_detect_panel(const char *pstring);

extern bool audio_get_enhance_type(char* pstring, size_t count);
extern bool get_lightsensor_type(char *pstring, int count);
extern bool is_leds_ctl_pwm(void);
extern bool is_leds_ctl_tca6507(void);
extern bool is_leds_ctl_mpp(void);
int get_leds_max_brightness(void);
extern bool get_product_type(char *pname);

extern bool is_sdcard_in_voltage_high(void);

/*Add sensor self adapt*/
extern bool is_gsensor_layout_top(void);
extern bool is_gyro_sensor_layout_top(void);

extern bool is_compass_sensor_layout_top(void);
extern bool product_type(char *pname);
extern bool get_lcd_name(char *pstring);
extern bool get_sdcard_drive_current_val(char* item_name, unsigned int *sdcard_current);

/*Add TP self adapt*/
extern void get_tp_resolution(char *version, int count);
extern void get_touchkey_keyorder(char *version, int count);
#ifdef CONFIG_HUAWEI_GPIO_UNITE
extern struct gpio_config_type *get_gpio_config_table(void);
extern int get_gpio_num_by_name(char *name);
extern struct pm_gpio_cfg_t *get_pm_gpio_config_table(void);
extern int get_pm_gpio_num_by_name(char *name);
#endif

#ifdef CONFIG_BATTERY_BQ275x0
extern bool is_use_fuel_gauge(void);
#endif

extern bool is_usbswitch_exist(void);
extern bool is_mhl_exist(void);
extern bool is_hdmi_exist(void);
/*delete some unused lines*/
extern int camera_detect_power_seq(const char *pstring);
enum CAMERA_POWRER_SEQ_TYPE {
	POWER_SEQ_DCM=1,
	POWER_SEQ_SBM,
    POWER_SEQ_U9202L,
	POWER_SEQ_VRZ,
    POWER_SEQ_C8869L,
	POWER_SEQ_MAX,
};

#ifdef CONFIG_HW_POWER_TREE
extern struct hw_config_power_tree *get_power_config_table(void);
#endif

extern int acdb_type_read(struct file *filp,char __user *buffer,size_t count, loff_t *ppos);
extern int acdb_debugfs(void);

extern bool is_felica_exist(void);

extern bool get_pa_val_by_name(char *pstring);
extern bool is_use_wireless_charger(void);

extern bool get_hw_config(const char* key, char* pbuf, size_t count,  unsigned int *ptype);
extern bool get_hw_config_int(const char* key, unsigned int* pbuf, unsigned int *ptype);

extern bool is_use_new_tz(void);//added for using new tz
extern bool is_use_cradle_adapt(void);//added for cradle charge 
#endif
