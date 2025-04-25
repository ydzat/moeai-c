/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: src/main.c
 * 描述: 内核模块入口点
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "../include/modules/mem_monitor.h"
#include "../include/utils/logger.h"
#include "../include/ipc/procfs_interface.h"
#include "../include/utils/lang.h"

/* 模块参数 */
static bool debug_mode = false;
module_param(debug_mode, bool, 0644);
MODULE_PARM_DESC(debug_mode, "Enable debug mode (default: false)");

static char *language = "en";
module_param(language, charp, 0644);
MODULE_PARM_DESC(language, "Set language (zh/en) (default: en)");

/* 模块信息 - 必须使用静态字符串常量 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C Kernel Module");
MODULE_VERSION("0.2.0");

/* 运行时显示国际化模块信息 */
static void show_module_info(void)
{
    pr_info("%s\n", lang_get(LANG_MODULE_LICENSE));
    pr_info("%s\n", lang_get(LANG_MODULE_AUTHOR));
    pr_info("%s\n", lang_get(LANG_MODULE_DESCRIPTION));
    pr_info("%s\n", lang_get(LANG_MODULE_VERSION));
}

/* 加载模块时的初始化函数 */
static int __init moeai_init(void)
{
    int ret = 0;
    
    show_module_info();
    pr_info("%s\n", lang_get(LANG_MODULE_INIT_START));
    
    /* 初始化语言系统 */
    if (lang_init(language) != 0) {
        char *msg = lang_getf(LANG_MODULE_INIT_LANG_FAILED, language);
        pr_warn("%s\n", msg ? msg : "Failed to initialize language");
        if (msg) kfree(msg);
    }
    
    /* 初始化日志系统 */
    ret = moeai_logger_init(debug_mode);
    if (ret) {
        pr_err("%s %d\n", lang_get(LANG_ERR_LOG_INIT_FAILED), ret);
        return ret;
    }
    
    /* 初始化内存监控模块 */
    ret = moeai_mem_monitor_init();
    if (ret) {
        pr_err("%s %d\n", lang_get(LANG_ERR_MEM_INIT_FAILED), ret);
        goto err_mem_monitor;
    }
    
    /* 初始化procfs接口 */
    ret = moeai_procfs_init();
    if (ret) {
        pr_err("%s %d\n", lang_get(LANG_ERR_PROC_INIT_FAILED), ret);
        goto err_procfs;
    }
    
    /* 启动内存监控 */
    ret = moeai_mem_monitor_start();
    if (ret) {
        pr_err("%s %d\n", lang_get(LANG_ERR_MEM_START_FAILED), ret);
        goto err_mem_start;
    }
    
    pr_info("%s\n", lang_get(LANG_MODULE_INIT_COMPLETE));
    return 0;

err_mem_start:
    moeai_procfs_exit();
err_procfs:
    moeai_mem_monitor_exit();
err_mem_monitor:
    moeai_logger_exit();
    return ret;
}

/* 卸载模块时的清理函数 */
static void __exit moeai_exit(void)
{
    pr_info("%s\n", lang_get(LANG_MODULE_EXIT_START));
    
    /* 停止内存监控 */
    moeai_mem_monitor_stop();
    
    /* 清理procfs接口 */
    moeai_procfs_exit();
    
    /* 清理内存监控模块 */
    moeai_mem_monitor_exit();
    
    /* 清理日志系统 */
    moeai_logger_exit();
    
    pr_info("%s\n", lang_get(LANG_MODULE_EXIT_COMPLETE));
}

module_init(moeai_init);
module_exit(moeai_exit);
