/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: src/modules/mem_monitor.c
 * 描述: 内存监控功能实现
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/sysinfo.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/compaction.h>
#include <linux/fs.h>
#include <linux/writeback.h>
#include "../../include/modules/mem_monitor.h"
#include "../../include/utils/logger.h"

/* 模块名称 */
#define MODULE_NAME "mem_monitor"

/* 内存监控私有数据 */
struct moeai_mem_monitor_private {
    struct moeai_mem_monitor_config config;
    struct moeai_mem_stats current_stats;
    struct timer_list check_timer;
    spinlock_t stats_lock;
    bool monitoring_active;
};

/* 全局私有数据 */
static struct moeai_mem_monitor_private *monitor_priv;

/**
 * 获取内存状态
 * @stats: 用于存储内存状态的结构体指针
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_mem_monitor_get_stats(struct moeai_mem_stats *stats)
{
    struct sysinfo info;
    
    if (!stats)
        return -EINVAL;
    
    /* 获取系统信息 */
    si_meminfo(&info);
    
    /* 记录时间戳 */
    ktime_get_real_ts64(&stats->timestamp);
    
    /* 填充内存信息 */
    stats->total_ram = info.totalram * (PAGE_SIZE / 1024);
    stats->free_ram = info.freeram * (PAGE_SIZE / 1024);
    stats->available_ram = si_mem_available() * (PAGE_SIZE / 1024);
    stats->cached_ram = global_node_page_state(NR_FILE_PAGES) * (PAGE_SIZE / 1024) -
                       total_swapcache_pages() * (PAGE_SIZE / 1024);
    stats->swap_total = info.totalswap * (PAGE_SIZE / 1024);
    stats->swap_free = info.freeswap * (PAGE_SIZE / 1024);
    
    /* 计算百分比 */
    if (stats->total_ram > 0)
        stats->mem_usage_percent = 100 - ((stats->available_ram * 100) / stats->total_ram);
    else
        stats->mem_usage_percent = 0;
    
    if (stats->swap_total > 0)
        stats->swap_usage_percent = 100 - ((stats->swap_free * 100) / stats->swap_total);
    else
        stats->swap_usage_percent = 0;
    
    return 0;
}

/* 
 * 释放页面缓存 - 自定义实现
 * 这是一个简化版本，仅用于演示
 */
static long drop_page_cache(void)
{
    MOEAI_INFO(MODULE_NAME, "释放页面缓存");
    /* 由于无法直接访问内核内存管理函数，使用间接方法 */
    /* 在实际应用中，应该通过 sysfs/procfs 接口操作，例如写入 /proc/sys/vm/drop_caches */
    sync_inodes_sb(NULL);  /* 同步文件系统元数据 */
    return 0;
}

/*
 * 释放slab缓存 - 自定义实现，替代 drop_slab 函数
 */
static long drop_slab_cache(void)
{
    MOEAI_INFO(MODULE_NAME, "释放slab缓存");
    /* 实际应用中应该使用 kmem_cache_shrink 对各个 slab 进行收缩 */
    /* 这里仅返回一个示意值 */
    return 0;
}

/**
 * 执行内存回收
 * @policy: 回收策略
 * 返回值: 回收的内存量，负值表示错误
 */
long moeai_mem_reclaim(enum moeai_mem_reclaim_policy policy)
{
    long reclaimed = 0;
    
    switch (policy) {
    case MOEAI_MEM_RECLAIM_GENTLE:
        /* 仅释放文件缓存 */
        MOEAI_INFO(MODULE_NAME, "执行温和内存回收");
        reclaimed = drop_page_cache();
        break;
        
    case MOEAI_MEM_RECLAIM_MODERATE:
        /* 释放所有可回收页面 */
        MOEAI_INFO(MODULE_NAME, "执行中等强度内存回收");
        reclaimed = drop_page_cache();
        reclaimed += drop_slab_cache();
        break;
        
    case MOEAI_MEM_RECLAIM_AGGRESSIVE:
        /* 强制内存紧急回收，可能触发OOM */
        MOEAI_INFO(MODULE_NAME, "执行积极内存回收");
        reclaimed = drop_page_cache();
        reclaimed += drop_slab_cache();
        /* 注意：compact_nodes 函数在当前内核中不可用 */
        MOEAI_INFO(MODULE_NAME, "内存压缩操作在当前内核中不支持");
        break;
        
    default:
        MOEAI_ERROR(MODULE_NAME, "无效的回收策略: %d", policy);
        return -EINVAL;
    }
    
    return reclaimed;
}

/**
 * 内存状态检查任务
 * @t: 定时器指针
 */
