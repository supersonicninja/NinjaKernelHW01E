
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <hsad/configdata.h>
#include "config_mgr.h"
#include <hsad/config_general_struct.h>
#include <hsad/config_interface.h>
#ifdef CONFIG_HUAWEI_GPIO_UNITE
#include <linux/irq.h>  
#include <hsad/config_debugfs.h>
#include <linux/io.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/regulator/gpio-regulator.h>
#include "../../../arch/arm/mach-msm/board-8960.h"
#endif
#ifdef CONFIG_HUAWEI_SUSPEND_GPIO
struct gpiomux_setting intsall_param[NR_GPIO_IRQS];
unsigned long save_gpio_num[NR_GPIO_IRQS]={0xff};
#endif

struct dentry *boardid_debugfs_root;
#ifdef CONFIG_HUAWEI_GPIO_UNITE
struct dentry *msmgpio_debugfs_root;
struct dentry *pmgpio_debugfs_root;
extern unsigned gpio_func_config_read(unsigned gpio);
extern unsigned gpio_in_out_config_read(unsigned gpio);
#endif

#define DEBUGFS_ROOT_PATH "sys/kernel/debug/boardid/common"
#define DEBUGFS_PATH_MAX_LENGTH 128

struct dentry *boardid_common_root;

struct dir_node {
	struct dentry *dentry;
	struct list_head list;
	char  path[DEBUGFS_PATH_MAX_LENGTH];
};

/* A list used for store dentry struct while creating the debugfs tree */
static struct list_head g_dentry_list;

static ssize_t config_common_read(struct file *filp, char __user *buffer,size_t count, loff_t *ppos);

static const struct file_operations config_common_fops = {
        .read = config_common_read,
};

/*
 * Description: try to find a dentry from g_dentry_list, use the path as a match key.
 * Return Value: 0 for failure, and return a pointer to a dentry structure if success.
 * */
static struct dentry * find_dentry_from_path(char* path)
{
	struct list_head *pos;
	struct dir_node *p;

	list_for_each(pos, &g_dentry_list)
	{
		p = container_of(pos, struct dir_node, list);

		if (!strcmp(path,p->path))
		{
			return p->dentry;
		}
	}

	return NULL;
}

/*
 * Description: Allocate a dir node for pdentry and add it to g_dentry_list, with
 *              the path as a find-use key.
 * */
static int add_dentry_to_list(struct dentry *pdentry, char* path)
{
        struct dir_node *pNode = kmalloc(sizeof(struct dir_node), GFP_KERNEL);

	if (pNode)
	{
		pNode->dentry = pdentry;
		strcpy(pNode->path, path);
		list_add(&(pNode->list), &g_dentry_list);
		return 0;
	}
	else
		return -ENOMEM;
}
/*
 * Description: This function destroys the g_dentry_list, including
 *              including free the memory allocated before.
 * */
static int destroy_dentry_list(void)
{
        struct list_head *pos, *next;
	struct dir_node *p;
	list_for_each_safe(pos, next, &g_dentry_list)
	{
		p = container_of(pos, struct dir_node, list);
		list_del(pos);
		kfree(p);
	}
	return 0;
}

/* This function create debugfs for  a config key */
static int generate_common_debugfs_line(struct dentry *root, const char* key)
{
	char* buf, *fullpath, *b;
	struct dentry *parent = root;
	int ret = 0;

	buf = kmalloc(sizeof(char)*DEBUGFS_PATH_MAX_LENGTH, GFP_KERNEL);
	if (!buf)
	{
		ret = -ENOMEM;
		goto fail;
	}
	fullpath =kmalloc(sizeof(char)*DEBUGFS_PATH_MAX_LENGTH, GFP_KERNEL);
	if (!fullpath)
	{
		ret = -ENOMEM;
		goto fail;
	}

	/* copy buffer for later use */
	strlcpy(buf, key, sizeof(char)*DEBUGFS_PATH_MAX_LENGTH);

	/* copy buff for fullpath, which contains a file's the absolute path*/
	strlcpy(fullpath, DEBUGFS_ROOT_PATH, sizeof(char)*DEBUGFS_PATH_MAX_LENGTH);

	b = strim(buf);

	while(b)
	{
		char* name = strsep(&b, "/");
		if (!name)
			continue;
		if (b)
		{
			/* the name represents a directory. */
			struct dentry* pdentry;
			strcat(fullpath, "/");
			strcat(fullpath, name);
			pdentry = find_dentry_from_path(fullpath);
			if (!pdentry)
			{
				/* the directory is not created before. */
				parent = debugfs_create_dir(name, parent);
				add_dentry_to_list(parent,fullpath);
			}
			else
			{
				/* the directory is created before, just parse next name */
				parent = pdentry;
			}
		}
		else
		{
			/* the name represents a file */
			debugfs_create_file(name, 0444, parent, NULL, &config_common_fops);
		}
	}

fail:
	if(buf)
		kfree(buf);
	if(fullpath)
		kfree(fullpath);
	return ret;
}

