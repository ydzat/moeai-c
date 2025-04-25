/**
 * MoeAI-C - Intelligent Kernel Assistant Module
 * 
 * File: test/test_logger.c
 * Description: Logger system unit test
 * 
 * Copyright © 2025 @ydzat
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "../include/utils/logger.h"
#include "../include/utils/common_defs.h"
#include "../include/utils/lang.h"

/* 测试环境初始化函数 */
static int __init test_logger_init(void)
{
    int ret;
    struct moeai_logger_config config;
    
    pr_info("%s\n", lang_get(LANG_TEST_LOG_START));
    
    /* 测试1: 初始化日志系统 */
    ret = moeai_logger_init(true);
    if (ret != 0) {
        pr_err("%s\n", lang_get(LANG_TEST_LOG_INIT_FAILED), ret);
        return ret;
    }
    pr_info("%s\n", lang_get(LANG_TEST_LOG_INIT_PASSED));
    
    /* 测试2: 获取日志配置 */
    ret = moeai_logger_get_config(&config);
    if (ret != 0) {
        pr_err("%s\n", lang_get(LANG_TEST_LOG_CONFIG_FAILED), ret);
        moeai_logger_exit();
        return ret;
    }
    
    /* 验证配置是否正确 */
    if (config.min_level != MOEAI_LOG_DEBUG || 
        !config.console_output || !config.buffer_output) {
        pr_err("%s\n", lang_get(LANG_TEST_LOG_CONFIG_FAILED),
               config.min_level, config.console_output, config.buffer_output);
        moeai_logger_exit();
        return -EINVAL;
    }
    pr_info("%s\n", lang_get(LANG_TEST_LOG_CONFIG_VERIFY_PASSED));
    
    /* 测试3: 创建procfs接口 */
    ret = moeai_logger_create_procfs();
    if (ret != 0) {
        pr_err("%s\n", lang_get(LANG_TEST_LOG_PROCFS_FAILED), ret);
        moeai_logger_exit();
        return ret;
    }
    pr_info("%s\n", lang_get(LANG_TEST_LOG_PROCFS_PASSED));
    
    /* 测试4: 写入不同级别的日志 */
    MOEAI_DEBUG("TestMod", lang_get(LANG_TEST_DEBUG_LOG_MSG));
    MOEAI_INFO("TestMod", lang_get(LANG_TEST_INFO_LOG_MSG));
    MOEAI_WARN("TestMod", lang_get(LANG_TEST_WARN_LOG_MSG));
    MOEAI_ERROR("TestMod", lang_get(LANG_TEST_ERROR_LOG_MSG), -1);
    MOEAI_FATAL("TestMod", lang_get(LANG_TEST_FATAL_LOG_MSG));
    pr_info("%s\n", lang_get(LANG_TEST_LOG_WRITE_PASSED));
    
    /* 测试5: 修改日志配置 */
    config.min_level = MOEAI_LOG_WARN;  /* 只记录警告及以上级别 */
    ret = moeai_logger_set_config(&config);
    if (ret != 0) {
        pr_err("%s\n", lang_get(LANG_TEST_LOG_CONFIG_SET_FAILED), ret);
        moeai_logger_exit();
        return ret;
    }
    
    /* 测试6: 验证日志级别过滤 */
    MOEAI_DEBUG("TestMod", lang_get(LANG_TEST_SHOULD_NOT_LOG_DEBUG));
    MOEAI_INFO("TestMod", lang_get(LANG_TEST_SHOULD_NOT_LOG_INFO));
    MOEAI_WARN("TestMod", lang_get(LANG_TEST_SHOULD_LOG_WARN));
    
    /* 重新获取配置验证是否生效 */
    ret = moeai_logger_get_config(&config);
    if (ret != 0 || config.min_level != MOEAI_LOG_WARN) {
        pr_err("%s\n", lang_get(LANG_TEST_LOG_CONFIG_LEVEL_FAILED), config.min_level);
        moeai_logger_exit();
        return -EINVAL;
    }
    pr_info("%s\n", lang_get(LANG_TEST_LOG_LEVEL_PASSED));
    
    /* 测试7: 清空日志缓冲区 */
    ret = moeai_logger_clear();
    if (ret != 0) {
        pr_err("%s\n", lang_get(LANG_TEST_LOG_CLEAR_FAILED), ret);
        moeai_logger_exit();
        return ret;
    }
    pr_info("%s\n", lang_get(LANG_TEST_LOG_CLEAR_PASSED));
    
    /* 测试8: 格式化日志测试 */
    MOEAI_INFO("FormatTest", lang_get(LANG_TEST_FORMAT_LOG_MSG), 
              42, "TestString", (void *)test_logger_init);
    pr_info("%s\n", lang_get(LANG_TEST_LOG_FORMAT_PASSED));
    
    pr_info("%s\n", lang_get(LANG_TEST_LOG_ALL_PASSED));
    
    /* 注意：不要在这里清理日志系统，因为是测试模块，会在exit函数中清理 */
    return 0;
}

/* 测试环境清理函数 */
static void __exit test_logger_exit(void)
{
    /* 清理日志系统 */
    moeai_logger_remove_procfs();
    moeai_logger_exit();
    
    pr_info("%s\n", lang_get(LANG_TEST_LOG_CLEANUP));
}

module_init(test_logger_init);
module_exit(test_logger_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C Logger System Test Module");
MODULE_VERSION("0.1");
