/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: test/test_mem_monitor.c
 * 描述: 内存监控模块单元测试
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "modules/mem_monitor.h"
#include "utils/logger.h"
#include "utils/common_defs.h"

/* 测试环境初始化函数 */
static int __init test_mem_monitor_init(void)
{
    int ret;
    struct moeai_mem_stats stats;
    struct moeai_mem_monitor_config config, new_config;
    long reclaimed;
    
    pr_info("MoeAI-C: 开始内存监控模块测试\n");
    
    /* 测试1: 初始化日志系统 */
    ret = moeai_logger_init(true);
    if (ret != 0) {
        pr_err("测试失败: 无法初始化日志系统，错误码: %d\n", ret);
        return ret;
    }
    pr_info("测试通过: 日志系统初始化成功\n");
    
    /* 测试2: 初始化内存监控模块 */
    ret = moeai_mem_monitor_init();
    if (ret != 0) {
        pr_err("测试失败: 无法初始化内存监控模块，错误码: %d\n", ret);
        moeai_logger_exit();
        return ret;
    }
    pr_info("测试通过: 内存监控模块初始化成功\n");
    
    /* 测试3: 获取内存状态 */
    ret = moeai_mem_monitor_get_stats(&stats);
    if (ret != 0) {
        pr_err("测试失败: 无法获取内存状态，错误码: %d\n", ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    
    pr_info("内存状态: 总量=%lu KB, 可用=%lu KB, 使用率=%u%%\n",
           stats.total_ram, stats.available_ram, stats.mem_usage_percent);
    pr_info("测试通过: 成功获取内存状态\n");
    
    /* 测试4: 获取默认配置 */
    ret = moeai_mem_monitor_get_config(&config);
    if (ret != 0) {
        pr_err("测试失败: 无法获取内存监控配置，错误码: %d\n", ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    
    pr_info("默认配置: 间隔=%ums, 阈值=%u/%u/%u%%, 自动回收=%s\n",
           config.check_interval_ms, config.warn_threshold, 
           config.critical_threshold, config.emergency_threshold,
           config.auto_reclaim ? "启用" : "禁用");
    pr_info("测试通过: 成功获取默认配置\n");
    
    /* 测试5: 设置新配置 */
    new_config = config;
    new_config.check_interval_ms = 10000;  /* 10秒 */
    new_config.warn_threshold = 60;        /* 60% */
    new_config.critical_threshold = 75;    /* 75% */
    new_config.emergency_threshold = 90;   /* 90% */
    
    ret = moeai_mem_monitor_set_config(&new_config);
    if (ret != 0) {
        pr_err("测试失败: 无法设置新配置，错误码: %d\n", ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    
    /* 验证新配置是否生效 */
    memset(&config, 0, sizeof(config));
    ret = moeai_mem_monitor_get_config(&config);
    if (ret != 0 || config.check_interval_ms != new_config.check_interval_ms ||
        config.warn_threshold != new_config.warn_threshold) {
        pr_err("测试失败: 新配置未正确应用\n");
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return -EINVAL;
    }
    pr_info("测试通过: 成功设置并验证新配置\n");
    
    /* 测试6: 启动内存监控 */
    ret = moeai_mem_monitor_start();
    if (ret != 0) {
        pr_err("测试失败: 无法启动内存监控，错误码: %d\n", ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    pr_info("测试通过: 成功启动内存监控\n");
    
    /* 测试7: 执行内存回收 */
    reclaimed = moeai_mem_reclaim(MOEAI_MEM_RECLAIM_GENTLE);
    if (reclaimed < 0) {
        pr_err("测试失败: 内存回收失败，错误码: %ld\n", reclaimed);
        moeai_mem_monitor_stop();
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return reclaimed;
    }
    pr_info("测试通过: 成功执行内存回收，释放了 %ld KB\n", reclaimed);
    
    /* 测试8: 停止内存监控 */
    ret = moeai_mem_monitor_stop();
    if (ret != 0) {
        pr_err("测试失败: 无法停止内存监控，错误码: %d\n", ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    pr_info("测试通过: 成功停止内存监控\n");
    
    pr_info("MoeAI-C: 内存监控模块测试全部通过!\n");
    
    /* 注意：不要在这里清理，在exit函数中清理 */
    return 0;
}

/* 测试环境清理函数 */
static void __exit test_mem_monitor_exit(void)
{
    /* 清理内存监控模块 */
    moeai_mem_monitor_exit();
    
    /* 清理日志系统 */
    moeai_logger_exit();
    
    pr_info("MoeAI-C: 内存监控模块测试清理完成\n");
}

module_init(test_mem_monitor_init);
module_exit(test_mem_monitor_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C 内存监控模块测试");
MODULE_VERSION("0.1");