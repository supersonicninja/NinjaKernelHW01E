==update:2011.08.04=============================================================================================================
仅支持手工加载此检测功能。
1.合入rsm代码后，重新编译整个工程，生成的rsm.ko和busybox、load_rsm.sh在打包system.img时会自动放到/system/bin目录中。
2.用adb shell连接手机，并使用load_rsm.sh脚本加载rsm.ko模块，加载完后，rsm默认会对所有进程进行监控，默认监控时间间隔为120s。
3.插入模块后在/proc/rsm/目录下生成pname和timeout两个文件，使用下面的命令进行参数的设置：
echo xx > pname 设置不需要监控的进程名称
echo xx > timeout 设置监控的间隔时间
最多可以排除监控的进程数为16个。
例如不需要监控rpcrotuer_smd_x，krmt_storagecln，krmt_storagecln，krtcclntd，krtcclntcbd，kbatteryclntd，kbatteryclntcbd等进程（多个进程间用逗号或者空格隔开），监控时间间隔为60s：
echo rpcrotuer_smd_x，krmt_storagecln，krmt_storagecln，krtcclntd，krtcclntcbd，kbatteryclntd，kbatteryclntcbd > pname
echo 60 > timeout

==init:2011.07.13=============================================================================================================
1.将rsm目录下的源代码copy到kernel/drivers/staing/rsm/下，使用 make bootiamge编译  生成的ko在out/target/product/msm7627_surf/obj/KERNEL_OBJ/drivers/staging/rsm目录下

2.使用load_rsm.sh脚本加载模块(确保手机上有busybox，脚本中用到grep和sed)

3.插入模块后在/proc/rsm/目录下生成pname和timeout两个文件
echo xx > pname 设置需要监控的进程名称
echo xx > timeout 设置监控的间隔时间


说明：使用rmmod 卸载模块时需要等timeout时间，这个问题暂时还未解决，后续版本会解决这个问题
