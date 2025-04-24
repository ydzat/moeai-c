# MoeAI-C 功能模块设计

## 概述

功能模块是MoeAI-C的专用监控和管理组件，每个模块专注于特定的系统资源或功能领域。功能模块位于`src/modules/`目录下，包含以下主要组件：

1. 内存监控模块 (`mem_monitor.c`)
2. 网络防护模块 (`net_guard.c`) 
3. 文件系统日志模块 (`fs_logger.c`)

每个功能模块都遵循统一的模块化设计，通过核心模块提供的注册机制接入系统，使用事件机制进行通信，并按需访问系统状态。

## 1. 模块通用结构

每个功能模块都需要实现以下通用接口：

```c
/* 模块初始化函数 */
int moeai_<模块名>_init(moeai_module_t *module);

/* 模块清理函数 */
void moeai_<模块名>_exit(moeai_module_t *module);

/* 模块启动函数 */
int moeai_<模块名>_start(moeai_module_t *module);

/* 模块停止函数 */
int moeai_<模块名>_stop(moeai_module_t *module);

/* 模块事件处理函数 */
int moeai_<模块名>_handle_event(moeai_module_t *module, void *event_data);

/* 模块命令处理函数 */
int moeai_<模块名>_handle_command(moeai_module_t *module, const char *cmd, size_t len);

/* 模块实例创建函数 */
moeai_module_t *moeai_<模块名>_create(void);
```

## 2. 内存监控模块 (`mem_monitor.c`)

### 2.1 职责

- 监控系统内存使用情况
- 在达到预设阈值时触发警告事件
- 提供内存回收机制
- 支持保护关键进程不被OOM Killer杀死

### 2.2 核心数据结构

```c
/* 内存监控模块私有数据 */
struct moeai_mem_monitor_private {
    moeai_mem_monitor_config_t config;    /* 模块配置 */
    moeai_mem_stats_t current_stats;      /* 当前内存统计 */
    struct timer_list check_timer;        /* 定期检查定时器 */
    rwlock_t config_lock;                 /* 配置读写锁 */
    spinlock_t stats_lock;                /* 统计数据锁 */
    bool monitoring_active;               /* 监控活动标志 */
    struct list_head protected_procs;     /* 受保护进程链表 */
};

/* 受保护进程结构 */
struct moeai_protected_proc {
    char name[MOEAI_MAX_NAME_LEN];        /* 进程名称 */
    pid_t pid;                            /* 进程ID（如果已知） */
    unsigned int score_adj;               /* OOM调整分数 */
    struct list_head list;                /* 链表节点 */
};
```

### 2.3 接口定义

```c
/**
 * 初始化内存监控模块
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_init(moeai_module_t *module);

/**
 * 清理内存监控模块
 * @param module 模块指针
 */
void moeai_mem_monitor_exit(moeai_module_t *module);

/**
 * 启动内存监控
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_start(moeai_module_t *module);

/**
 * 停止内存监控
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_stop(moeai_module_t *module);

/**
 * 获取当前内存状态
 * @param stats 用于存储状态的结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_get_stats(moeai_mem_stats_t *stats);

/**
 * 执行内存回收
 * @param policy 回收策略
 * @return 成功回收的内存大小(KB)，失败返回负数错误码
 */
long moeai_mem_reclaim(moeai_mem_reclaim_policy_t policy);

/**
 * 更新内存监控配置
 * @param config 配置结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_set_config(const moeai_mem_monitor_config_t *config);

/**
 * 获取内存监控配置
 * @param config 用于存储配置的结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_get_config(moeai_mem_monitor_config_t *config);

/**
 * 添加受保护进程
 * @param name 进程名
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_protect_process(const char *name);

/**
 * 移除受保护进程
 * @param name 进程名
 * @return 成功返回0，失败返回错误码
 */
int moeai_mem_monitor_unprotect_process(const char *name);
```

### 2.4 模块处理流程

1. **初始化阶段**：
   - 设置默认配置（检查间隔、阈值、回收策略）
   - 初始化内存状态结构
   - 创建定时器

2. **监控阶段**：
   - 定期采集内存统计信息（总内存、空闲内存、可用内存、缓存、交换等）
   - 计算内存使用率并与阈值比较
   - 在超过阈值时触发相应级别的事件

3. **处理阶段**：
   - 根据配置决定是否自动回收内存
   - 根据资源压力等级选择回收策略（轻度、中度、重度）
   - 在严重缺乏时通知用户态代理做出更高级决策

4. **保护机制**：
   - 维护受保护进程列表
   - 为受保护进程设置低OOM分数，降低被kill概率
   - 提供保护/解除保护API给用户态访问

## 3. 网络防护模块 (`net_guard.c`)

### 3.1 职责

