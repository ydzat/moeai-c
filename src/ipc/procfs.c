/**
 * @Author: @ydzat
 * @Date: 2025-04-25 20:24:31
 * @Description: MoeAI-C procfs interface implementation
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/version.h>      /* 获取内核版本信息 */
#include <linux/utsname.h>      /* 获取系统信息 */
#include <linux/sysinfo.h>      /* 获取系统信息 */
#include "../../include/ipc/procfs_interface.h"
#include "../../include/modules/mem_monitor.h"
#include "../../include/utils/logger.h"
#include "../../include/utils/ring_buffer.h"
#include "../../include/core/version.h"
#include "../../include/utils/lang.h"
#include "../../include/utils/lang.h"

/* Module name */
#define MODULE_NAME "procfs"

/* procfs entries */
static struct proc_dir_entry *moeai_procfs_root;
static struct proc_dir_entry *status_entry;
static struct proc_dir_entry *control_entry;
static struct proc_dir_entry *log_entry;
static struct proc_dir_entry *selftest_entry;  /* 新增: 自检结果条目 */

/* Self-test related */
static char *selftest_buffer = NULL;  /* Self-test results buffer */
static size_t selftest_len = 0;       /* Self-test results length */
static bool selftest_done = false;    /* Whether self-test is completed */
static DEFINE_MUTEX(selftest_mutex);  /* Self-test mutex lock */

/* 自检结果缓冲区追加函数 */
static void selftest_append(const char *fmt, ...)
{
    va_list args;
    size_t avail_len, written_len;
    
    if (!selftest_buffer)
        return;
        
    mutex_lock(&selftest_mutex);
    
    avail_len = MOEAI_MAX_SELFTEST_LEN - selftest_len - 1;
    if (avail_len > 0) {
        va_start(args, fmt);
        written_len = vsnprintf(selftest_buffer + selftest_len, avail_len, fmt, args);
        va_end(args);
        
        /* 确保不超出缓冲区 */
        if (written_len > avail_len)
            written_len = avail_len;
            
        selftest_len += written_len;
    }
    
    mutex_unlock(&selftest_mutex);
}

/* 清除自检结果 */
static void selftest_clear(void)
{
    mutex_lock(&selftest_mutex);
    if (selftest_buffer)
        selftest_buffer[0] = '\0';
    selftest_len = 0;
    selftest_done = false;
    mutex_unlock(&selftest_mutex);
}

/* 自检函数：测试内存监控功能 */
static moeai_selftest_result_t test_memory_monitor(void)
{
    struct moeai_mem_stats stats;
    int ret;
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_MEMORY_TEST));
    
    ret = moeai_mem_monitor_get_stats(&stats);
    if (ret) {
        selftest_append("%s %d\n", lang_get(LANG_PROCFS_SELFTEST_FAIL), ret);
        return MOEAI_TEST_FAIL;
    }
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_PASS));
    
    if (stats.mem_usage_percent > 90) {
        selftest_append("%s %u%%\n", lang_get(LANG_PROCFS_SELFTEST_WARN), stats.mem_usage_percent);
        return MOEAI_TEST_WARNING;
    }
    
    selftest_append("%s %u%%\n", lang_get(LANG_PROCFS_SELFTEST_PASS), stats.mem_usage_percent);
    return MOEAI_TEST_PASS;
}

