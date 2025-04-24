/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/core/state.h
 * 描述: 系统状态管理接口
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef MOEAI_STATE_H
#define MOEAI_STATE_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>

/* 系统状态类型 */
enum moeai_system_state {
    MOEAI_STATE_NORMAL = 0,    /* 正常状态 */
    MOEAI_STATE_WARNING,       /* 警告状态 */
    MOEAI_STATE_CRITICAL,      /* 临界状态 */
    MOEAI_STATE_EMERGENCY      /* 紧急状态 */
};

/* 资源类型 */
enum moeai_resource_type {
    MOEAI_RESOURCE_MEMORY = 0,  /* 内存资源 */
    MOEAI_RESOURCE_CPU,         /* CPU资源 */
    MOEAI_RESOURCE_NETWORK,     /* 网络资源 */
    MOEAI_RESOURCE_IO,          /* IO资源 */
    MOEAI_RESOURCE_MAX          /* 边界检查 */
};

/* 资源阈值结构体 */
struct moeai_threshold {
    unsigned int warning;       /* 警告阈值（百分比） */
    unsigned int critical;      /* 临界阈值（百分比） */
    unsigned int emergency;     /* 紧急阈值（百分比） */
};

/* 系统状态结构体 */
struct moeai_system_state_mgr {
    atomic_t resource_usage[MOEAI_RESOURCE_MAX];  /* 资源使用率 */
    struct moeai_threshold thresholds[MOEAI_RESOURCE_MAX]; /* 资源阈值 */
    enum moeai_system_state system_state;      /* 整体系统状态 */
    spinlock_t lock;                        /* 保护状态更新的自旋锁 */
};

/**
 * 初始化系统状态管理器
 * @return 成功返回0，失败返回错误码
 */
int moeai_state_init(void);

/**
 * 清理系统状态管理器
 */
void moeai_state_exit(void);

/**
 * 更新资源使用率
 * @param resource 资源类型
 * @param usage 资源使用率（百分比）
 * @return 成功返回0，失败返回错误码
 */
int moeai_state_update_resource(enum moeai_resource_type resource, unsigned int usage);

/**
 * 获取资源使用率
 * @param resource 资源类型
 * @return 资源使用率（百分比）
 */
unsigned int moeai_state_get_resource_usage(enum moeai_resource_type resource);

/**
 * 设置资源阈值
 * @param resource 资源类型
 * @param warning 警告阈值（百分比）
 * @param critical 临界阈值（百分比）
 * @param emergency 紧急阈值（百分比）
 * @return 成功返回0，失败返回错误码
 */
int moeai_state_set_thresholds(
    enum moeai_resource_type resource,
    unsigned int warning,
    unsigned int critical,
    unsigned int emergency
);

/**
 * 获取当前系统状态
 * @return 系统状态
 */
enum moeai_system_state moeai_state_get_system_state(void);

/**
 * 根据资源使用率和阈值评估系统状态
 * 此函数会更新全局系统状态
 * @return 评估后的系统状态
 */
enum moeai_system_state moeai_state_evaluate(void);

/**
 * 获取资源类型的字符串表示
 * @param resource 资源类型
 * @return 资源类型的字符串表示
 */
const char *moeai_resource_type_string(enum moeai_resource_type resource);

/**
 * 获取系统状态的字符串表示
 * @param state 系统状态
 * @return 系统状态的字符串表示
 */
const char *moeai_system_state_string(enum moeai_system_state state);

#endif /* MOEAI_STATE_H */