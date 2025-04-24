/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/core/init.h
 * 描述: 核心初始化接口定义
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef MOEAI_CORE_INIT_H
#define MOEAI_CORE_INIT_H

#include <linux/types.h>

/**
 * MoeAI-C系统核心初始化
 * @param debug_mode 是否启用调试模式
 * @return 成功返回0，失败返回错误码
 */
int moeai_core_init(bool debug_mode);

/**
 * MoeAI-C系统核心清理
 */
void moeai_core_exit(void);

/**
 * 注册标准模块
 * @return 成功返回0，失败返回错误码
 */
int moeai_register_modules(void);

/**
 * 注销标准模块
 */
void moeai_unregister_modules(void);

/**
 * 获取调试模式状态
 * @return 如果调试模式开启返回true，否则返回false
 */
bool moeai_is_debug_mode(void);

#endif /* MOEAI_CORE_INIT_H */