/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/core/scheduler.h
 * 描述: 调度器和事件管理接口
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef MOEAI_SCHEDULER_H
#define MOEAI_SCHEDULER_H

#include <linux/types.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/completion.h>

#include "./module.h"

/* 事件类型枚举 */
enum moeai_event_type {
    MOEAI_EVENT_NONE = 0,        /* 无事件 */
    MOEAI_EVENT_MEMORY_PRESSURE, /* 内存压力事件 */
    MOEAI_EVENT_NETWORK_ALERT,   /* 网络异常事件 */
    MOEAI_EVENT_FS_ACTIVITY,     /* 文件系统活动事件 */
    MOEAI_EVENT_SYSTEM_LOAD,     /* 系统负载事件 */
    MOEAI_EVENT_USER_COMMAND,    /* 用户命令事件 */
    MOEAI_EVENT_TIMER,           /* 定时器事件 */
    MOEAI_EVENT_MODULE_STATE,    /* 模块状态变更事件 */
    MOEAI_EVENT_CUSTOM,          /* 自定义事件（模块特定） */
    MOEAI_EVENT_MAX              /* 用于边界检查 */
};

/* 事件优先级枚举 */
enum moeai_event_priority {
    MOEAI_PRIORITY_LOW = 0,      /* 低优先级 */
    MOEAI_PRIORITY_NORMAL,       /* 普通优先级 */
    MOEAI_PRIORITY_HIGH,         /* 高优先级 */
    MOEAI_PRIORITY_CRITICAL      /* 关键优先级 */
};

/* 事件结构体定义 */
struct moeai_event {
    enum moeai_event_type type;        /* 事件类型 */
    enum moeai_event_priority priority; /* 事件优先级 */
    struct timespec64 timestamp;       /* 事件产生时间 */
    void *data;                        /* 事件数据指针 */
    size_t data_size;                  /* 事件数据大小 */
    char source[32];                   /* 事件源（模块名或其他标识） */
    struct completion *done;           /* 事件处理完成信号 */
    void (*free_fn)(void *data);       /* 数据释放函数 */
};

/* 调度器状态枚举 */
enum moeai_scheduler_state {
    MOEAI_SCHEDULER_STOPPED = 0, /* 已停止 */
    MOEAI_SCHEDULER_RUNNING,     /* 正在运行 */
    MOEAI_SCHEDULER_PAUSED       /* 已暂停 */
};

/* 调度器回调函数类型 */
typedef int (*moeai_scheduler_event_handler_fn)(struct moeai_event *event);

/**
 * 初始化调度器
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_init(void);

/**
 * 清理调度器
 */
void moeai_scheduler_exit(void);

/**
 * 启动调度器
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_start(void);

/**
 * 停止调度器
 */
void moeai_scheduler_stop(void);

/**
 * 暂停调度器
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_pause(void);

/**
 * 恢复调度器
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_resume(void);

/**
 * 获取调度器状态
 * @return 调度器当前状态
 */
enum moeai_scheduler_state moeai_scheduler_get_state(void);

/**
 * 创建新事件
 * @param type 事件类型
 * @param priority 事件优先级
 * @param data 事件数据指针
 * @param data_size 事件数据大小
 * @param source 事件源标识
 * @param free_fn 数据释放函数，NULL表示不需要释放
 * @return 成功返回事件指针，失败返回NULL
 */
struct moeai_event *moeai_event_create(
    enum moeai_event_type type,
    enum moeai_event_priority priority,
    void *data,
    size_t data_size,
    const char *source,
    void (*free_fn)(void *data)
);

/**
 * 销毁事件
 * @param event 事件指针
 */
void moeai_event_destroy(struct moeai_event *event);

/**
 * 发布事件到调度器
 * @param event 事件指针
 * @param wait 是否等待事件处理完成
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_post_event(struct moeai_event *event, bool wait);

/**
 * 注册事件处理器
 * @param type 事件类型
 * @param handler 处理函数
 * @param module 关联的模块，NULL表示系统级处理器
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_register_handler(
    enum moeai_event_type type,
    moeai_scheduler_event_handler_fn handler,
    struct moeai_module *module
);

/**
 * 注销事件处理器
 * @param type 事件类型
 * @param handler 处理函数
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_unregister_handler(
    enum moeai_event_type type,
    moeai_scheduler_event_handler_fn handler
);

/**
 * 获取事件类型的字符串表示
 * @param type 事件类型
 * @return 事件类型的字符串表示
 */
const char *moeai_event_type_string(enum moeai_event_type type);

/**
 * 获取事件优先级的字符串表示
 * @param priority 事件优先级
 * @return 事件优先级的字符串表示
 */
const char *moeai_event_priority_string(enum moeai_event_priority priority);

#endif /* MOEAI_SCHEDULER_H */