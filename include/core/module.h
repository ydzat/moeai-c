/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/core/module.h
 * 描述: 功能模块的抽象接口与管理系统
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef MOEAI_MODULE_H
#define MOEAI_MODULE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#include "core/state.h"

/* 定义模块状态枚举 */
enum moeai_module_state {
    MOEAI_MODULE_UNLOADED = 0,  /* 未加载 */
    MOEAI_MODULE_LOADED,        /* 已加载但未初始化 */
    MOEAI_MODULE_INITIALIZED,   /* 已初始化并就绪 */
    MOEAI_MODULE_RUNNING,       /* 正在运行 */
    MOEAI_MODULE_PAUSED,        /* 已暂停 */
    MOEAI_MODULE_ERROR          /* 错误状态 */
};

/**
 * 模块回调函数类型定义
 */
struct moeai_module;

/* 模块初始化回调 */
typedef int (*moeai_module_init_fn)(struct moeai_module *module);

/* 模块清理回调 */
typedef void (*moeai_module_exit_fn)(struct moeai_module *module);

/* 模块启动回调 */
typedef int (*moeai_module_start_fn)(struct moeai_module *module);

/* 模块停止回调 */
typedef int (*moeai_module_stop_fn)(struct moeai_module *module);

/* 模块处理事件回调 */
typedef int (*moeai_module_handle_event_fn)(struct moeai_module *module, void *event_data);

/* 模块处理命令回调 */
typedef int (*moeai_module_handle_command_fn)(struct moeai_module *module, const char *cmd, size_t len);

/**
 * 模块结构体定义
 */
struct moeai_module {
    char name[32];                         /* 模块名称 */
    char description[128];                 /* 模块描述 */
    enum moeai_module_state state;         /* 模块状态 */
    unsigned int priority;                 /* 模块优先级（越小越高） */
    struct list_head list;                 /* 用于模块链表的节点 */
    
    /* 模块回调函数 */
    moeai_module_init_fn init;             /* 初始化函数 */
    moeai_module_exit_fn exit;             /* 清理函数 */
    moeai_module_start_fn start;           /* 启动函数 */
    moeai_module_stop_fn stop;             /* 停止函数 */
    moeai_module_handle_event_fn handle_event; /* 事件处理函数 */
    moeai_module_handle_command_fn handle_command; /* 命令处理函数 */
    
    void *private_data;                    /* 模块私有数据指针 */
    spinlock_t lock;                       /* 保护模块状态的自旋锁 */
};

/**
 * 模块管理器初始化
 * @return 成功返回0，失败返回错误码
 */
int moeai_module_manager_init(void);

/**
 * 模块管理器清理
 */
void moeai_module_manager_exit(void);

/**
 * 注册功能模块
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_module_register(struct moeai_module *module);

/**
 * 注销功能模块
 * @param module 模块指针
 */
void moeai_module_unregister(struct moeai_module *module);

/**
 * 通过名称查找模块
 * @param name 模块名称
 * @return 成功返回模块指针，失败返回NULL
 */
struct moeai_module *moeai_module_find(const char *name);

/**
 * 启动所有已注册模块
 * @return 成功返回0，失败返回错误码
 */
int moeai_module_start_all(void);

/**
 * 停止所有已注册模块
 */
void moeai_module_stop_all(void);

/**
 * 向模块发送事件
 * @param module 模块指针
 * @param event_data 事件数据
 * @return 成功返回0，失败返回错误码
 */
int moeai_module_send_event(struct moeai_module *module, void *event_data);

/**
 * 向模块发送命令
 * @param module 模块指针
 * @param cmd 命令字符串
 * @param len 命令字符串长度
 * @return 成功返回0，失败返回错误码
 */
int moeai_module_send_command(struct moeai_module *module, const char *cmd, size_t len);

/**
 * 获取模块状态字符串
 * @param state 模块状态
 * @return 状态的字符串表示
 */
const char *moeai_module_state_string(enum moeai_module_state state);

#endif /* MOEAI_MODULE_H */