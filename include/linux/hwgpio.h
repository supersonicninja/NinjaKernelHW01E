

#include <mach/gpiomux.h>
#include  <linux/mfd/pm8xxx/gpio.h>
#define NOSET 0xff
#define PM8921_GPIO_NUM 44

struct gpio_config_type {
	char * name;
    int gpio_number;
    enum gpiomux_func init_func;
	enum gpiomux_drv  init_drv;
	enum gpiomux_pull init_pull;
	enum gpiomux_dir  init_dir;
	enum gpiomux_func sleep_func;
    enum gpiomux_drv  sleep_drv;
	enum gpiomux_pull sleep_pull;
	enum gpiomux_dir  sleep_dir; 
};

struct pm_gpio_cfg_t{
	char * name;
	int	gpio_number;
	struct pm_gpio cfg;
};