static int generate_common_debugfs_tree(struct dentry *root)
{
        struct board_id_general_struct *config_pairs_ptr;
	config_pair* pconfig;

	config_pairs_ptr = get_board_id_general_struct(COMMON_MODULE_NAME);

	if(NULL == config_pairs_ptr)
	{
		return -ENOENT;
	}

	INIT_LIST_HEAD(&g_dentry_list);

	pconfig = (config_pair*)config_pairs_ptr->data_array.config_pair_ptr;
	while(NULL != pconfig->key )
	{
		/* create every key in debugfs now. */
		int ret = generate_common_debugfs_line(root, pconfig->key);
		if (0 != ret)
			return ret;
		pconfig++;
	}

	return 0;
}

static ssize_t config_boardid_read(struct file *filp,  char  __user *buffer,size_t count, loff_t *ppos)
{

	int len = 0;
	char idarray[32];
	memset(idarray, 0, 32);

	len = sprintf(idarray, "board id: 0x%02x\n", g_current_board_id);

	return simple_read_from_buffer(buffer, count, ppos,(void *) idarray, len);

}


static ssize_t config_common_read(struct file *filp, char __user *buffer,size_t count, loff_t *ppos)
{
	struct board_id_general_struct *config_pairs_ptr;
	config_pair* pconfig;
	int len = 0;
	int ret = 0;
	char *ptr      = NULL;
	char *fullpath = NULL;
	char *pkey     = NULL;
	char *ppath    = NULL;

	config_pairs_ptr = get_board_id_general_struct(COMMON_MODULE_NAME);

	fullpath = (char*)kmalloc((DEBUGFS_PATH_MAX_LENGTH*sizeof(char)), GFP_KERNEL);
	if (!fullpath)
	{
		ret = -ENOMEM;
		goto fail;
	}

	ptr = (char*)kmalloc((PAGE_SIZE*sizeof(char)),GFP_KERNEL);
	if(!ptr)
	{
		ret = -ENOMEM;
		goto fail;
	}

	memset(ptr,0,PAGE_SIZE);
	memset(fullpath, 0, sizeof(char)*DEBUGFS_PATH_MAX_LENGTH);

	ppath = d_path((const struct path*)&filp->f_path, fullpath, sizeof(char)*DEBUGFS_PATH_MAX_LENGTH);

	if(NULL == config_pairs_ptr)
	{
		HW_CONFIG_DEBUG(" can not find  module:common\n");
		len = snprintf(ptr,PAGE_SIZE," can not find  module:common\n");
		len = simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	pkey = strstr(ppath, DEBUGFS_ROOT_PATH"/");
	if (pkey)
	{
		pkey += strlen(DEBUGFS_ROOT_PATH"/");
	}

	pconfig = (config_pair*)config_pairs_ptr->data_array.config_pair_ptr;
	while(NULL != pconfig->key )
	{
	        if (!strcmp(pkey, pconfig->key))
		{
			if(E_CONFIG_DATA_TYPE_STRING == pconfig->type)
			{
				len = snprintf(ptr,PAGE_SIZE,"string:%s\n",(char*)pconfig->data);
			}
			else if(E_CONFIG_DATA_TYPE_INT == pconfig->type)
			{
				len = snprintf(ptr,PAGE_SIZE,"int:%d\n",pconfig->data);
			}
			else if(E_CONFIG_DATA_TYPE_BOOL == pconfig->type)
			{
				len = snprintf(ptr,PAGE_SIZE,"bool:%s\n",pconfig->data?"yes":"no");
			}
			else if(E_CONFIG_DATA_TYPE_ENUM == pconfig->type)
			{
				len = snprintf(ptr,PAGE_SIZE,"enum:%d\n",pconfig->data);
			}
			else
			{
				len = snprintf(ptr,PAGE_SIZE,"unknow type:%d\n",pconfig->data);
			}
			*(ptr+len) = 0;
			break;
		}
		pconfig++;
	}

	ret = simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);

fail:
	if(fullpath)
	{
		kfree(fullpath);
	}
	if(ptr)
	{
		kfree(ptr);
	}
	return ret;
}