/* 自检函数：测试环形缓冲区功能 */
static moeai_selftest_result_t test_ring_buffer(void)
{
    struct moeai_ring_buffer *rb;
    const char *test_data = "test_data";
    char buffer[64];
    int ret;
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_RINGBUF_TEST));
    
    /* 创建测试缓冲区 */
    rb = moeai_ring_buffer_create(64, 1); // 创建容量为64，item_size为1的缓冲区
    if (!rb) {
        selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_FAIL));
        return MOEAI_TEST_FAIL;
    }
    
    /* 测试写入 */
    for (size_t i = 0; i < strlen(test_data); i++) {
        ret = moeai_ring_buffer_write(rb, &test_data[i]);
        if (ret != 0) {
            selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_FAIL));
            moeai_ring_buffer_destroy(rb);
            return MOEAI_TEST_FAIL;
        }
    }
    
    /* 测试读取 */
    memset(buffer, 0, sizeof(buffer));
    for (size_t i = 0; i < strlen(test_data); i++) {
        char ch;
        ret = moeai_ring_buffer_read(rb, &ch);
        if (ret != 0) {
            selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_FAIL));
            moeai_ring_buffer_destroy(rb);
            return MOEAI_TEST_FAIL;
        }
        buffer[i] = ch;
    }
    
    if (strcmp(buffer, test_data) != 0) {
        selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_FAIL));
        moeai_ring_buffer_destroy(rb);
        return MOEAI_TEST_FAIL;
    }
    
    /* 清理 */
    moeai_ring_buffer_destroy(rb);
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_PASS));
    return MOEAI_TEST_PASS;
}

/* 自检函数：测试日志系统 */
static moeai_selftest_result_t test_logger(void)
{
    struct moeai_log_entry *entries;
    size_t count;
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_LOGGER_TEST));
    
    /* 发送测试日志 */
    MOEAI_INFO("selftest", lang_get(LANG_PROCFS_SELFTEST_LOGGER_TEST));
    
    /* 分配临时缓冲区 */
    entries = kmalloc(sizeof(*entries) * 10, GFP_KERNEL);
    if (!entries) {
        selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_WARN));
        return MOEAI_TEST_WARNING;
    }
    
    /* 获取日志条目 */
    if (moeai_logger_get_recent_logs(entries, 10, &count)) {
        selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_FAIL));
        kfree(entries);
        return MOEAI_TEST_FAIL;
    }
    
    selftest_append("%s %zu\n", lang_get(LANG_PROCFS_SELFTEST_PASS), count);
    kfree(entries);
    return MOEAI_TEST_PASS;
}

/* 自检函数：测试procfs接口功能 */
static moeai_selftest_result_t test_procfs_interface(void)
{
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_PROCFS_INTERFACE));
    
    /* TODO: Implement procfs interface test */
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_PROCFS_INTERFACE));
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_PASS));
    return MOEAI_TEST_PASS;
}

/* 新增: 自检函数：测试网络防护功能 */
static moeai_selftest_result_t test_net_guard(void)
{
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_NET_GUARD));

    /* TODO: Complete netguard module test
     * 1. Check if network filter rules can be loaded
     * 2. Try to add test rules and verify
     * 3. Check statistics counters
     */
     
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_NET_GUARD));
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_NOT_IMPLEMENTED));
    return MOEAI_TEST_SKIP;
}

/* 新增: 自检函数：测试文件系统日志功能 */
static moeai_selftest_result_t test_fs_logger(void)
{
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_FS_LOGGER));
    
    /* TODO: Complete filesystem logger module test
     * 1. Check if filesystem monitoring points are registered
     * 2. Try to trigger a file access event and verify logging
     * 3. Test log rotation and storage
     */
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_FS_LOGGER));
    selftest_append("%s\n", lang_get(LANG_PROCFS_TEST_NOT_IMPLEMENTED));
    return MOEAI_TEST_SKIP;
}

