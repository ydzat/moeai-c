/**
 * MoeAI-C - Intelligent Kernel Assistant Module
 * 
 * File: test/test_mem_monitor.c
 * Description: Memory monitor module unit test
 * 
 * Copyright © 2025 @ydzat
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "../include/modules/mem_monitor.h"
#include "../include/utils/logger.h"
#include "../include/utils/common_defs.h"
#include "../include/utils/lang.h"

/* Test environment initialization function */
static int __init test_mem_monitor_init(void)
{
    int ret;
    struct moeai_mem_stats stats;
    struct moeai_mem_monitor_config config, new_config;
    long reclaimed;
    
    pr_info("%s", lang_get(LANG_TEST_MEM_START));
    
    /* Test 1: Initialize logger system */
    ret = moeai_logger_init(true);
    if (ret != 0) {
        pr_err("%s", lang_get(LANG_TEST_MEM_LOG_INIT_FAILED), ret);
        return ret;
    }
    pr_info("%s", lang_get(LANG_TEST_MEM_LOG_INIT_PASSED));
    
    /* Test 2: Initialize memory monitor */
    ret = moeai_mem_monitor_init();
    if (ret != 0) {
        pr_err("%s", lang_get(LANG_TEST_MEM_INIT_FAILED), ret);
        moeai_logger_exit();
        return ret;
    }
    pr_info("%s", lang_get(LANG_TEST_MEM_INIT_PASSED));
    
    /* Test 3: Get memory stats */
    ret = moeai_mem_monitor_get_stats(&stats);
    if (ret != 0) {
        pr_err("%s", lang_get(LANG_TEST_MEM_GET_STATS_FAILED), ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    
    pr_info("%s", lang_get(LANG_TEST_MEM_STATS_FORMAT),
           stats.total_ram, stats.available_ram, stats.mem_usage_percent);
    pr_info("%s", lang_get(LANG_TEST_MEM_GET_STATS_PASSED));
    
    /* Test 4: Get default config */
    ret = moeai_mem_monitor_get_config(&config);
    if (ret != 0) {
        pr_err("%s", lang_get(LANG_TEST_MEM_GET_CONFIG_FAILED), ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    
    pr_info("%s", lang_get(LANG_TEST_MEM_CONFIG_FORMAT),
           config.check_interval_ms, config.warn_threshold, 
           config.critical_threshold, config.emergency_threshold,
           config.auto_reclaim ? lang_get(LANG_PROCFS_AUTO_RECLAIM_ON) : 
                                lang_get(LANG_PROCFS_AUTO_RECLAIM_OFF));
    pr_info("%s", lang_get(LANG_TEST_MEM_GET_CONFIG_PASSED));
    
    /* Test 5: Set new config */
    new_config = config;
    new_config.check_interval_ms = 10000;  /* 10 seconds */
    new_config.warn_threshold = 60;        /* 60% */
    new_config.critical_threshold = 75;    /* 75% */
    new_config.emergency_threshold = 90;   /* 90% */
    
    ret = moeai_mem_monitor_set_config(&new_config);
    if (ret != 0) {
        pr_err("%s", lang_get(LANG_TEST_MEM_SET_CONFIG_FAILED), ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    
    /* Verify new config is applied */
    memset(&config, 0, sizeof(config));
    ret = moeai_mem_monitor_get_config(&config);
    if (ret != 0 || config.check_interval_ms != new_config.check_interval_ms ||
        config.warn_threshold != new_config.warn_threshold) {
        pr_err("%s", lang_get(LANG_TEST_MEM_CONFIG_NOT_APPLIED));
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return -EINVAL;
    }
    pr_info("%s", lang_get(LANG_TEST_MEM_SET_CONFIG_PASSED));
    
    /* Test 6: Start memory monitor */
    ret = moeai_mem_monitor_start();
    if (ret != 0) {
        pr_err("%s", lang_get(LANG_TEST_MEM_START_FAILED), ret);
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return ret;
    }
    pr_info("%s", lang_get(LANG_TEST_MEM_START_PASSED));
    
    /* Test 7: Perform memory reclaim */
    /* 更安全的类型转换方式 */
    {
        long (*reclaim_fn)(enum moeai_mem_reclaim_policy);
        *(void **)&reclaim_fn = (void *)moeai_mem_reclaim;
        reclaimed = reclaim_fn(MOEAI_MEM_RECLAIM_GENTLE);
    }
    if (reclaimed < 0) {
        int err = (int)reclaimed;
        pr_err("%s", lang_get(LANG_TEST_MEM_RECLAIM_FAILED), err);
        moeai_mem_monitor_stop();
        moeai_mem_monitor_exit();
        moeai_logger_exit();
        return err;
    }
    pr_info("%s", lang_get(LANG_TEST_MEM_RECLAIM_PASSED), (int)reclaimed);
    
    /* Test 8: Stop memory monitor */
    moeai_mem_monitor_stop();
    pr_info("%s", lang_get(LANG_TEST_MEM_STOP_PASSED));
    
    pr_info("%s", lang_get(LANG_TEST_MEM_ALL_PASSED));
    
    /* Note: Don't cleanup here, will be done in exit function */
    return 0;
}

/* Test environment cleanup function */
static void __exit test_mem_monitor_exit(void)
{
    /* Cleanup memory monitor */
    moeai_mem_monitor_exit();
    
    /* Cleanup logger system */
    moeai_logger_exit();
    
    pr_info("%s", lang_get(LANG_TEST_MEM_CLEANUP));
}

module_init(test_mem_monitor_init);
module_exit(test_mem_monitor_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C Memory Monitor Test Module");
MODULE_VERSION("0.1");