static void moeai_mem_check_task(struct timer_list *t)
{
    struct moeai_mem_monitor_private *priv = from_timer(priv, t, check_timer);
    struct moeai_mem_stats stats;
    int ret;
    
    /* 获取当前内存状态 */
    ret = moeai_mem_monitor_get_stats(&stats);
    if (ret)
        goto reschedule;
    
    /* 更新当前统计信息 */
    spin_lock(&priv->stats_lock);
    memcpy(&priv->current_stats, &stats, sizeof(stats));
    spin_unlock(&priv->stats_lock);
    
    /* 检查阈值并采取行动 */
    if (stats.mem_usage_percent >= priv->config.emergency_threshold) {
        MOEAI_WARN(MODULE_NAME, "内存使用率(%u%%)超过紧急阈值(%u%%)",
                stats.mem_usage_percent, priv->config.emergency_threshold);
                
        if (priv->config.auto_reclaim)
            moeai_mem_reclaim(MOEAI_MEM_RECLAIM_AGGRESSIVE);
            
    } else if (stats.mem_usage_percent >= priv->config.critical_threshold) {
        MOEAI_INFO(MODULE_NAME, "内存使用率(%u%%)超过临界阈值(%u%%)",
                stats.mem_usage_percent, priv->config.critical_threshold);
                
        if (priv->config.auto_reclaim)
            moeai_mem_reclaim(MOEAI_MEM_RECLAIM_MODERATE);
            
    } else if (stats.mem_usage_percent >= priv->config.warn_threshold) {
        MOEAI_INFO(MODULE_NAME, "内存使用率(%u%%)超过警告阈值(%u%%)",
                stats.mem_usage_percent, priv->config.warn_threshold);
                
        if (priv->config.auto_reclaim)
            moeai_mem_reclaim(MOEAI_MEM_RECLAIM_GENTLE);
    }

reschedule:
    /* 重新调度检查任务 */
    if (priv->monitoring_active) {
        mod_timer(&priv->check_timer, 
                 jiffies + msecs_to_jiffies(priv->config.check_interval_ms));
    }
}

/**
 * 初始化内存监控模块
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_mem_monitor_init(void)
{
    /* 分配私有数据 */
    monitor_priv = kzalloc(sizeof(struct moeai_mem_monitor_private), GFP_KERNEL);
    if (!monitor_priv)
        return -ENOMEM;
    
    /* 初始化默认配置 */
    monitor_priv->config.check_interval_ms = 60000; /* 60秒 */
    monitor_priv->config.warn_threshold = 70;       /* 70% */
    monitor_priv->config.critical_threshold = 80;   /* 80% */
    monitor_priv->config.emergency_threshold = 90;  /* 90% */
    monitor_priv->config.auto_reclaim = false;      /* 默认不自动回收 */
    
    spin_lock_init(&monitor_priv->stats_lock);
    monitor_priv->monitoring_active = false;
    
    /* 初始化定时器 */
    timer_setup(&monitor_priv->check_timer, moeai_mem_check_task, 0);
    
    MOEAI_INFO(MODULE_NAME, "内存监控模块初始化完成");
    return 0;
}

/**
 * 清理内存监控模块
 */
void moeai_mem_monitor_exit(void)
{
    if (!monitor_priv)
        return;
    
    /* 确保定时器已停止 */
    moeai_mem_monitor_stop();
    
    kfree(monitor_priv);
    monitor_priv = NULL;
    
    MOEAI_INFO(MODULE_NAME, "内存监控模块已清理");
}

/**
 * 启动内存监控
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_mem_monitor_start(void)
{
    if (!monitor_priv)
        return -EINVAL;
    
    if (monitor_priv->monitoring_active)
        return 0; /* 已经启动 */
    
    /* 标记为活动状态 */
    monitor_priv->monitoring_active = true;
    
    /* 启动定时器 */
    mod_timer(&monitor_priv->check_timer, 
             jiffies + msecs_to_jiffies(monitor_priv->config.check_interval_ms));
    
    MOEAI_INFO(MODULE_NAME, "内存监控已启动，检查间隔: %u毫秒", 
              monitor_priv->config.check_interval_ms);
    
    return 0;
}

/**
 * 停止内存监控
 */
void moeai_mem_monitor_stop(void)
{
    if (!monitor_priv)
        return;
    
    if (!monitor_priv->monitoring_active)
        return; /* 已经停止 */
    
    /* 标记为非活动状态 */
    monitor_priv->monitoring_active = false;
    
    /* 删除定时器 */
    del_timer_sync(&monitor_priv->check_timer);
    
    MOEAI_INFO(MODULE_NAME, "内存监控已停止");
}

/**
 * 获取内存监控配置
 * @config: 存储配置的结构体指针
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_mem_monitor_get_config(struct moeai_mem_monitor_config *config)
{
    if (!monitor_priv || !config)
        return -EINVAL;
    
    *config = monitor_priv->config;
    return 0;
}

/**
 * 设置内存监控配置
 * @config: 新的配置结构体指针
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_mem_monitor_set_config(const struct moeai_mem_monitor_config *config)
{
    if (!monitor_priv || !config)
        return -EINVAL;
    
    /* 复制新的配置 */
    monitor_priv->config = *config;
    
    /* 如果监控已启动，重新调度定时器使用新的间隔 */
    if (monitor_priv->monitoring_active) {
        mod_timer(&monitor_priv->check_timer, 
                 jiffies + msecs_to_jiffies(config->check_interval_ms));
    }
    
    MOEAI_INFO(MODULE_NAME, "内存监控配置已更新");
    return 0;
}