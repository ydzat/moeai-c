/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/core/version.h
 * 描述: 版本定义和版本信息管理
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef _MOEAI_VERSION_H
#define _MOEAI_VERSION_H

#include <linux/types.h>

/* 版本信息 */
#define MOEAI_VERSION_MAJOR    0
#define MOEAI_VERSION_MINOR    1
#define MOEAI_VERSION_PATCH    0
#define MOEAI_VERSION_SUFFIX   "-MVP"

/* 获取版本信息 */
void moeai_version_info(char *buf, size_t size);

#endif /* _MOEAI_VERSION_H */