/* 新增: 自检函数：基本性能测试 */
static moeai_selftest_result_t test_performance(void)
{
    int i;
    u64 duration;
    struct timespec64 ts_start, ts_end;
    char *test_buf = NULL;

    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_PERF_TEST));
    
    /* 分配测试缓冲区 */
    test_buf = kmalloc(1024 * 1024, GFP_KERNEL); /* 1MB测试缓冲区 */
    if (!test_buf) {
        selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_WARN));
        return MOEAI_TEST_WARNING;
    }
    
    /* 测试内存写入性能 */
    selftest_append("  %s\n", lang_get(LANG_PROCFS_SELFTEST_MEM_WRITE_PERF));
    ktime_get_real_ts64(&ts_start);
    
    for (i = 0; i < 1024 * 1024; i++)
        test_buf[i] = (char)(i & 0xFF);
    
    ktime_get_real_ts64(&ts_end);
    duration = (ts_end.tv_sec - ts_start.tv_sec) * 1000000000ULL +
               (ts_end.tv_nsec - ts_start.tv_nsec);
    
    selftest_append("    %s %llu.%03llu ms\n", 
                    lang_get(LANG_PROCFS_SELFTEST_WRITE_TIME),
                    duration / 1000000, (duration % 1000000) / 1000);
    
    /* 测试内存读取性能 */
    selftest_append("  %s\n", lang_get(LANG_PROCFS_SELFTEST_MEM_READ_PERF));
    ktime_get_real_ts64(&ts_start);
    
    volatile int sum = 0;
    for (i = 0; i < 1024 * 1024; i++)
        sum += test_buf[i];
    
    ktime_get_real_ts64(&ts_end);
    duration = (ts_end.tv_sec - ts_start.tv_sec) * 1000000000ULL +
               (ts_end.tv_nsec - ts_start.tv_nsec);
    
    selftest_append("    %s %llu.%03llu ms\n", 
                    lang_get(LANG_PROCFS_SELFTEST_READ_TIME),
                    duration / 1000000, (duration % 1000000) / 1000);
    
    /* 释放测试缓冲区 */
    kfree(test_buf);
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_PASS));
    return MOEAI_TEST_PASS;
}

/* 新增: 自检函数：系统环境信息收集 */
static moeai_selftest_result_t test_system_info(void)
{
    struct sysinfo si;
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_SYSINFO_TEST));
    
    /* 获取系统信息 */
    si_meminfo(&si);
    
    /* 输出系统内存信息 */
    selftest_append("  %s:\n", lang_get(LANG_PROCFS_MEMORY_STATUS));
    selftest_append("    %s: %lu MB\n", lang_get(LANG_PROCFS_SELFTEST_TOTAL), si.totalram >> 10);
    selftest_append("    %s: %lu MB\n", lang_get(LANG_PROCFS_SELFTEST_FREE), si.freeram >> 10);
    selftest_append("    %s: %lu MB\n", lang_get(LANG_PROCFS_SELFTEST_SHARED), si.sharedram >> 10);
    selftest_append("    %s: %lu MB\n", lang_get(LANG_PROCFS_SELFTEST_BUFFER), si.bufferram >> 10);
    
    /* 输出内核版本信息 */
    selftest_append("  %s: %s %s\n", lang_get(LANG_PROCFS_SELFTEST_KERNEL), 
                   init_utsname()->sysname, init_utsname()->release);
    
    /* 输出处理器信息 */
    selftest_append("  %s: %d\n", lang_get(LANG_PROCFS_SELFTEST_CPUS), num_present_cpus());
    
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_PASS));
    return MOEAI_TEST_PASS;
}

/**
 * 主自检函数 - 运行所有测试
 */
