#ifndef _LINUX_ADPS990X_H
#define _LINUX_ADPS990X_H
#define APDS990x_DRV_NAME	"apds990x"
#define DRIVER_VERSION		"1.0.4"

//#define APDS990x_INT		67

#define APDS990x_ALS_THRESHOLD_HSYTERESIS	20	/* 20 = 20% */
/* wenjuan 00182148 20111206 begin */
#define  SCALE  20
/* wenjuan 00182148 20111206 end */

/* Change History
 *
 * 1.0.1	Functions apds990x_show_rev(), apds990x_show_id() and apds990x_show_status()
 *			have missing CMD_BYTE in the i2c_smbus_read_byte_data(). APDS-990x needs
 *			CMD_BYTE for i2c write/read byte transaction.
 *
 *
 * 1.0.2	Include PS switching threshold level when interrupt occurred
 *
 *
 * 1.0.3	Implemented ISR and delay_work, correct PS threshold storing
 *
 * 1.0.4	Added Input Report Event
 */

/*
 * Defines
 */

#define APDS990x_ENABLE_REG    0x00
#define APDS990x_ATIME_REG    0x01
#define APDS990x_PTIME_REG    0x02
#define APDS990x_WTIME_REG    0x03
#define APDS990x_AILTL_REG    0x04
#define APDS990x_AILTH_REG    0x05
#define APDS990x_AIHTL_REG    0x06
#define APDS990x_AIHTH_REG    0x07
#define APDS990x_PILTL_REG    0x08
#define APDS990x_PILTH_REG    0x09
#define APDS990x_PIHTL_REG    0x0A
#define APDS990x_PIHTH_REG    0x0B
#define APDS990x_PERS_REG    0x0C
#define APDS990x_CONFIG_REG    0x0D
#define APDS990x_PPCOUNT_REG    0x0E
#define APDS990x_CONTROL_REG    0x0F
#define APDS990x_REV_REG    0x11
#define APDS990x_ID_REG        0x12
#define APDS990x_STATUS_REG    0x13
#define APDS990x_CDATAL_REG    0x14
#define APDS990x_CDATAH_REG    0x15
#define APDS990x_IRDATAL_REG    0x16
#define APDS990x_IRDATAH_REG    0x17
#define APDS990x_PDATAL_REG    0x18
#define APDS990x_PDATAH_REG    0x19

#define CMD_BYTE    0x80
#define CMD_WORD    0xA0
#define CMD_SPECIAL    0xE0

#define CMD_CLR_PS_INT    0xE5
#define CMD_CLR_ALS_INT    0xE6
#define CMD_CLR_PS_ALS_INT    0xE7

#define LSENSOR_MAX_LEVEL 7
#define LSENSOR_PNAME_LEN 20
/*
 * Structs
 */

struct apds990x_data {
    struct i2c_client *client;
    struct mutex update_lock;
	struct mutex suspend_lock;
    struct delayed_work    dwork;    /* for PS interrupt */
    struct delayed_work    als_dwork; /* for ALS polling */
    struct input_dev *input_dev_als;
    struct input_dev *input_dev_ps;

    unsigned int enable;
    unsigned int atime;
    unsigned int ptime;
    unsigned int wtime;
    unsigned int ailt;
    unsigned int aiht;
    unsigned int pilt;
    unsigned int piht;
    unsigned int pers;
    unsigned int config;
    unsigned int ppcount;
    unsigned int control;

    /* control flag from HAL */
    unsigned int enable_ps_sensor;
    unsigned int enable_als_sensor;

    /* PS parameters */
    unsigned int ps_threshold;
    unsigned int ps_hysteresis_threshold; /* always lower than ps_threshold */
    unsigned int ps_detection;        /* 0 = near-to-far; 1 = far-to-near */
    unsigned int ps_data;            /* to store PS data */

    /* ALS parameters */
    unsigned int als_threshold_l;    /* low threshold */
    unsigned int als_threshold_h;    /* high threshold */
    unsigned int als_data;            /* to store ALS data */

    unsigned int als_gain;            /* needed for Lux calculation */
    unsigned int als_poll_delay;    /* needed for light sensor polling : micro-second (us) */
    unsigned int als_atime;            /* storage for als integratiion time */
    uint16_t *als_adc_tbl_ptr;
};
struct apds990x_platform_data {
    int (*gpio_config_interrupt)(int enable);
    int (*power_on)(struct device*);
    int (*power_off)(void);
    void* (*get_adc_tbl)(int *,int *);
};

#endif