#ifdef CONFIG_HUAWEI_GPIO_UNITE
void convert_gpio_config_to_string(struct gpiomux_setting* cfg,struct gpio_config_str* config_str)
{
	switch(cfg->func)
	{
		case 0: {config_str->func = "GPIO"; break;}
		case 1: {config_str->func = "FUN_1";break;}
		case 2: {config_str->func = "FUN_2";break;}
		case 3: {config_str->func = "FUN_3";break;}
		case 4: {config_str->func = "FUN_4";break;}
		case 5: {config_str->func = "FUN_5";break;}
		case 6: {config_str->func = "FUN_6";break;}
		case 7: {config_str->func = "FUN_7";break;}
		case 8: {config_str->func = "FUN_8";break;}
		case 9: {config_str->func = "FUN_9";break;}
		case 10: {config_str->func = "FUN_A";break;}
		case 11: {config_str->func = "FUN_B";break;}
		case 12: {config_str->func = "FUN_C";break;}
		case 13: {config_str->func = "FUN_D";break;}
		case 14: {config_str->func = "FUN_E";break;}
		case 15: {config_str->func = "FUN_F";break;}
		case NOSET: {config_str->func = "noset";break;}
		default:config_str->func = "error";
	}

	switch(cfg->drv)
	{
		case 0:{config_str->drv = "2ma";break;}
		case 1:{config_str->drv = "4ma";break;}
		case 2:{config_str->drv = "6ma";break;}
		case 3:{config_str->drv = "8ma";break;}
		case 4:{config_str->drv = "10ma";break;}
		case 5:{config_str->drv = "12ma";break;}
		case 6:{config_str->drv = "14ma";break;}
		case 7:{config_str->drv = "16ma";break;}
		case NOSET:{config_str->drv = "noset";break;}
		default:config_str->drv = "error";
	}

	switch(cfg->pull)
	{
		case 0:{config_str->pull = "nopull";break;}
		case 1:{config_str->pull = "pulldown";break;}
		case 2:{config_str->pull = "keeper";break;}
		case 3:{config_str->pull = "pullup";break;}
		case NOSET:{config_str->pull = "noset";break;}
		default:config_str->pull = "error";
	}

	switch(cfg->dir)
	{
		case 0:{config_str->dir = "in";break;}
		case 1:{config_str->dir = "out H";break;}
		case 2:{config_str->dir = "out L";break;}
		case NOSET:{config_str->dir = "noset";break;}
		default:config_str->dir = "error";
	}
}

void convert_pm_gpio_config_to_string(struct pm_gpio param , struct pm_gpio_str * cfg)
{
	switch(param.function)
	{
		case 0: {cfg->function = "gpio"; break;}
		case 1: {cfg->function = "paired";break;}
		case 2: {cfg->function = "fun_1";break;}
		case 3: {cfg->function = "fun_2";break;}
		case 4: {cfg->function = "dtest1";break;}
		case 5: {cfg->function = "dtest2";break;}
		case 6: {cfg->function = "dtest3";break;}
		case 7: {cfg->function = "dtest4";break;}
		case NOSET: {cfg->function = "noset";break;}
		default:cfg->function = "error";
	}

	switch(param.out_strength)
	{
		case 0:{cfg->out_strength = "no";break;}
		case 1:{cfg->out_strength = "high";break;}
		case 2:{cfg->out_strength = "med";break;}
		case 3:{cfg->out_strength = "low";break;}
		case NOSET:{cfg->out_strength = "noset";break;}
		default:cfg->out_strength = "error";
	}

	switch(param.pull)
	{
		case 0:{cfg->pull = "pu_30";break;}
		case 1:{cfg->pull = "pu_1.5";break;}
		case 2:{cfg->pull = "pu_31.5";break;}
		case 3:{cfg->pull = "pu_1.5_30";break;}
		case 4:{cfg->pull = "pulldown";break;}
		case 5:{cfg->pull = "nopull";break;}
		case NOSET:{cfg->pull = "noset";break;}
		default:cfg->pull = "error";
	}

	switch(param.vin_sel)
	{
		case 0:{cfg->vin_sel = "VPH";break;}
		case 1:{cfg->vin_sel = "BB";break;}
		case 2:{cfg->vin_sel = "S4";break;}
		case 3:{cfg->vin_sel = "L15";break;}
		case 4:{cfg->vin_sel = "L4";break;}
		case 5:{cfg->vin_sel = "L3";break;}
		case 6:{cfg->vin_sel = "L17";break;}
		case NOSET:{cfg->vin_sel = "noset";break;}
		default:cfg->vin_sel = "error";
	}

	switch(param.direction)
	{
		case 1:{cfg->direction = "out";break;}
		case 2:{cfg->direction = "in";break;}
		case 3:{cfg->direction = "both";break;}
		case NOSET:{cfg->direction = "noset";break;}
		default:cfg->direction = "error";
	}

	switch(param.output_buffer)
	{
		case 0: {cfg->output_buffer = "cmos"; break;}
		case 1: {cfg->output_buffer = "open drain";break;}
		case NOSET: {cfg->output_buffer = "noset";break;}
		default:cfg->output_buffer = "error";
	}

	switch(param.output_value)
	{
		case 0: {cfg->output_value = "low"; break;}
		case 1: {cfg->output_value = "high";break;}
		case NOSET: {cfg->output_value = "noset";break;}
		default:cfg->output_value = "error";
	}

	switch(param.inv_int_pol)
	{
		case 0:{cfg->inv_int_pol = "disable";break;}
		case 1:{cfg->inv_int_pol = "enable";break;}
		case NOSET:{cfg->inv_int_pol = "noset";break;}
		default:cfg->inv_int_pol = "error";
	}

	switch(param.disable_pin)
	{
		case 0:{cfg->disable_pin = "enable";break;}
		case 1:{cfg->disable_pin = "disable";break;}
		case NOSET:{cfg->disable_pin = "noset";break;}
		default:cfg->disable_pin = "error";
	}
}