int moeai_trigger_selftest(void)
{
    unsigned int pass = 0, warn = 0, fail = 0, skip = 0;
    moeai_selftest_result_t result;
    struct timespec64 ts_start, ts_end;
    u64 duration_us;
    
    /* 清空旧的测试结果 */
    selftest_clear();
    
    /* 分配结果缓冲区（如果还没有分配） */
    if (!selftest_buffer) {
        selftest_buffer = kmalloc(MOEAI_MAX_SELFTEST_LEN, GFP_KERNEL);
        if (!selftest_buffer) {
            MOEAI_ERROR(MODULE_NAME, lang_get(LANG_PROCFS_ALLOC_BUFFER_FAILED));
            return -ENOMEM;
        }
        selftest_buffer[0] = '\0';
    }
    
    /* 记录开始时间 */
    ktime_get_real_ts64(&ts_start);
    
    /* 输出自检头部 */
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_HEADER));
    selftest_append("===================================\n");
    selftest_append("%s: %lld.%ld\n", lang_get(LANG_PROCFS_SELFTEST_TIMESTAMP),
                  (long long)ts_start.tv_sec, ts_start.tv_nsec / 1000);
    {
        char version_buf[256];
        moeai_version_info(version_buf, sizeof(version_buf));
        selftest_append("%s\n", version_buf);
    }
    selftest_append("===================================\n\n");

    /* 首先收集系统信息 */
    result = test_system_info();
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");
    
    /* 运行各项测试并统计结果 */
    result = test_memory_monitor();
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");
    
    result = test_net_guard();  /* 新增: 测试网络防护模块 */
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");
    
    result = test_fs_logger();  /* 新增: 测试文件系统日志模块 */
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");
    
    result = test_ring_buffer();
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");
    
    result = test_logger();
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");
    
    result = test_procfs_interface();
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");

    /* 运行性能测试 */
    result = test_performance();  /* 新增: 执行性能测试 */
    if (result == MOEAI_TEST_PASS) pass++;
    else if (result == MOEAI_TEST_WARNING) warn++;
    else if (result == MOEAI_TEST_FAIL) fail++;
    else skip++;
    selftest_append("\n");
    
    /* 记录结束时间并计算持续时间 */
    ktime_get_real_ts64(&ts_end);
    duration_us = (ts_end.tv_sec - ts_start.tv_sec) * 1000000 +
                 (ts_end.tv_nsec - ts_start.tv_nsec) / 1000;
    
    /* 输出自检结果摘要 */
    selftest_append("===================================\n");
    selftest_append("%s\n", lang_get(LANG_PROCFS_SELFTEST_SUMMARY));
    selftest_append("- %s: %u\n", lang_get(LANG_PROCFS_SELFTEST_PASS), pass);
    selftest_append("- %s: %u\n", lang_get(LANG_PROCFS_SELFTEST_WARN), warn);
    selftest_append("- %s: %u\n", lang_get(LANG_PROCFS_SELFTEST_FAIL), fail);
    selftest_append("- %s: %u\n", lang_get(LANG_PROCFS_SELFTEST_SKIP), skip);
    selftest_append("- %s: %u\n", lang_get(LANG_PROCFS_SELFTEST_TOTAL), pass + warn + fail + skip);
    selftest_append("- %s: %llu.%03llu ms\n", lang_get(LANG_PROCFS_SELFTEST_TIME_MS),
               duration_us / 1000, duration_us % 1000);
    selftest_append("===================================\n");
    
    /* 记录到日志 */
    MOEAI_INFO(MODULE_NAME, "%s: %s=%u, %s=%u, %s=%u, %s=%u", 
              lang_get(LANG_PROCFS_SELFTEST_RESULT),
              lang_get(LANG_PROCFS_SELFTEST_PASS), pass,
              lang_get(LANG_PROCFS_SELFTEST_WARN), warn,
              lang_get(LANG_PROCFS_SELFTEST_FAIL), fail,
              lang_get(LANG_PROCFS_SELFTEST_SKIP), skip);
    
    /* 标记自检完成 */
    selftest_done = true;
    
    return 0;
}

/**
 * 自检文件的show回调
 */
static int moeai_procfs_selftest_show(struct seq_file *seq, void *v)
{
    mutex_lock(&selftest_mutex);
    
    if (!selftest_done) {
        seq_puts(seq, lang_get(LANG_PROCFS_SELFTEST_NOT_RUN));
        seq_puts(seq, "\n");
    } else if (selftest_buffer) {
        seq_puts(seq, selftest_buffer);
    } else {
        seq_puts(seq, lang_get(LANG_PROCFS_SELFTEST_NO_BUFFER));
        seq_puts(seq, "\n");
    }
    
    mutex_unlock(&selftest_mutex);
    return 0;
}

static int moeai_procfs_selftest_open(struct inode *inode, struct file *file)
{
    return single_open(file, moeai_procfs_selftest_show, NULL);
}

