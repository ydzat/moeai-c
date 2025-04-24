/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: src/core/init.c
 * 描述: 核心初始化实现
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>

#include "core/init.h"
#include "core/version.h"
#include "utils/logger.h"
#include "utils/common_defs.h"

/* 模块状态变量 */
static bool core_initialized = false;
static bool debug_mode = false;

/* 获取调试模式状态 */
bool moeai_is_debug_mode(void)
{
    return debug_mode;
}

/* 注册标准模块 */
int moeai_register_modules(void)
{
    /* MVP阶段简化实现，后续可拓展 */
    MOEAI_INFO("Core", "注册标准模块");
    return 0;
}

/* 注销标准模块 */
void moeai_unregister_modules(void)
{
    /* MVP阶段简化实现，后续可拓展 */
    MOEAI_INFO("Core", "注销标准模块");
}

/* 核心初始化 */
int moeai_core_init(bool enable_debug)
{
    int ret = 0;
    
    /* 防止重复初始化 */
    if (core_initialized) {
        pr_warn("MoeAI-C: 核心系统已经初始化\n");
        return 0;
    }
    
    /* 设置调试模式 */
    debug_mode = enable_debug;
    
    MOEAI_INFO("Core", "核心系统初始化开始，调试模式: %s", 
              debug_mode ? "开启" : "关闭");
    
    /* 注册标准模块 */
    ret = moeai_register_modules();
    if (ret) {
        MOEAI_ERROR("Core", "标准模块注册失败，错误码: %d", ret);
        return ret;
    }
    
    /* 标记为已初始化 */
    core_initialized = true;
    
    MOEAI_INFO("Core", "核心系统初始化完成");
    return 0;
}

/* 核心清理 */
void moeai_core_exit(void)
{
    /* 检查是否已初始化 */
    if (!core_initialized) {
        pr_warn("MoeAI-C: 核心系统尚未初始化，无需清理\n");
        return;
    }
    
    MOEAI_INFO("Core", "核心系统清理开始");
    
    /* 注销标准模块 */
    moeai_unregister_modules();
    
    /* 重置状态 */
    core_initialized = false;
    
    MOEAI_INFO("Core", "核心系统清理完成");
}

EXPORT_SYMBOL(moeai_core_init);
EXPORT_SYMBOL(moeai_core_exit);
EXPORT_SYMBOL(moeai_register_modules);
EXPORT_SYMBOL(moeai_unregister_modules);
EXPORT_SYMBOL(moeai_is_debug_mode);