comdef.h   ----  通用数据类型的定义
config_interface.h   ---- 接口层头文件
config_mgr.h  --- 数据管理层头文件；
configdata.h   --- 该模块内部数据结构定义的头文件；
hwconfig_enum.h  --- 配置表格中的枚举型定义，可以包含其他的模块的头文件

hw_s8600_configs.xml   --- s8600的配置数据
hw_u8600_configs.xml   --- u8600的配置数据
product_bootid.xml  --- 各产品和boot id的对应关系

hw_s8600_configs.c  --- s8600的配置数据
hw_u8600_configs.c  --- u8600的配置数据
hw_ver_total_config.c  --- 所有产品的配置数据

parse_prudct.pl --- 将各产品的xml配置数据转成c文件配置数据
parse_product_id.pl  --- 生成hw_ver_total_config.c文件

config_mgr.c   --- 数据管理层实现
config_interface.c  --- 接口层实现

test.c --- 测试驱动文件

