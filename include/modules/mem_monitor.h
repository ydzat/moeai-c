/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/modules/mem_monitor.h
 * 描述: 内存监控模块接口 (MVP简化版)
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef _MOEAI_MEM_MONITOR_H
#define _MOEAI_MEM_MONITOR_H

#include <linux/types.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/spinlock.h>

/* 内存回收策略枚举 */
enum moeai_mem_reclaim_policy {
    MOEAI_MEM_RECLAIM_GENTLE = 0,    /* 温和回收 - 仅释放文件缓存 */
    MOEAI_MEM_RECLAIM_MODERATE = 1,  /* 中等回收 - 释放所有可回收页面 */
    MOEAI_MEM_RECLAIM_AGGRESSIVE = 2 /* 积极回收 - 强制内存紧急回收，可能触发OOM */
};

/* 内存统计结构体 */
struct moeai_mem_stats {
    struct timespec64 timestamp;  /* 统计时间戳 */
    unsigned long total_ram;      /* 总物理内存 (KB) */
    unsigned long free_ram;       /* 空闲内存 (KB) */
    unsigned long available_ram;  /* 可用内存 (KB) */
    unsigned long cached_ram;     /* 缓存内存 (KB) */
    unsigned int mem_usage_percent; /* 内存使用百分比 */
    unsigned long swap_total;     /* 交换空间总量 (KB) */
    unsigned long swap_free;      /* 空闲交换空间 (KB) */
    unsigned int swap_usage_percent; /* 交换空间使用百分比 */
};

/* 内存监控配置结构体 */
struct moeai_mem_monitor_config {
    unsigned int check_interval_ms; /* 检查间隔 (毫秒) */
    unsigned int warn_threshold;    /* 警告阈值 (百分比) */
    unsigned int critical_threshold; /* 临界阈值 (百分比) */
    unsigned int emergency_threshold; /* 紧急阈值 (百分比) */
    bool auto_reclaim;              /* 自动回收标志 */
};

/* 内存监控模块API */
int moeai_mem_monitor_init(void);
void moeai_mem_monitor_exit(void);
int moeai_mem_monitor_start(void);
void moeai_mem_monitor_stop(void);
int moeai_mem_monitor_get_stats(struct moeai_mem_stats *stats);
int moeai_mem_monitor_get_config(struct moeai_mem_monitor_config *config);
int moeai_mem_monitor_set_config(const struct moeai_mem_monitor_config *config);
long moeai_mem_reclaim(enum moeai_mem_reclaim_policy policy);

#endif /* _MOEAI_MEM_MONITOR_H */