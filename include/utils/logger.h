/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/utils/logger.h
 * 描述: 日志系统接口定义
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef _MOEAI_LOGGER_H
#define _MOEAI_LOGGER_H

#include <linux/types.h>

/* 日志级别枚举 */
enum moeai_log_level {
    MOEAI_LOG_DEBUG = 0,  /* 调试信息 */
    MOEAI_LOG_INFO = 1,   /* 普通信息 */
    MOEAI_LOG_WARN = 2,   /* 警告信息 */
    MOEAI_LOG_ERROR = 3,  /* 错误信息 */
    MOEAI_LOG_FATAL = 4   /* 致命错误 */
};

/* 日志条目结构体 */
struct moeai_log_entry {
    u64 timestamp;                /* 纳秒级时间戳 */
    enum moeai_log_level level;   /* 日志级别 */
    char module[16];              /* 模块名称 */
    char message[256];            /* 日志消息 */
};

/* 日志系统配置结构体 */
struct moeai_logger_config {
    enum moeai_log_level min_level; /* 最小日志级别 */
    bool console_output;            /* 是否输出到控制台 */
    bool buffer_output;             /* 是否输出到缓冲区 */
    size_t buffer_size;             /* 缓冲区大小 */
};

/* 日志系统API */
int moeai_logger_init(bool debug_mode);
void moeai_logger_exit(void);
void moeai_log(enum moeai_log_level level, const char *module, const char *fmt, ...);
int moeai_logger_get_recent_logs(struct moeai_log_entry *entries, size_t max_entries, size_t *count);
int moeai_logger_set_config(const struct moeai_logger_config *config);
int moeai_logger_get_config(struct moeai_logger_config *config);

/* 便捷日志宏 */
#define MOEAI_DEBUG(module, fmt, ...) \
    moeai_log(MOEAI_LOG_DEBUG, module, fmt, ##__VA_ARGS__)

#define MOEAI_INFO(module, fmt, ...) \
    moeai_log(MOEAI_LOG_INFO, module, fmt, ##__VA_ARGS__)

#define MOEAI_WARN(module, fmt, ...) \
    moeai_log(MOEAI_LOG_WARN, module, fmt, ##__VA_ARGS__)

#define MOEAI_ERROR(module, fmt, ...) \
    moeai_log(MOEAI_LOG_ERROR, module, fmt, ##__VA_ARGS__)

#define MOEAI_FATAL(module, fmt, ...) \
    moeai_log(MOEAI_LOG_FATAL, module, fmt, ##__VA_ARGS__)

#endif /* _MOEAI_LOGGER_H */