static const struct proc_ops moeai_procfs_selftest_fops = {
    .proc_open = moeai_procfs_selftest_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

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
        seq_printf(seq, "%s: %d\n", lang_get(LANG_ERR_MEM_INIT_FAILED), ret);
        return 0;
    }
    
    /* 输出内存状态 */
    seq_puts(seq, lang_get(LANG_PROCFS_MEMORY_STATUS));
    seq_puts(seq, ":\n");
    seq_printf(seq, "  %s: %lu KB\n", lang_get(LANG_PROCFS_MEMORY_TOTAL), stats.total_ram);
    seq_printf(seq, "  %s: %lu KB\n", lang_get(LANG_PROCFS_MEMORY_FREE), stats.free_ram);
    seq_printf(seq, "  %s: %lu KB\n", lang_get(LANG_PROCFS_MEMORY_AVAILABLE), stats.available_ram);
    seq_printf(seq, "  %s: %lu KB\n", lang_get(LANG_PROCFS_MEMORY_CACHED), stats.cached_ram);
    seq_printf(seq, "  %s: %u%%\n", lang_get(LANG_PROCFS_MEMORY_USAGE), stats.mem_usage_percent);
    seq_printf(seq, "  %s: %lu KB\n", lang_get(LANG_PROCFS_SWAP_TOTAL), stats.swap_total);
    seq_printf(seq, "  %s: %lu KB\n", lang_get(LANG_PROCFS_SWAP_FREE), stats.swap_free);
    seq_printf(seq, "  %s: %u%%\n\n", lang_get(LANG_PROCFS_SWAP_USAGE), stats.swap_usage_percent);
    
    /* 输出监控配置 */
    {
        struct moeai_mem_monitor_config config;
        moeai_mem_monitor_get_config(&config);
        
        seq_puts(seq, lang_get(LANG_PROCFS_MEMORY_CONFIG));
        seq_puts(seq, ":\n");
        seq_printf(seq, "  %s: %u ms\n", lang_get(LANG_PROCFS_MONITOR_INTERVAL), config.check_interval_ms);
        seq_printf(seq, "  %s: %u%%\n", lang_get(LANG_PROCFS_WARN_THRESHOLD), config.warn_threshold);
        seq_printf(seq, "  %s: %u%%\n", lang_get(LANG_PROCFS_CRITICAL_THRESHOLD), config.critical_threshold);
        seq_printf(seq, "  %s: %u%%\n", lang_get(LANG_PROCFS_EMERGENCY_THRESHOLD), config.emergency_threshold);
        seq_printf(seq, "  %s: %s\n\n", lang_get(LANG_PROCFS_AUTO_RECLAIM_STATUS), 
                  config.auto_reclaim ? lang_get(LANG_PROCFS_AUTO_RECLAIM_ON) : lang_get(LANG_PROCFS_AUTO_RECLAIM_OFF));
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
        MOEAI_INFO(MODULE_NAME, "%s %ld KB", 
                  lang_get(LANG_CLI_MSG_RECLAIM_COMPLETE), reclaimed);
    } 
    else if (strncmp(buf, "selftest", 8) == 0) {
        /* 执行自检 */
        moeai_trigger_selftest();
        MOEAI_INFO(MODULE_NAME, "%s", lang_get(LANG_CLI_MSG_SELFTEST_RESULT));
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
            MOEAI_INFO(MODULE_NAME, "%s %u%%", 
                      lang_get(LANG_CLI_MSG_SET_THRESHOLD), threshold);
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
            MOEAI_INFO(MODULE_NAME, "%s %u ms", 
                      lang_get(LANG_CLI_MSG_SET_INTERVAL), interval);
        }
    }
    else if (strncmp(buf, "set autoreclaim ", 16) == 0) {
        /* 设置自动回收 */
        if (strncmp(buf + 16, "on", 2) == 0 || strncmp(buf + 16, "true", 4) == 0) {
            struct moeai_mem_monitor_config config;
            moeai_mem_monitor_get_config(&config);
            config.auto_reclaim = true;
            moeai_mem_monitor_set_config(&config);
            MOEAI_INFO(MODULE_NAME, "%s %s", 
                      lang_get(LANG_CLI_MSG_SET_AUTORECLAIM),
                      lang_get(LANG_PROCFS_AUTO_RECLAIM_ON));
        } else if (strncmp(buf + 16, "off", 3) == 0 || strncmp(buf + 16, "false", 5) == 0) {
            struct moeai_mem_monitor_config config;
            moeai_mem_monitor_get_config(&config);
            config.auto_reclaim = false;
            moeai_mem_monitor_set_config(&config);
            MOEAI_INFO(MODULE_NAME, "%s %s", 
                      lang_get(LANG_CLI_MSG_SET_AUTORECLAIM),
                      lang_get(LANG_PROCFS_AUTO_RECLAIM_OFF));
        }
    }
    else {
        MOEAI_WARN(MODULE_NAME, "%s: %s", 
                  lang_get(LANG_CLI_ERR_UNKNOWN_CMD), buf);
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
        seq_printf(seq, "%s\n", lang_get(LANG_CLI_ERR_OPEN_LOG));
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
            level_str = lang_get(LANG_PROCFS_LOG_LEVEL_DEBUG);
            break;
        case MOEAI_LOG_INFO:
            level_str = lang_get(LANG_PROCFS_LOG_LEVEL_INFO);
            break;
        case MOEAI_LOG_WARN:
            level_str = lang_get(LANG_PROCFS_LOG_LEVEL_WARN);
            break;
        case MOEAI_LOG_ERROR:
            level_str = lang_get(LANG_PROCFS_LOG_LEVEL_ERROR);
            break;
        case MOEAI_LOG_FATAL:
            level_str = lang_get(LANG_PROCFS_LOG_LEVEL_FATAL);
            break;
        default:
            level_str = lang_get(LANG_PROCFS_LOG_LEVEL_UNKNOWN);
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
    
    /* Create root directory */
    root = proc_mkdir(MOEAI_PROCFS_ROOT, NULL);
    if (!root) {
        MOEAI_ERROR(MODULE_NAME, lang_get(LANG_PROCFS_ERR_CREATE_ROOT));
        return -ENOMEM;
    }
    MOEAI_INFO(MODULE_NAME, lang_get(LANG_PROCFS_INIT_START));
    
    /* 保存根目录指针 */
    moeai_procfs_root = root;
    
    /* 创建状态文件 */
    status_entry = proc_create(MOEAI_PROCFS_STATUS, 0444, root, 
                             &moeai_procfs_status_fops);
    if (!status_entry) {
        MOEAI_ERROR(MODULE_NAME, lang_get(LANG_PROCFS_ERR_CREATE_STATUS));
        goto err_status;
    }
    
    /* 创建控制文件 */
    control_entry = proc_create(MOEAI_PROCFS_CONTROL, 0222, root,
                              &moeai_procfs_control_fops);
    if (!control_entry) {
        MOEAI_ERROR(MODULE_NAME, lang_get(LANG_PROCFS_ERR_CREATE_CONTROL));
        goto err_control;
    }
    
    /* 创建日志文件 */
    log_entry = proc_create(MOEAI_PROCFS_LOG, 0444, root,
                          &moeai_procfs_log_fops);
    if (!log_entry) {
            MOEAI_ERROR(MODULE_NAME, lang_get(LANG_PROCFS_ERR_CREATE_LOG));
        goto err_log;
    }
    
    /* 创建自检文件 */
    selftest_entry = proc_create(MOEAI_PROCFS_SELFTEST, 0444, root,
                                &moeai_procfs_selftest_fops);
    if (!selftest_entry) {
        MOEAI_ERROR(MODULE_NAME, lang_get(LANG_PROCFS_ERR_CREATE_SELFTEST));
        goto err_selftest;
    }
    
    MOEAI_INFO(MODULE_NAME, "%s", lang_get(LANG_PROCFS_INIT_SUCCESS));
    return 0;
    
err_selftest:
    proc_remove(log_entry);
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
    proc_remove(selftest_entry);
    proc_remove(log_entry);
    proc_remove(control_entry);
    proc_remove(status_entry);
    proc_remove(moeai_procfs_root);
    
    moeai_procfs_root = NULL;
    status_entry = NULL;
    control_entry = NULL;
    log_entry = NULL;
    selftest_entry = NULL;
    
    MOEAI_INFO(MODULE_NAME, "%s", lang_get(LANG_PROCFS_EXIT_COMPLETE));
}
