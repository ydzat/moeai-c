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

/* 模块参数 */
static bool debug_mode = false;
module_param(debug_mode, bool, 0644);
MODULE_PARM_DESC(debug_mode, "启用调试模式");

/* 模块信息 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C - 智能内核助手模块");
MODULE_VERSION("0.1.0");

/* 加载模块时的初始化函数 */
static int __init moeai_init(void)
{
    int ret = 0;
    
    pr_info("MoeAI-C: 内核助手模块正在初始化...\n");
    
    /* 初始化日志系统 */
    ret = moeai_logger_init(debug_mode);
    if (ret) {
        pr_err("MoeAI-C: 日志系统初始化失败，错误码: %d\n", ret);
        return ret;
    }
    
    /* 初始化内存监控模块 */
    ret = moeai_mem_monitor_init();
    if (ret) {
        pr_err("MoeAI-C: 内存监控模块初始化失败，错误码: %d\n", ret);
        goto err_mem_monitor;
    }
    
    /* 初始化procfs接口 */
    ret = moeai_procfs_init();
    if (ret) {
        pr_err("MoeAI-C: procfs接口初始化失败，错误码: %d\n", ret);
        goto err_procfs;
    }
    
    /* 启动内存监控 */
    ret = moeai_mem_monitor_start();
    if (ret) {
        pr_err("MoeAI-C: 内存监控启动失败，错误码: %d\n", ret);
        goto err_mem_start;
    }
    
    pr_info("MoeAI-C: 初始化完成，模块已就绪\n");
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
    pr_info("MoeAI-C: 模块正在卸载...\n");
    
    /* 停止内存监控 */
    moeai_mem_monitor_stop();
    
    /* 清理procfs接口 */
    moeai_procfs_exit();
    
    /* 清理内存监控模块 */
    moeai_mem_monitor_exit();
    
    /* 清理日志系统 */
    moeai_logger_exit();
    
    pr_info("MoeAI-C: 模块已成功卸载\n");
}

module_init(moeai_init);
module_exit(moeai_exit);