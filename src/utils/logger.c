/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: src/utils/logger.c
 * 描述: 日志系统实现
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/stdarg.h>
#include "../../include/utils/logger.h"
#include "../../include/utils/ring_buffer.h"

/* 日志缓冲区大小 */
#define MOEAI_LOG_BUFFER_SIZE 100

/* 日志系统上下文 */
struct moeai_logger_context {
    struct moeai_logger_config config;
    struct moeai_ring_buffer *log_buffer;
    spinlock_t buffer_lock;
};

/* 全局日志上下文 */
static struct moeai_logger_context moeai_logger_ctx;

/**
 * 初始化日志系统
 * @debug_mode: 是否启用调试模式
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_logger_init(bool debug_mode)
{
    memset(&moeai_logger_ctx, 0, sizeof(moeai_logger_ctx));
    
    /* 设置默认配置 */
    moeai_logger_ctx.config.min_level = debug_mode ? MOEAI_LOG_DEBUG : MOEAI_LOG_INFO;
    moeai_logger_ctx.config.console_output = true;
    moeai_logger_ctx.config.buffer_output = true;
    moeai_logger_ctx.config.buffer_size = MOEAI_LOG_BUFFER_SIZE;
    
    spin_lock_init(&moeai_logger_ctx.buffer_lock);
    
    /* 创建日志环形缓冲区 */
    moeai_logger_ctx.log_buffer = moeai_ring_buffer_create(
        moeai_logger_ctx.config.buffer_size,
        sizeof(struct moeai_log_entry)
    );
    
    if (!moeai_logger_ctx.log_buffer) {
        pr_err("MoeAI-C: 无法创建日志缓冲区\n");
        return -ENOMEM;
    }
    
    pr_info("MoeAI-C: 日志系统初始化成功，调试模式: %s\n", 
            debug_mode ? "开启" : "关闭");
    
    return 0;
}

/**
 * 清理日志系统
 */
void moeai_logger_exit(void)
{
    if (moeai_logger_ctx.log_buffer) {
        moeai_ring_buffer_destroy(moeai_logger_ctx.log_buffer);
        moeai_logger_ctx.log_buffer = NULL;
    }
    
    pr_info("MoeAI-C: 日志系统已清理\n");
}

/**
 * 记录一条日志
 * @level: 日志级别
 * @module: 模块名称
 * @fmt: 格式化字符串
 * @...: 变长参数
 */
void moeai_log(enum moeai_log_level level, const char *module, const char *fmt, ...)
{
    va_list args;
    struct moeai_log_entry entry;
    char message[256];
    int ret;
    
    /* 检查日志级别 */
    if (level < moeai_logger_ctx.config.min_level)
        return;
        
    /* 格式化消息 */
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    
    /* 填充日志条目 */
    entry.timestamp = ktime_get_real_ns();
    entry.level = level;
    strncpy(entry.module, module, sizeof(entry.module) - 1);
    entry.module[sizeof(entry.module) - 1] = '\0';
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';
    
    /* 输出到内核日志 */
    if (moeai_logger_ctx.config.console_output) {
        const char *level_str;
        
        switch (level) {
        case MOEAI_LOG_DEBUG:
            level_str = "DEBUG";
            break;
        case MOEAI_LOG_INFO:
            level_str = "INFO";
            break;
        case MOEAI_LOG_WARN:
            level_str = "WARN";
            break;
        case MOEAI_LOG_ERROR:
            level_str = "ERROR";
            break;
        case MOEAI_LOG_FATAL:
            level_str = "FATAL";
            break;
        default:
            level_str = "UNKNOWN";
            break;
        }
        
        printk("%s: MoeAI-C [%s] %s\n", level_str, module, message);
    }
    
    /* 写入环形缓冲区 */
    if (moeai_logger_ctx.config.buffer_output && moeai_logger_ctx.log_buffer) {
        spin_lock(&moeai_logger_ctx.buffer_lock);
        ret = moeai_ring_buffer_write(moeai_logger_ctx.log_buffer, &entry);
        spin_unlock(&moeai_logger_ctx.buffer_lock);
        
        if (ret)
            printk(KERN_WARNING "MoeAI-C: 无法写入日志缓冲区，错误码: %d\n", ret);
    }
}

/**
 * 获取最近的日志条目
 * @entries: 用于存储日志条目的缓冲区
 * @max_entries: 最大条目数
 * @count: 实际返回的条目数
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_logger_get_recent_logs(struct moeai_log_entry *entries, size_t max_entries, size_t *count)
{
    int ret;
    
    if (!entries || !count || max_entries == 0)
        return -EINVAL;
    
    if (!moeai_logger_ctx.log_buffer)
        return -EINVAL;
    
    spin_lock(&moeai_logger_ctx.buffer_lock);
    ret = moeai_ring_buffer_read_batch(moeai_logger_ctx.log_buffer, entries, max_entries, count);
    spin_unlock(&moeai_logger_ctx.buffer_lock);
    
    return ret;
}

/**
 * 设置日志配置
 * @config: 新的配置
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_logger_set_config(const struct moeai_logger_config *config)
{
    if (!config)
        return -EINVAL;
    
    spin_lock(&moeai_logger_ctx.buffer_lock);
    
    /* 检查缓冲区大小是否改变 */
    if (config->buffer_size != moeai_logger_ctx.config.buffer_size && 
        config->buffer_output) {
        struct moeai_ring_buffer *new_buffer;
        
        /* 创建新的缓冲区 */
        new_buffer = moeai_ring_buffer_create(config->buffer_size, 
                                            sizeof(struct moeai_log_entry));
        if (!new_buffer) {
            spin_unlock(&moeai_logger_ctx.buffer_lock);
            return -ENOMEM;
        }
        
        /* 替换旧的缓冲区 */
        if (moeai_logger_ctx.log_buffer) {
            moeai_ring_buffer_destroy(moeai_logger_ctx.log_buffer);
        }
        
        moeai_logger_ctx.log_buffer = new_buffer;
    }
    
    /* 更新配置 */
    moeai_logger_ctx.config = *config;
    
    spin_unlock(&moeai_logger_ctx.buffer_lock);
    
    return 0;
}

/**
 * 获取当前日志配置
 * @config: 存储当前配置的缓冲区
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_logger_get_config(struct moeai_logger_config *config)
{
    if (!config)
        return -EINVAL;
    
    spin_lock(&moeai_logger_ctx.buffer_lock);
    *config = moeai_logger_ctx.config;
    spin_unlock(&moeai_logger_ctx.buffer_lock);
    
    return 0;
}