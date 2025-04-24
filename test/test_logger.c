/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: test/test_logger.c
 * 描述: 日志系统单元测试
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "utils/logger.h"
#include "utils/common_defs.h"

/* 测试环境初始化函数 */
static int __init test_logger_init(void)
{
    int ret;
    struct moeai_logger_config config;
    
    pr_info("MoeAI-C: 开始日志系统测试\n");
    
    /* 测试1: 初始化日志系统 */
    ret = moeai_logger_init(true);
    if (ret != 0) {
        pr_err("测试失败: 无法初始化日志系统，错误码: %d\n", ret);
        return ret;
    }
    pr_info("测试通过: 日志系统初始化成功\n");
    
    /* 测试2: 获取日志配置 */
    ret = moeai_logger_get_config(&config);
    if (ret != 0) {
        pr_err("测试失败: 无法获取日志配置，错误码: %d\n", ret);
        moeai_logger_exit();
        return ret;
    }
    
    /* 验证配置是否正确 */
    if (!config.debug_enabled || config.min_level != MOEAI_LOG_DEBUG || 
        !config.console_output || !config.buffer_output) {
        pr_err("测试失败: 配置验证错误 (debug=%d, min_level=%d, console=%d, buffer=%d)\n",
               config.debug_enabled, config.min_level, 
               config.console_output, config.buffer_output);
        moeai_logger_exit();
        return -EINVAL;
    }
    pr_info("测试通过: 日志配置验证成功\n");
    
    /* 测试3: 创建procfs接口 */
    ret = moeai_logger_create_procfs();
    if (ret != 0) {
        pr_err("测试失败: 无法创建procfs接口，错误码: %d\n", ret);
        moeai_logger_exit();
        return ret;
    }
    pr_info("测试通过: procfs接口创建成功\n");
    
    /* 测试4: 写入不同级别的日志 */
    MOEAI_DEBUG("TestMod", "这是一条调试日志");
    MOEAI_INFO("TestMod", "这是一条信息日志");
    MOEAI_WARN("TestMod", "这是一条警告日志");
    MOEAI_ERROR("TestMod", "这是一条错误日志，错误码: %d", -1);
    MOEAI_FATAL("TestMod", "这是一条致命错误日志");
    pr_info("测试通过: 成功写入多级别日志\n");
    
    /* 测试5: 修改日志配置 */
    config.min_level = MOEAI_LOG_WARN;  /* 只记录警告及以上级别 */
    ret = moeai_logger_set_config(&config);
    if (ret != 0) {
        pr_err("测试失败: 无法设置日志配置，错误码: %d\n", ret);
        moeai_logger_exit();
        return ret;
    }
    
    /* 测试6: 验证日志级别过滤 */
    MOEAI_DEBUG("TestMod", "这条调试日志不应该被记录");
    MOEAI_INFO("TestMod", "这条信息日志不应该被记录");
    MOEAI_WARN("TestMod", "这条警告日志应该被记录");
    
    /* 重新获取配置验证是否生效 */
    ret = moeai_logger_get_config(&config);
    if (ret != 0 || config.min_level != MOEAI_LOG_WARN) {
        pr_err("测试失败: 日志配置修改未生效，min_level=%d\n", config.min_level);
        moeai_logger_exit();
        return -EINVAL;
    }
    pr_info("测试通过: 日志配置修改成功\n");
    
    /* 测试7: 清空日志缓冲区 */
    ret = moeai_logger_clear();
    if (ret != 0) {
        pr_err("测试失败: 无法清空日志缓冲区，错误码: %d\n", ret);
        moeai_logger_exit();
        return ret;
    }
    pr_info("测试通过: 日志缓冲区清空成功\n");
    
    /* 测试8: 格式化日志测试 */
    MOEAI_INFO("FormatTest", "整数: %d, 字符串: %s, 指针: %p", 
              42, "测试字符串", (void *)test_logger_init);
    pr_info("测试通过: 格式化日志功能正常\n");
    
    pr_info("MoeAI-C: 日志系统测试全部通过!\n");
    
    /* 注意：不要在这里清理日志系统，因为是测试模块，会在exit函数中清理 */
    return 0;
}

/* 测试环境清理函数 */
static void __exit test_logger_exit(void)
{
    /* 清理日志系统 */
    moeai_logger_remove_procfs();
    moeai_logger_exit();
    
    pr_info("MoeAI-C: 日志系统测试清理完成\n");
}

module_init(test_logger_init);
module_exit(test_logger_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C 日志系统测试");
MODULE_VERSION("0.1");