/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: src/ipc/procfs.c
 * 描述: procfs接口实现
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/time.h>
#include "../../include/ipc/procfs_interface.h"
#include "../../include/modules/mem_monitor.h"
#include "../../include/utils/logger.h"
#include "../../include/core/version.h"

/* 模块名称 */
#define MODULE_NAME "procfs"

/* procfs 条目 */
static struct proc_dir_entry *moeai_procfs_root;
static struct proc_dir_entry *status_entry;
static struct proc_dir_entry *control_entry;
static struct proc_dir_entry *log_entry;

/**
 * 状态文件的show回调
 */
static int moeai_procfs_status_show(struct seq_file *seq, void *v)
{
    struct moeai_mem_stats stats;
    char version_buf[256];
    int ret;
    
    /* 获取版本信息 */
    moeai_version_info(version_buf, sizeof(version_buf));
    seq_printf(seq, "%s\n\n", version_buf);
    
    /* 获取内存状态 */
    ret = moeai_mem_monitor_get_stats(&stats);
    if (ret) {
        seq_printf(seq, "Error: 无法获取内存状态 (错误码: %d)\n", ret);
        return 0;
    }
    
    /* 输出内存状态 */
    seq_puts(seq, "内存状态:\n");
    seq_printf(seq, "  总物理内存: %lu KB\n", stats.total_ram);
    seq_printf(seq, "  空闲内存: %lu KB\n", stats.free_ram);
    seq_printf(seq, "  可用内存: %lu KB\n", stats.available_ram);
    seq_printf(seq, "  缓存内存: %lu KB\n", stats.cached_ram);
    seq_printf(seq, "  内存使用率: %u%%\n", stats.mem_usage_percent);
    seq_printf(seq, "  交换空间总量: %lu KB\n", stats.swap_total);
    seq_printf(seq, "  空闲交换空间: %lu KB\n", stats.swap_free);
    seq_printf(seq, "  交换空间使用率: %u%%\n\n", stats.swap_usage_percent);
    
    /* 输出监控配置 */
    {
        struct moeai_mem_monitor_config config;
        moeai_mem_monitor_get_config(&config);
        
        seq_puts(seq, "监控配置:\n");
        seq_printf(seq, "  检查间隔: %u ms\n", config.check_interval_ms);
        seq_printf(seq, "  警告阈值: %u%%\n", config.warn_threshold);
        seq_printf(seq, "  临界阈值: %u%%\n", config.critical_threshold);
        seq_printf(seq, "  紧急阈值: %u%%\n", config.emergency_threshold);
        seq_printf(seq, "  自动回收: %s\n\n", config.auto_reclaim ? "开启" : "关闭");
    }
    
    return 0;
}

static int moeai_procfs_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, moeai_procfs_status_show, NULL);
}