static ssize_t msm_gpio_init_cfg_read(struct file *filp,  char  __user *buffer,size_t count, loff_t *ppos)
{
	int i;
	char *ptr, *arr_ptr_tmp;
	int len = 0,cnt = 0;
	int gpio_size = 4*PAGE_SIZE;
	struct gpiomux_setting active_cfg;
	struct gpiomux_setting suspend_cfg;
	struct gpio_config_str *active_cfg_str;
	struct gpio_config_str *suspend_cfg_str;
	struct gpio_config_type *gpio_ptr = get_gpio_config_table();

	ptr = (char*)kmalloc(( gpio_size*sizeof(char)) ,GFP_KERNEL);

	if(!ptr)
		return -ENOMEM;

	memset( ptr, 0, gpio_size);

	arr_ptr_tmp = ptr;

	if( NULL == gpio_ptr )
	{
		HW_CONFIG_DEBUG(" can not find  module:gpio\n");
		len = snprintf(ptr,gpio_size," can not find  module:gpio\n");
		len=simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	active_cfg_str = kmalloc(sizeof(*active_cfg_str) ,GFP_KERNEL);

	if(!active_cfg_str)
	{
		HW_CONFIG_DEBUG(" msm gpio arr malloc failed\n");
		len = snprintf(ptr,gpio_size," msm gpio arr malloc failed\n");
		len=simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	suspend_cfg_str = kmalloc(sizeof(*suspend_cfg_str) ,GFP_KERNEL);

	if(!suspend_cfg_str)
	{
		HW_CONFIG_DEBUG(" msm gpio arr malloc failed\n");
		len = snprintf(ptr,gpio_size," msm gpio arr malloc failed\n");
		len=simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		kfree(active_cfg_str);
		return len;
	}

	len = snprintf(ptr, gpio_size, "%-24s%-27s%-27s\n",   \
				"=================","======active config======","========suspend config========");
	ptr += len;
	cnt += len;
	len = snprintf(ptr, gpio_size - cnt, "%-4s%-20s%-6s%-6s%-9s%-6s%-6s%-6s%-9s%-6s\n",   \
				"num","name","func","drv","pull","dir","func","drv","pull","dir");
	ptr += len;
	cnt += len;

	for(i = 0; i < NR_GPIO_IRQS; i++)
	{
		active_cfg.func = (gpio_ptr + i)->init_func;
		active_cfg.drv = (gpio_ptr + i)->init_drv;
		active_cfg.pull = (gpio_ptr + i)->init_pull;
		active_cfg.dir = (gpio_ptr + i)->init_dir;
		suspend_cfg.func = (gpio_ptr + i)->sleep_func;
		suspend_cfg.drv = (gpio_ptr + i)->sleep_drv;
		suspend_cfg.pull = (gpio_ptr + i)->sleep_pull;
		suspend_cfg.dir = (gpio_ptr + i)->sleep_dir;

		if( gpio_size - cnt <= 0)
		{
			HW_CONFIG_DEBUG("max size over one page");
			printk("max size over one page\n");
			break;
		}

		convert_gpio_config_to_string(&active_cfg,active_cfg_str);
		convert_gpio_config_to_string(&suspend_cfg,suspend_cfg_str);

		len = snprintf(ptr, gpio_size - cnt, "%-4d%-20s%-6s%-6s%-9s%-6s%-6s%-6s%-9s%-6s\n",   \
					(gpio_ptr + i)->gpio_number,   \
					(gpio_ptr + i)->name,            \
					active_cfg_str->func,         \
					active_cfg_str->drv,          \
					active_cfg_str->pull,         \
					active_cfg_str->dir,           \
					suspend_cfg_str->func,      \
					suspend_cfg_str->drv,        \
					suspend_cfg_str->pull,       \
					suspend_cfg_str->dir);

		ptr   += len;
		cnt += len;
	}

	len = snprintf(ptr, gpio_size - cnt, "\n");
	ptr += len;
	cnt += len;

	*ptr = '\0';

	len = strlen(arr_ptr_tmp);
	len = simple_read_from_buffer(buffer, count, ppos, (void *) arr_ptr_tmp, len);
	kfree(arr_ptr_tmp);
	kfree(active_cfg_str);
	kfree(suspend_cfg_str);
	return len;
}

static ssize_t pm_gpio_init_cfg_read(struct file *filp,  char  __user *buffer,size_t count, loff_t *ppos)
{
	int i;
	char *ptr, *arr_ptr_tmp;
	int len = 0,cnt = 0;
	struct pm_gpio_str * pm_cfg_ptr;
	struct pm_gpio_cfg_t *pm_gpio_ptr = get_pm_gpio_config_table();

	ptr = (char*)kmalloc(( PAGE_SIZE*sizeof(char)) ,GFP_KERNEL);

	if(!ptr)
		return -ENOMEM;

	memset( ptr, 0, PAGE_SIZE);

	arr_ptr_tmp = ptr;

	if( NULL == pm_gpio_ptr )
	{
		HW_CONFIG_DEBUG(" can not find  module:gpio\n");
		len = snprintf(ptr,PAGE_SIZE," can not find  module:gpio\n");
		len=simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	pm_cfg_ptr = kmalloc(sizeof(*pm_cfg_ptr) ,GFP_KERNEL);

	if(!pm_cfg_ptr)
	{
		HW_CONFIG_DEBUG(" pm gpio arr malloc failed\n");
		len = snprintf(ptr,PAGE_SIZE," pm gpio arr malloc failed\n");
		len=simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	len = snprintf(ptr, PAGE_SIZE, "%-4s%-15s%-6s%-6s%-9s%-4s%-4s%-8s%-8s%-8s%-6s\n",   \
				"num","name","func","drv","pull","vin","dir","out_buf","out_val","inv_pol","disable");
	ptr += len;
	cnt += len;

	for(i = 0; i < PM8921_GPIO_NUM; i++)
	{
		if( PAGE_SIZE - cnt <= 0)
		{
			HW_CONFIG_DEBUG("max size over one page");
			break;
		}
	
		convert_pm_gpio_config_to_string(pm_gpio_ptr[i].cfg,pm_cfg_ptr);
	
		len = snprintf(ptr, PAGE_SIZE - cnt, "%-4d%-15s%-6s%-6s%-9s%-6s%-6s%-6s%-6s%-9s%-6s\n",   \
					(pm_gpio_ptr + i)->gpio_number,   \
					(pm_gpio_ptr + i)->name,            \
					pm_cfg_ptr->function,         \
					pm_cfg_ptr->out_strength,          \
					pm_cfg_ptr->pull,         \
					pm_cfg_ptr->vin_sel,         \
					pm_cfg_ptr->direction,           \
					pm_cfg_ptr->output_buffer,      \
					pm_cfg_ptr->output_value,        \
					pm_cfg_ptr->inv_int_pol,       \
					pm_cfg_ptr->disable_pin);

		ptr   += len;
		cnt += len;
	}

	len = snprintf(ptr, PAGE_SIZE - cnt, "\n");
	ptr += len;
	cnt += len;

	*ptr = '\0';

	len = strlen(arr_ptr_tmp);
	len = simple_read_from_buffer(buffer, count, ppos, (void *) arr_ptr_tmp, len);
	kfree(arr_ptr_tmp);
	kfree(pm_cfg_ptr);
	return len;
}

static ssize_t msm_gpio_curr_cfg_read(struct file *filp,  char  __user *buffer,size_t count, loff_t *ppos)
{
	int i,config_value,inout_value;
	char *ptr, *arr_ptr_tmp;
	int len = 0,cnt = 0;
	int gpio_size = 4*PAGE_SIZE;
	struct gpiomux_setting cfg;
	struct gpio_config_str *cfg_str;

	ptr = (char*)kmalloc(( gpio_size*sizeof(char)) ,GFP_KERNEL);

	if(!ptr)
		return -ENOMEM;

	memset( ptr, 0, gpio_size);

	arr_ptr_tmp = ptr;

	cfg_str = kmalloc(sizeof(*cfg_str) ,GFP_KERNEL);

	if(!cfg_str)
	{
		HW_CONFIG_DEBUG(" msm gpio arr malloc failed\n");
		len = snprintf(ptr,gpio_size," msm gpio arr malloc failed\n");
		len=simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	ptr += len;
	cnt += len;
	len = snprintf(ptr, gpio_size - cnt, "%-4s%-6s%-6s%-9s%-6s\n",   \
				"num","func","drv","pull","dir");
	ptr += len;
	cnt += len;

	for(i = 0; i < NR_GPIO_IRQS; i++)
	{
		if( gpio_size - cnt <= 0)
		{
			HW_CONFIG_DEBUG("max size over one page");
			break;
		}

		config_value = gpio_func_config_read(i);
		inout_value = gpio_in_out_config_read(i);
		inout_value = (inout_value >> 1) & 0x01;

		cfg.func = (config_value >> 2) & 0x0f;
		cfg.drv = (config_value >> 6) & 0x07;
		cfg.pull = config_value & 0x03;
		cfg.dir = (config_value >> 9) & 0x01;

		if(cfg.dir)
		{
			if(inout_value)
			{
				cfg.dir = GPIOMUX_OUT_HIGH;
			}
			else
			{
				cfg.dir = GPIOMUX_OUT_LOW;
			}
		}
		else
		{
			cfg.dir = GPIOMUX_IN;
		}

		convert_gpio_config_to_string(&cfg,cfg_str);

		len = snprintf(ptr, gpio_size - cnt, "%-4d%-6s%-6s%-9s%-6s\n",   \
					i,   \
					cfg_str->func,         \
					cfg_str->drv,          \
					cfg_str->pull,         \
					cfg_str->dir);

		ptr   += len;
		cnt += len;

	}

	len = snprintf(ptr, gpio_size - cnt, "\n");
	ptr += len;
	cnt += len;

	*ptr = '\0';

	len = strlen(arr_ptr_tmp);
	len = simple_read_from_buffer(buffer, count, ppos, (void *) arr_ptr_tmp, len);
	kfree(arr_ptr_tmp);
	kfree(cfg_str);
	return len;

}

static int input_param_check(char *buf, long int *param, int num_of_par)
{
	char *token;
	int cnt;
	int num_check_flag = 0;
	int param_check_flag = 0;

	token = strsep(&buf, " ");

	for (cnt = 0; cnt < num_of_par; cnt++)
	{
		if (token != NULL)
		{
			if (strict_strtoul(token, 10, &param[cnt]) != 0)
			{
				return -EINVAL;
			}

			token = strsep(&buf, " ");
		}
		else
		{
			num_check_flag = -EFAULT;
			break;
		}
	}

	if(!num_check_flag)
	{
		if(param[0] < 0 || param[0] > NR_GPIO_IRQS)
			param_check_flag = -EFAULT;

		if(param[1] < GPIOMUX_FUNC_GPIO || param[1] > GPIOMUX_FUNC_F)
			param_check_flag = -EFAULT;

		if(param[2] < GPIOMUX_DRV_2MA || param[2] > GPIOMUX_DRV_16MA)
			param_check_flag = -EFAULT;

		if(param[3] < GPIOMUX_PULL_NONE || param[3] > GPIOMUX_PULL_UP)
			param_check_flag = -EFAULT;

		if(param[4] < GPIOMUX_IN || param[4] > GPIOMUX_OUT_LOW)
			param_check_flag = -EFAULT;

	}
	else
	{
		return num_check_flag;
	}

	if(param_check_flag)
	{
		return param_check_flag;
	}

	return 0;
}

#ifdef CONFIG_HUAWEI_SUSPEND_GPIO
static ssize_t msm_gpio_sleep_cfg_read(struct file *filp,  char  __user *buffer,size_t count, loff_t *ppos)
{
	int i,config_value,inout_value;
	char *ptr, *arr_ptr_tmp;
	int len = 0,cnt = 0;
	int gpio_size = 4*PAGE_SIZE;
	struct gpiomux_setting cfg;
	struct gpio_config_str *cfg_str;

	ptr = (char*)kmalloc(( gpio_size*sizeof(char)) ,GFP_KERNEL);

	if(!ptr)
		return -ENOMEM;

	memset( ptr, 0, gpio_size);

	arr_ptr_tmp = ptr;

	cfg_str = kmalloc(sizeof(*cfg_str) ,GFP_KERNEL);

	if(!cfg_str)
	{
		HW_CONFIG_DEBUG(" msm gpio arr malloc failed\n");
		len = snprintf(ptr,gpio_size," msm gpio arr malloc failed\n");
		len=simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	ptr += len;
	cnt += len;
	len = snprintf(ptr, gpio_size - cnt, "%-4s%-6s%-6s%-9s%-6s\n",   \
				"num","func","drv","pull","dir");
	ptr += len;
	cnt += len;

	for(i = 0; i < NR_GPIO_IRQS; i++)
	{
		if( gpio_size - cnt <= 0)
		{
			HW_CONFIG_DEBUG("max size over one page");
			break;
		}

		config_value = gpio_func_config_read(i);
		inout_value = gpio_in_out_config_read(i);
		inout_value = (inout_value >> 1) & 0x01;

		cfg.func = (config_value >> 2) & 0x0f;
		cfg.drv = (config_value >> 6) & 0x07;
		cfg.pull = config_value & 0x03;
		cfg.dir = (config_value >> 9) & 0x01;

		if(cfg.dir)
		{
			if(inout_value)
			{
				cfg.dir = GPIOMUX_OUT_HIGH;
			}
			else
			{
				cfg.dir = GPIOMUX_OUT_LOW;
			}
		}
		else
		{
			cfg.dir = GPIOMUX_IN;
		}

		convert_gpio_config_to_string(&cfg,cfg_str);

		len = snprintf(ptr, gpio_size - cnt, "%-4d%-6s%-6s%-9s%-6s\n",   \
					i,   \
					cfg_str->func,         \
					cfg_str->drv,          \
					cfg_str->pull,         \
					cfg_str->dir);

		ptr   += len;
		cnt += len;

	}

	len = snprintf(ptr, gpio_size - cnt, "\n");
	ptr += len;
	cnt += len;

	*ptr = '\0';

	len = strlen(arr_ptr_tmp);
	len = simple_read_from_buffer(buffer, count, ppos, (void *) arr_ptr_tmp, len);
	kfree(arr_ptr_tmp);
	kfree(cfg_str);
	return len;
}

void suspend_gpio_write_all(void)
{
	int i;
	
	for(i=0;i<NR_GPIO_IRQS;i++)
	{
	if(save_gpio_num[i] == i)
		{
		__msm_gpiomux_write(save_gpio_num[i],intsall_param[i]);
		printk("suspend_gpio_write_all	gpio config num is %d\n", i);
		}
	}
}
void suspend_gpio_read_all_have_write_in(void)
{
	int i,config_value,inout_value;
	for(i=0;i<NR_GPIO_IRQS;i++)
	{
		if(save_gpio_num[i] == i)
		{
		config_value = gpio_func_config_read(save_gpio_num[i]);
		inout_value = gpio_in_out_config_read(save_gpio_num[i]);
		inout_value = (inout_value >> 1) & 0x01;

		intsall_param[i].func = (config_value >> 2) & 0x0f;
		intsall_param[i].drv = (config_value >> 6) & 0x07;
		intsall_param[i].pull = config_value & 0x03;
		intsall_param[i].dir = (config_value >> 9) & 0x01;

		if(intsall_param[i].dir)
		{
			if(inout_value)
			{
				intsall_param[i].dir = GPIOMUX_OUT_HIGH;
			}
			else
			{
				intsall_param[i].dir = GPIOMUX_OUT_LOW;
			}
		}
		else
		{
			intsall_param[i].dir = GPIOMUX_IN;
		}

		}
       }

}

static ssize_t msm_gpio_sleep_cfg_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char lbuf[32];
	int rc;
	unsigned long param[5];
	u8 gpio_num;

	if (cnt > sizeof(lbuf) - 1)
	{
		return -EINVAL;
	}

	rc = copy_from_user(lbuf, ubuf, cnt);
	if (rc)
	{
		return -EFAULT;
	}

	lbuf[cnt] = '\0';

	rc = input_param_check(lbuf, param, 5);
	if (!rc)
	{
		gpio_num = param[0];
		intsall_param[gpio_num].func =  param[1];
		intsall_param[gpio_num].drv = param[2];
		intsall_param[gpio_num].pull = param[3];
		intsall_param[gpio_num].dir  = param[4];
		save_gpio_num[gpio_num]=param[0];
	}
	else
	{
		pr_err("%s: Error, invalid parameters\n", __func__);
		rc = -EINVAL;
		goto gpio_config_error;
	}

gpio_config_error:
	if (rc == 0)
		rc = cnt;
	else
		pr_err("%s: rc = %d\n", __func__, rc);

	return rc;
}
#endif

static ssize_t msm_gpio_curr_cfg_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char lbuf[32];
	int rc;
	unsigned long param[5];
	struct gpiomux_setting val;

	if (cnt > sizeof(lbuf) - 1)
	{
		return -EINVAL;
	}

	rc = copy_from_user(lbuf, ubuf, cnt);
	if (rc)
	{
		return -EFAULT;
	}

	lbuf[cnt] = '\0';

	rc = input_param_check(lbuf, param, 5);
	if (!rc)
	{
		val.func = param[1];
		val.drv = param[2];
		val.pull = param[3];
		val.dir = param[4];

		__msm_gpiomux_write(param[0],val);
	}
	else
	{
		pr_err("%s: Error, invalid parameters\n", __func__);
		rc = -EINVAL;
		goto gpio_config_error;
	}

gpio_config_error:
	if (rc == 0)
		rc = cnt;
	else
		pr_err("%s: rc = %d\n", __func__, rc);

	return rc;
}
#endif

static const struct file_operations config_boardid_fops = {
        .read = config_boardid_read,
};

#ifdef CONFIG_HUAWEI_GPIO_UNITE
static const struct file_operations msm_gpio_init_cfg_fops = {
        .read = msm_gpio_init_cfg_read,
};

static const struct file_operations msm_gpio_curr_cfg_fops = {
        .read = msm_gpio_curr_cfg_read,
	 .write = msm_gpio_curr_cfg_write,

};

static const struct file_operations pm_gpio_init_cfg_fops = {
        .read = pm_gpio_init_cfg_read,
};

#endif

#ifdef CONFIG_HUAWEI_SUSPEND_GPIO
static const struct file_operations msm_gpio_sleep_cfg_fops = {
        .read = msm_gpio_sleep_cfg_read,
	 .write = msm_gpio_sleep_cfg_write,
};
#endif

int  config_debugfs_init(void)
{
	struct dentry *boardid_file;

#ifdef CONFIG_HUAWEI_GPIO_UNITE
	struct dentry *msm_gpio_init_file,*msm_gpio_curr_file,*pm_gpio_init_file;
#endif
#ifdef CONFIG_HUAWEI_SUSPEND_GPIO
	struct dentry *msm_gpio_sleep_file;
#endif
	boardid_debugfs_root = debugfs_create_dir("boardid", NULL);
	if(!boardid_debugfs_root)
		return -ENOENT;

	boardid_common_root = debugfs_create_dir("common", boardid_debugfs_root);
	if (!boardid_common_root)
		return -ENOENT;

#ifdef CONFIG_HUAWEI_GPIO_UNITE
	msmgpio_debugfs_root = debugfs_create_dir("msmgpio", boardid_debugfs_root);
	if(!msmgpio_debugfs_root)
		return -ENOENT;

	pmgpio_debugfs_root = debugfs_create_dir("pmgpio", boardid_debugfs_root);
	if(!pmgpio_debugfs_root)
		return -ENOENT;
#endif

	boardid_file = debugfs_create_file("id", 0444, boardid_debugfs_root, NULL, &config_boardid_fops);

	generate_common_debugfs_tree(boardid_common_root);

#ifdef CONFIG_HUAWEI_GPIO_UNITE
	msm_gpio_init_file = debugfs_create_file("init_config",0444,msmgpio_debugfs_root,NULL,&msm_gpio_init_cfg_fops);

	msm_gpio_curr_file = debugfs_create_file("curr_config",S_IFREG | S_IWUSR|S_IWGRP ,msmgpio_debugfs_root, (void *) "curr_config",&msm_gpio_curr_cfg_fops);

	pm_gpio_init_file = debugfs_create_file("init_config",0444,pmgpio_debugfs_root,NULL,&pm_gpio_init_cfg_fops);
#endif
#ifdef CONFIG_HUAWEI_SUSPEND_GPIO
	msm_gpio_sleep_file = debugfs_create_file("sleep_config",S_IFREG | S_IWUSR|S_IWGRP ,msmgpio_debugfs_root, (void *) "sleep_config",&msm_gpio_sleep_cfg_fops);
#endif
	destroy_dentry_list();

	return 0;

}
