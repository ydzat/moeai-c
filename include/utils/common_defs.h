/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/utils/common_defs.h
 * 描述: 全局宏和常量定义
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef MOEAI_COMMON_DEFS_H
#define MOEAI_COMMON_DEFS_H

#include <linux/kernel.h>
#include <linux/version.h>

/* 模块版本信息 */
#define MOEAI_MAJOR_VERSION    0
#define MOEAI_MINOR_VERSION    1
#define MOEAI_PATCH_VERSION    0
#define MOEAI_VERSION_STR     "0.1.0"

/* 通用错误码定义 */
#define MOEAI_SUCCESS           0      /* 成功 */
#define MOEAI_ERROR_GENERIC    -1      /* 通用错误 */
#define MOEAI_ERROR_NOMEM      -2      /* 内存分配失败 */
#define MOEAI_ERROR_INVAL      -3      /* 无效参数 */
#define MOEAI_ERROR_BUSY       -4      /* 资源忙 */
#define MOEAI_ERROR_TIMEOUT    -5      /* 操作超时 */
#define MOEAI_ERROR_NOENT      -6      /* 不存在的实体 */
#define MOEAI_ERROR_IO         -7      /* IO错误 */
#define MOEAI_ERROR_AGAIN      -8      /* 需要重试 */

/* 模块功能宏控制 */
#define MOEAI_FEATURE_MEMORY_MONITOR    1   /* 内存监控功能 */
#define MOEAI_FEATURE_NETWORK_GUARD     1   /* 网络防护功能 */
#define MOEAI_FEATURE_FS_LOGGER         1   /* 文件系统日志功能 */
#define MOEAI_FEATURE_PROCFS            1   /* procfs接口功能 */
#define MOEAI_FEATURE_NETLINK           0   /* netlink接口功能（未实现） */

/* 默认配置值 */
#define MOEAI_DEFAULT_BUFFER_SIZE       4096
#define MOEAI_MAX_PATH_LEN              256
#define MOEAI_MAX_NAME_LEN              64
#define MOEAI_MAX_CMD_LEN               128
#define MOEAI_MAX_MODULES               16
#define MOEAI_MAX_EVENTS                64

/* procfs相关定义 */
#define MOEAI_PROCFS_ROOT               "moeai"
#define MOEAI_PROCFS_LOG                "log"
#define MOEAI_PROCFS_STATUS             "status"
#define MOEAI_PROCFS_CONTROL            "control"
#define MOEAI_PROCFS_MODULES            "modules"

/* 内核版本兼容宏 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
#define TIMESPEC64_TO_TIMESPEC(ts, ts64) ({ (ts)->tv_sec = (ts64)->tv_sec; (ts)->tv_nsec = (ts64)->tv_nsec; })
#define TIMESPEC_TO_TIMESPEC64(ts64, ts) ({ (ts64)->tv_sec = (ts)->tv_sec; (ts64)->tv_nsec = (ts)->tv_nsec; })
#endif

/* 格式化字符串安全处理 */
#define MOEAI_FORMAT_STRING(buf, size, fmt, ...) ({ \
    int __ret = snprintf(buf, size, fmt, ##__VA_ARGS__); \
    if (__ret >= size) \
        buf[size - 1] = '\0'; \
    __ret; \
})

/* 内存分配与释放跟踪宏 */
#ifdef CONFIG_MOEAI_MEM_DEBUG
#define moeai_kmalloc(size, flags) ({ \
    void *__ptr = kmalloc(size, flags); \
    pr_debug("MoeAI-C: kmalloc(%zu) = %p at %s:%d\n", \
             (size_t)size, __ptr, __FILE__, __LINE__); \
    __ptr; \
})
#define moeai_kfree(ptr) do { \
    pr_debug("MoeAI-C: kfree(%p) at %s:%d\n", ptr, __FILE__, __LINE__); \
    kfree(ptr); \
} while (0)
#else
#define moeai_kmalloc(size, flags) kmalloc(size, flags)
#define moeai_kfree(ptr) kfree(ptr)
#endif

/* 调试宏 */
#ifdef CONFIG_MOEAI_DEBUG
#define MOEAI_DEBUG_PRINT(fmt, ...) \
    pr_debug("MoeAI-C [%s:%d]: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define MOEAI_DEBUG_PRINT(fmt, ...) do {} while (0)
#endif

#endif /* MOEAI_COMMON_DEFS_H */