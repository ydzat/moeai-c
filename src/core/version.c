/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: src/core/version.c
 * 描述: 版本信息实现
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/build_bug.h>
#include <linux/utsname.h>
#include "../../include/core/version.h"

/**
 * 获取模块版本信息
 * @buf: 用于存储版本信息的缓冲区
 * @size: 缓冲区大小
 */
void moeai_version_info(char *buf, size_t size)
{
    struct new_utsname *uname;
    
    if (!buf || size == 0)
        return;
    
    /* 获取系统信息 */
    uname = utsname();
    
    /* 格式化版本信息 */
    snprintf(buf, size, 
             "MoeAI-C v%d.%d.%d%s - 智能内核助手模块\n"
             "运行于: %s %s %s %s",
             MOEAI_VERSION_MAJOR, MOEAI_VERSION_MINOR, MOEAI_VERSION_PATCH,
             MOEAI_VERSION_SUFFIX,
             uname->sysname, uname->release, uname->version, uname->machine);
}