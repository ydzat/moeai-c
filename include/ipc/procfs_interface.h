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
#define MOEAI_PROCFS_SELFTEST "selftest"  /* 新增: 自检接口路径 */

/* 命令字符串最大长度 */
#define MOEAI_MAX_CMD_LEN    256

/* 自检结果存储的最大长度 */
#define MOEAI_MAX_SELFTEST_LEN  8192

/* 自检功能的结果码 */
typedef enum moeai_selftest_result {
    MOEAI_TEST_PASS = 0,      /* 测试通过 */
    MOEAI_TEST_WARNING = 1,   /* 测试通过但有警告 */
    MOEAI_TEST_FAIL = 2,      /* 测试失败 */
    MOEAI_TEST_SKIP = 3,      /* 测试跳过 */
} moeai_selftest_result_t;

/* procfs 接口 API */
int moeai_procfs_init(void);
void moeai_procfs_exit(void);

/* 自检功能触发函数 (提供给 control write 使用) */
int moeai_trigger_selftest(void);

#endif /* _MOEAI_PROCFS_INTERFACE_H */