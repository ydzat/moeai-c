/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/ipc/procfs_interface.h
 * 描述: /proc文件系统接口定义
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef _MOEAI_PROCFS_INTERFACE_H
#define _MOEAI_PROCFS_INTERFACE_H

#include <linux/proc_fs.h>

/* procfs 路径定义 */
#define MOEAI_PROCFS_ROOT    "moeai"
#define MOEAI_PROCFS_STATUS  "status"
#define MOEAI_PROCFS_CONTROL "control"
#define MOEAI_PROCFS_LOG     "log"

/* 命令字符串最大长度 */
#define MOEAI_MAX_CMD_LEN    256

/* procfs 接口 API */
int moeai_procfs_init(void);
void moeai_procfs_exit(void);

#endif /* _MOEAI_PROCFS_INTERFACE_H */