- 监控系统网络连接状态
- 检测异常流量模式和可疑连接
- 提供基本的连接限制功能
- 记录网络活动日志

### 3.2 核心数据结构

```c
/* 网络监控模块私有数据 */
struct moeai_net_guard_private {
    struct moeai_net_guard_config config;  /* 模块配置 */
    struct list_head watched_ports;        /* 被监控的端口列表 */
    struct list_head blocked_ips;          /* 被阻止的IP列表 */
    struct timer_list check_timer;         /* 定期检查定时器 */
    struct netlink_kernel_cfg nl_cfg;      /* Netfilter配置 */
    struct sock *nl_sock;                  /* Netlink套接字 */
    rwlock_t config_lock;                  /* 配置读写锁 */
    spinlock_t conn_lock;                  /* 连接列表锁 */
    bool monitoring_active;                /* 监控活动标志 */
};

/* 网络连接记录结构 */
struct moeai_net_connection {
    __be32 src_ip;                         /* 源IP */
    __be32 dst_ip;                         /* 目标IP */
    __be16 src_port;                       /* 源端口 */
    __be16 dst_port;                       /* 目标端口 */
    u8 protocol;                           /* 协议 */
    unsigned long timestamp;               /* 连接时间戳 */
    unsigned int packets;                  /* 数据包计数 */
    unsigned long bytes;                   /* 字节计数 */
    struct list_head list;                 /* 链表节点 */
};
```

### 3.3 接口定义

```c
/**
 * 初始化网络防护模块
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_init(moeai_module_t *module);

/**
 * 清理网络防护模块
 * @param module 模块指针
 */
void moeai_net_guard_exit(moeai_module_t *module);

/**
 * 启动网络监控
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_start(moeai_module_t *module);

/**
 * 停止网络监控
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_stop(moeai_module_t *module);

/**
 * 设置网络防护配置
 * @param config 配置结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_set_config(const struct moeai_net_guard_config *config);

/**
 * 获取网络防护配置
 * @param config 用于存储配置的结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_get_config(struct moeai_net_guard_config *config);

/**
 * 添加监控端口
 * @param port 端口号
 * @param protocol 协议（TCP/UDP）
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_watch_port(unsigned short port, int protocol);

/**
 * 移除监控端口
 * @param port 端口号
 * @param protocol 协议（TCP/UDP）
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_unwatch_port(unsigned short port, int protocol);

/**
 * 阻止指定IP
 * @param ip IP地址（网络字节序）
 * @param mask 子网掩码（掩码位数）
 * @param duration_sec 阻止持续时间（秒），0表示永久
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_block_ip(__be32 ip, int mask, unsigned int duration_sec);

/**
 * 解除IP阻止
 * @param ip IP地址（网络字节序）
 * @param mask 子网掩码（掩码位数）
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_unblock_ip(__be32 ip, int mask);

/**
 * 获取当前网络连接统计
 * @param stats 用于存储统计信息的结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_net_guard_get_stats(struct moeai_net_stats *stats);
```

## 4. 文件系统日志模块 (`fs_logger.c`)

### 4.1 职责

- 监控文件系统关键操作
- 记录文件的创建、修改、删除等事件
- 监视特定目录和文件的变化
- 检测可能的异常文件操作

### 4.2 核心数据结构

```c
/* 文件系统监控模块私有数据 */
struct moeai_fs_logger_private {
    struct moeai_fs_logger_config config;  /* 模块配置 */
    struct list_head watch_paths;          /* 监视路径列表 */
    struct fsnotify_group *notify_group;   /* fsnotify组 */
    rwlock_t config_lock;                  /* 配置读写锁 */
    spinlock_t events_lock;                /* 事件列表锁 */
    struct list_head recent_events;        /* 最近事件列表 */
    int event_count;                       /* 事件计数器 */
    bool monitoring_active;                /* 监控活动标志 */
};

/* 监视路径结构 */
struct moeai_watch_path {
    char path[MOEAI_MAX_PATH_LEN];         /* 监视路径 */
    unsigned int mask;                     /* 事件掩码 */
    bool recursive;                        /* 是否递归监视 */
    struct list_head list;                 /* 链表节点 */
};

/* 文件系统事件结构 */
struct moeai_fs_event {
    char path[MOEAI_MAX_PATH_LEN];         /* 事件路径 */
    unsigned int mask;                     /* 事件掩码 */
    struct timespec64 timestamp;           /* 事件时间戳 */
    struct list_head list;                 /* 链表节点 */
};
```

### 4.3 接口定义