static const struct proc_ops moeai_procfs_status_fops = {
    .proc_open = moeai_procfs_status_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/**
 * 控制文件的write回调
 */
static ssize_t moeai_procfs_control_write(struct file *file, const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    char buf[MOEAI_MAX_CMD_LEN];
    size_t buf_size = min(count, sizeof(buf) - 1);
    
    /* 复制用户数据 */
    if (copy_from_user(buf, user_buf, buf_size))
        return -EFAULT;
    
    /* 确保字符串结束 */
    buf[buf_size] = '\0';
    
    /* 处理命令 */
    if (strncmp(buf, "reclaim", 7) == 0) {
        /* 执行内存回收 */
        long reclaimed = moeai_mem_reclaim(MOEAI_MEM_RECLAIM_MODERATE);
        MOEAI_INFO(MODULE_NAME, "执行内存回收，释放了 %ld KB", reclaimed);
    } 
    else if (strncmp(buf, "set threshold ", 13) == 0) {
        /* 设置阈值 */
        unsigned int threshold;
        if (kstrtouint(buf + 13, 10, &threshold) == 0) {
            struct moeai_mem_monitor_config config;
            moeai_mem_monitor_get_config(&config);
            config.warn_threshold = threshold;
            config.critical_threshold = threshold + 10;
            config.emergency_threshold = threshold + 20;
            moeai_mem_monitor_set_config(&config);
            MOEAI_INFO(MODULE_NAME, "设置内存阈值为 %u%%", threshold);
        }
    }
    else if (strncmp(buf, "set interval ", 13) == 0) {
        /* 设置检查间隔 */
        unsigned int interval;
        if (kstrtouint(buf + 13, 10, &interval) == 0) {
            struct moeai_mem_monitor_config config;
            moeai_mem_monitor_get_config(&config);
            config.check_interval_ms = interval;
            moeai_mem_monitor_set_config(&config);
            MOEAI_INFO(MODULE_NAME, "设置检查间隔为 %u ms", interval);
        }
    }
    else if (strncmp(buf, "set autoreclaim ", 16) == 0) {
        /* 设置自动回收 */
        if (strncmp(buf + 16, "on", 2) == 0 || strncmp(buf + 16, "true", 4) == 0) {
            struct moeai_mem_monitor_config config;
            moeai_mem_monitor_get_config(&config);
            config.auto_reclaim = true;
            moeai_mem_monitor_set_config(&config);
            MOEAI_INFO(MODULE_NAME, "启用自动内存回收");
        } else if (strncmp(buf + 16, "off", 3) == 0 || strncmp(buf + 16, "false", 5) == 0) {
            struct moeai_mem_monitor_config config;
            moeai_mem_monitor_get_config(&config);
            config.auto_reclaim = false;
            moeai_mem_monitor_set_config(&config);
            MOEAI_INFO(MODULE_NAME, "禁用自动内存回收");
        }
    }
    else {
        MOEAI_WARN(MODULE_NAME, "未知命令: %s", buf);
    }
    
    /* 返回处理的数据大小 */
    return count;
}

static const struct proc_ops moeai_procfs_control_fops = {
    .proc_write = moeai_procfs_control_write,
};

/**
 * 日志文件的show回调
 */
static int moeai_procfs_log_show(struct seq_file *seq, void *v)
{
    struct moeai_log_entry *entries;
    size_t count, i;
    const char *level_str;
    struct timespec64 ts;
    
    /* 分配临时缓冲区 */
    entries = kmalloc(sizeof(*entries) * 100, GFP_KERNEL);
    if (!entries)
        return -ENOMEM;
    
    /* 获取日志条目 */
    if (moeai_logger_get_recent_logs(entries, 100, &count)) {
        seq_puts(seq, "Error: 无法获取日志条目\n");
        kfree(entries);
        return 0;
    }
    
    /* 遍历并输出日志 */
    for (i = 0; i < count; i++) {
        /* 将纳秒时间戳转换为timespec */
        ts = ns_to_timespec64(entries[i].timestamp);
        
        /* 获取日志级别字符串 */
        switch (entries[i].level) {
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
        
        /* 输出格式化日志条目 */
        seq_printf(seq, "[%5lld.%06ld] %-5s [%-8s] %s\n",
                  (long long)ts.tv_sec, ts.tv_nsec / 1000,
                  level_str, entries[i].module, entries[i].message);
    }
    
    kfree(entries);
    return 0;
}

static int moeai_procfs_log_open(struct inode *inode, struct file *file)
{
    return single_open(file, moeai_procfs_log_show, NULL);
}

static const struct proc_ops moeai_procfs_log_fops = {
    .proc_open = moeai_procfs_log_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/**
 * 初始化procfs接口
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_procfs_init(void)
{
    struct proc_dir_entry *root = NULL;
    
    /* 创建主目录 */
    root = proc_mkdir(MOEAI_PROCFS_ROOT, NULL);
    if (!root) {
        MOEAI_ERROR(MODULE_NAME, "无法创建procfs根目录");
        return -ENOMEM;
    }
    
    /* 保存根目录指针 */
    moeai_procfs_root = root;
    
    /* 创建状态文件 */
    status_entry = proc_create(MOEAI_PROCFS_STATUS, 0444, root, 
                             &moeai_procfs_status_fops);
    if (!status_entry) {
        MOEAI_ERROR(MODULE_NAME, "无法创建status文件");
        goto err_status;
    }
    
    /* 创建控制文件 */
    control_entry = proc_create(MOEAI_PROCFS_CONTROL, 0222, root,
                              &moeai_procfs_control_fops);
    if (!control_entry) {
        MOEAI_ERROR(MODULE_NAME, "无法创建control文件");
        goto err_control;
    }
    
    /* 创建日志文件 */
    log_entry = proc_create(MOEAI_PROCFS_LOG, 0444, root,
                          &moeai_procfs_log_fops);
    if (!log_entry) {
        MOEAI_ERROR(MODULE_NAME, "无法创建log文件");
        goto err_log;
    }
    
    MOEAI_INFO(MODULE_NAME, "procfs接口创建成功");
    return 0;
    
err_log:
    proc_remove(control_entry);
err_control:
    proc_remove(status_entry);
err_status:
    proc_remove(root);
    moeai_procfs_root = NULL;
    return -ENOMEM;
}

/**
 * 清理procfs接口
 */
void moeai_procfs_exit(void)
{
    if (!moeai_procfs_root)
        return;
    
    /* 删除所有条目 */
    proc_remove(log_entry);
    proc_remove(control_entry);
    proc_remove(status_entry);
    proc_remove(moeai_procfs_root);
    
    moeai_procfs_root = NULL;
    status_entry = NULL;
    control_entry = NULL;
    log_entry = NULL;
    
    MOEAI_INFO(MODULE_NAME, "procfs接口已清理");
}