```c
/**
 * 初始化文件系统日志模块
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_init(moeai_module_t *module);

/**
 * 清理文件系统日志模块
 * @param module 模块指针
 */
void moeai_fs_logger_exit(moeai_module_t *module);

/**
 * 启动文件系统监控
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_start(moeai_module_t *module);

/**
 * 停止文件系统监控
 * @param module 模块指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_stop(moeai_module_t *module);

/**
 * 添加监视路径
 * @param path 路径
 * @param mask 事件掩码
 * @param recursive 是否递归监视
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_add_watch(const char *path, unsigned int mask, bool recursive);

/**
 * 移除监视路径
 * @param path 路径
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_remove_watch(const char *path);

/**
 * 设置文件系统日志配置
 * @param config 配置结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_set_config(const struct moeai_fs_logger_config *config);

/**
 * 获取文件系统日志配置
 * @param config 用于存储配置的结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_get_config(struct moeai_fs_logger_config *config);

/**
 * 获取最近的文件系统事件
 * @param events 用于存储事件的数组
 * @param max_count 数组的最大容量
 * @param actual_count 实际返回的事件数量
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_get_recent_events(
    struct moeai_fs_event *events,
    int max_count,
    int *actual_count
);

/**
 * 清除最近的文件系统事件
 * @return 成功返回0，失败返回错误码
 */
int moeai_fs_logger_clear_events(void);
```

## 5. 功能模块的注册与初始化流程

每个功能模块都通过以下流程注册到系统核心：

1. **模块定义**：使用`moeai_module_t`结构定义模块属性和回调函数
2. **创建实例**：调用`moeai_<模块名>_create()`创建模块实例
3. **注册到核心**：调用`moeai_module_register()`注册模块
4. **初始化**：内核核心调用模块的`init`回调函数
5. **启动**：内核核心调用模块的`start`回调函数

例如，在`src/core/init.c`中会有类似如下的模块注册代码：

```c
/* 注册所有功能模块 */
static int moeai_register_modules(void)
{
    moeai_module_t *mem_monitor = NULL;
    moeai_module_t *net_guard = NULL;
    moeai_module_t *fs_logger = NULL;
    int ret = 0;

    /* 创建并注册内存监控模块 */
    mem_monitor = moeai_mem_monitor_create();
    if (!mem_monitor) {
        MOEAI_ERROR("core", "Failed to create memory monitor module");
        ret = -ENOMEM;
        goto err;
    }
    
    ret = moeai_module_register(mem_monitor);
    if (ret) {
        MOEAI_ERROR("core", "Failed to register memory monitor module");
        goto err_mem;
    }

    /* 创建并注册网络防护模块 */
    net_guard = moeai_net_guard_create();
    if (!net_guard) {
        MOEAI_ERROR("core", "Failed to create network guard module");
        ret = -ENOMEM;
        goto err_net;
    }
    
    ret = moeai_module_register(net_guard);
    if (ret) {
        MOEAI_ERROR("core", "Failed to register network guard module");
        goto err_net_reg;
    }

    /* 创建并注册文件系统日志模块 */
    fs_logger = moeai_fs_logger_create();
    if (!fs_logger) {
        MOEAI_ERROR("core", "Failed to create filesystem logger module");
        ret = -ENOMEM;
        goto err_fs;
    }
    
    ret = moeai_module_register(fs_logger);
    if (ret) {
        MOEAI_ERROR("core", "Failed to register filesystem logger module");
        goto err_fs_reg;
    }

    return 0;

err_fs_reg:
    moeai_kfree(fs_logger);
err_fs:
    moeai_module_unregister(net_guard);
err_net_reg:
    moeai_kfree(net_guard);
err_net:
    moeai_module_unregister(mem_monitor);
err_mem:
    moeai_kfree(mem_monitor);
err:
    return ret;
}
```

## 6. 功能模块之间的交互

功能模块不直接相互调用，而是通过核心模块提供的事件系统进行间接通信：

1. **发布事件**：模块将检测到的状态变化封装为事件，通过调度器发布
2. **事件处理**：其他模块注册感兴趣的事件处理器，当事件发生时被调用
3. **共享状态**：通过状态管理器共享全局系统状态

例如，内存监控模块可能向调度器发送内存压力事件，网络防护模块订阅该事件从而在内存紧张时调整其行为。

## 7. 功能模块与核心模块的关系

功能模块与核心模块之间的关系如下：

1. **依赖关系**：功能模块依赖于核心的模块管理和事件调度
2. **回调通知**：核心模块通过回调在生命周期关键点通知功能模块
3. **资源共享**：核心模块提供共享工具（日志、缓冲区等）给功能模块使用
4. **接口一致**：所有功能模块都需要实现相同的接口约定

## 8. 功能模块与用户态的交互

功能模块通过通信模块（procfs/netlink）与用户态交互：

1. **命令接收**：接收来自CLI工具或agent的命令并执行对应操作
2. **状态汇报**：将自身状态和事件记录到procfs或通过netlink发送
3. **配置更新**：允许用户态更改监控配置（如阈值、检查频率等）
4. **查询支持**：响应用户态的状态查询请求