# MoeAI-C 核心模块设计

## 概述

核心模块是MoeAI-C系统的中枢，负责管理其他模块的生命周期、处理事件分发，以及维护系统状态。核心模块位于`src/core/`目录下，由以下几个主要组件组成：

1. 模块管理系统 (`module_loader.c`)
2. 调度器与事件系统 (`scheduler.c`)
3. 状态管理器 (`state.h`实现部分)
4. 事件循环 (`event_loop.c`)
5. 初始化与清理 (`init.c`)

这些组件协同工作，为MoeAI-C系统提供稳定、高效的运行环境和基础架构。

## 1. 模块管理系统 (`module_loader.c`)

### 1.1 职责

- 管理功能模块的生命周期（加载、初始化、启动、停止、卸载）
- 提供模块注册和查找机制
- 处理模块间的依赖关系
- 维护模块状态和错误处理

### 1.2 核心数据结构

```c
/* 模块管理器上下文 */
struct moeai_module_manager {
    struct list_head modules;       /* 已注册模块链表 */
    spinlock_t lock;                /* 模块链表锁 */
    int module_count;               /* 已注册模块数量 */
    atomic_t initialized;           /* 初始化标志 */
};
```

### 1.3 接口定义

```c
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
int moeai_module_register(moeai_module_t *module);

/**
 * 注销功能模块
 * @param module 模块指针
 */
void moeai_module_unregister(moeai_module_t *module);

/**
 * 通过名称查找模块
 * @param name 模块名称
 * @return 成功返回模块指针，失败返回NULL
 */
moeai_module_t *moeai_module_find(const char *name);

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
int moeai_module_send_event(moeai_module_t *module, void *event_data);

/**
 * 向模块发送命令
 * @param module 模块指针
 * @param cmd 命令字符串
 * @param len 命令字符串长度
 * @return 成功返回0，失败返回错误码
 */
int moeai_module_send_command(moeai_module_t *module, const char *cmd, size_t len);

/**
 * 获取模块状态字符串
 * @param state 模块状态
 * @return 状态的字符串表示
 */
const char *moeai_module_state_string(moeai_module_state_t state);
```

### 1.4 模块生命周期

```
            +-------------+
            |  UNLOADED   |
            +------+------+
                   |
                   v
            +-------------+
 +--------->|   LOADED    |
 |          +------+------+
 |                 |
 |                 v
 |          +-------------+
 |     +--->|  INITIALIZED|<---+
 |     |    +------+------+    |
 |     |           |           |
 |     |           v           |
 |     |    +-------------+    |
 |     |    |   RUNNING   |    |
 |     |    +------+------+    |
 |     |           |           |
 |     |           v           |
 |     |    +-------------+    |
 |     +----+   PAUSED    +----+
 |          +------+------+
 |                 |
 |                 v
 |          +-------------+
 +----------+    ERROR    |
            +-------------+
```

模块状态转换规则：

1. **UNLOADED → LOADED**：注册模块后，模块处于已加载状态
2. **LOADED → INITIALIZED**：模块初始化成功后进入已初始化状态
3. **INITIALIZED → RUNNING**：调用启动函数后，模块进入运行状态
4. **RUNNING → PAUSED**：暂停模块运行，但不清理资源
5. **PAUSED → RUNNING**：恢复模块运行
6. **PAUSED → INITIALIZED**：停止模块运行，保持初始化状态
7. **任何状态 → ERROR**：出现错误时进入错误状态
8. **ERROR → LOADED**：错误恢复后重新加载模块

### 1.5 模块注册流程

1. 模块创建自身的`moeai_module_t`实例并设置回调函数
2. 调用`moeai_module_register`向模块管理器注册
3. 模块管理器检查模块名称是否唯一
4. 将模块添加到已注册模块链表
5. 检查是否需要自动初始化（如果模块管理器已经启动）

## 2. 调度器与事件系统 (`scheduler.c`)

### 2.1 职责

- 提供事件创建、发布和处理机制
- 根据事件优先级进行调度
- 管理事件处理器的注册
- 提供异步和同步事件处理模式
- 维护事件处理线程和工作队列

### 2.2 核心数据结构

```c
/* 事件处理器结构 */
struct moeai_event_handler {
    moeai_event_type_t type;                   /* 事件类型 */
    moeai_scheduler_event_handler_fn handler;  /* 处理函数 */
    moeai_module_t *module;                    /* 关联模块 */
    struct list_head list;                     /* 链表节点 */
};

/* 调度器上下文 */
struct moeai_scheduler_ctx {
    struct list_head handlers[MOEAI_EVENT_MAX];  /* 事件处理器链表数组 */
    struct list_head event_queue;                /* 事件队列 */
    wait_queue_head_t wait_queue;                /* 等待队列 */
    spinlock_t handlers_lock;                    /* 处理器链表锁 */
    spinlock_t queue_lock;                       /* 事件队列锁 */
    atomic_t running;                            /* 运行标志 */
    struct task_struct *thread;                  /* 调度线程 */
    atomic_t pending_events;                     /* 待处理事件计数 */
};
```

### 2.3 接口定义

```c
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
moeai_scheduler_state_t moeai_scheduler_get_state(void);

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
moeai_event_t *moeai_event_create(
    moeai_event_type_t type,
    moeai_event_priority_t priority,
    void *data,
    size_t data_size,
    const char *source,
    void (*free_fn)(void *data)
);

/**
 * 销毁事件
 * @param event 事件指针
 */
void moeai_event_destroy(moeai_event_t *event);

/**
 * 发布事件到调度器
 * @param event 事件指针
 * @param wait 是否等待事件处理完成
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_post_event(moeai_event_t *event, bool wait);

/**
 * 注册事件处理器
 * @param type 事件类型
 * @param handler 处理函数
 * @param module 关联的模块，NULL表示系统级处理器
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_register_handler(
    moeai_event_type_t type,
    moeai_scheduler_event_handler_fn handler,
    moeai_module_t *module
);

/**
 * 注销事件处理器
 * @param type 事件类型
 * @param handler 处理函数
 * @return 成功返回0，失败返回错误码
 */
int moeai_scheduler_unregister_handler(
    moeai_event_type_t type,
    moeai_scheduler_event_handler_fn handler
);
```

### 2.4 事件处理流程

```
+----------------+    +----------------+    +----------------+
|                |    |                |    |                |
|  事件源(模块)   +--->|    调度器      +--->| 事件处理器(模块) |
|                |    |                |    |                |
+----------------+    +----------------+    +----------------+
                            ^   |
                            |   |
                            |   v
                      +----------------+
                      |                |
                      |   事件队列      |
                      |                |
                      +----------------+
```

1. **事件创建**：通过`moeai_event_create`创建事件对象
2. **事件发布**：通过`moeai_scheduler_post_event`将事件发布到调度器
3. **事件入队**：调度器将事件添加到事件队列，并根据优先级排序
4. **事件分发**：调度线程从队列取出事件，查找对应的处理器
5. **事件处理**：调用所有注册的处理器处理事件
6. **事件完成**：处理完成后，调用完成回调并释放事件资源

### 2.5 优先级调度

事件按照优先级进行调度，高优先级事件会先于低优先级事件处理。事件优先级从高到低分为：

1. **CRITICAL**：关键优先级，需要立即处理的紧急事件
2. **HIGH**：高优先级，重要但不紧急的事件
3. **NORMAL**：普通优先级，常规事件
4. **LOW**：低优先级，可以延迟处理的事件

同等优先级的事件按照FIFO顺序处理。

## 3. 状态管理器 (`state.h`实现部分)

### 3.1 职责

- 维护系统整体状态和各子系统状态
- 根据资源使用情况评估系统状态级别
- 提供状态查询和更新接口
- 支持资源阈值配置

### 3.2 核心数据结构

```c
/* 系统状态管理器 */
struct moeai_system_state_manager {
    atomic_t resource_usage[MOEAI_RESOURCE_MAX];  /* 资源使用率 */
    moeai_threshold_t thresholds[MOEAI_RESOURCE_MAX]; /* 资源阈值 */
    moeai_system_state_t system_state;      /* 整体系统状态 */
    spinlock_t lock;                        /* 保护状态更新的自旋锁 */
};
```

### 3.3 接口定义

```c
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
int moeai_state_update_resource(moeai_resource_type_t resource, unsigned int usage);

/**
 * 获取资源使用率
 * @param resource 资源类型
 * @return 资源使用率（百分比）
 */
unsigned int moeai_state_get_resource_usage(moeai_resource_type_t resource);

/**
 * 设置资源阈值
 * @param resource 资源类型
 * @param warning 警告阈值（百分比）
 * @param critical 临界阈值（百分比）
 * @param emergency 紧急阈值（百分比）
 * @return 成功返回0，失败返回错误码
 */
int moeai_state_set_thresholds(
    moeai_resource_type_t resource,
    unsigned int warning,
    unsigned int critical,
    unsigned int emergency
);

/**
 * 获取当前系统状态
 * @return 系统状态
 */
moeai_system_state_t moeai_state_get_system_state(void);

/**
 * 根据资源使用率和阈值评估系统状态
 * 此函数会更新全局系统状态
 * @return 评估后的系统状态
 */
moeai_system_state_t moeai_state_evaluate(void);
```

### 3.4 状态级别

系统状态级别从低到高分为：

1. **NORMAL**：系统运行正常，资源充足
2. **WARNING**：系统出现轻微压力，资源使用率较高
3. **CRITICAL**：系统压力较大，资源使用率接近极限
4. **EMERGENCY**：系统处于紧急状态，资源严重不足

状态级别由所有受监控资源的最高压力级别决定。

## 4. 事件循环 (`event_loop.c`)

### 4.1 职责

- 提供主事件循环线程实现
- 处理事件队列中的事件
- 管理定时器和周期性任务
- 实现事件优先级排序和处理

### 4.2 核心数据结构

```c
/* 定时器任务结构 */
struct moeai_timer_task {
    void (*func)(void *data);         /* 任务函数 */
    void *data;                       /* 任务数据 */
    unsigned long interval_ms;        /* 执行间隔（毫秒） */
    unsigned long next_run;           /* 下一次执行时间（jiffies） */
    bool repeat;                      /* 是否重复执行 */
    struct list_head list;            /* 链表节点 */
};

/* 事件循环上下文 */
struct moeai_event_loop_ctx {
    struct task_struct *thread;       /* 事件循环线程 */
    atomic_t running;                 /* 运行标志 */
    struct list_head timer_tasks;     /* 定时器任务列表 */
    spinlock_t tasks_lock;            /* 任务列表锁 */
    wait_queue_head_t wait_queue;     /* 等待队列 */
    atomic_t pending_tasks;           /* 待处理任务计数 */
};
```

### 4.3 接口定义

```c
/**
 * 初始化事件循环
 * @return 成功返回0，失败返回错误码
 */
int moeai_event_loop_init(void);

/**
 * 清理事件循环
 */
void moeai_event_loop_exit(void);

/**
 * 启动事件循环
 * @return 成功返回0，失败返回错误码
 */
int moeai_event_loop_start(void);

/**
 * 停止事件循环
 */
void moeai_event_loop_stop(void);

/**
 * 添加定时器任务
 * @param func 任务函数
 * @param data 任务数据
 * @param interval_ms 执行间隔（毫秒）
 * @param repeat 是否重复执行
 * @return 成功返回任务ID，失败返回负数错误码
 */
int moeai_event_loop_add_timer(
    void (*func)(void *data),
    void *data,
    unsigned long interval_ms,
    bool repeat
);

/**
 * 移除定时器任务
 * @param task_id 任务ID
 * @return 成功返回0，失败返回错误码
 */
int moeai_event_loop_remove_timer(int task_id);

/**
 * 主事件循环函数（内部使用）
 * @param data 线程数据
 * @return 线程返回值
 */
int moeai_event_loop_thread(void *data);
```

### 4.4 事件循环流程

1. **初始化**：创建事件循环线程和相关资源
2. **循环过程**：
   - 检查是否有待处理事件
   - 处理到期的定时器任务
   - 从事件队列取出事件并处理
   - 没有事件时进入等待状态
3. **唤醒条件**：
   - 新事件到达
   - 定时器任务到期
   - 收到停止信号

## 5. 初始化与清理 (`init.c`)

### 5.1 职责

- 负责整个MoeAI-C系统的初始化和清理
- 按正确的顺序初始化各个子系统
- 处理初始化失败的错误恢复
- 注册标准模块并启动系统

### 5.2 初始化流程

```
+------------------+
| 日志系统初始化    |
+--------+---------+
         |
+--------v---------+
| 状态管理器初始化  |
+--------+---------+
         |
+--------v---------+
| 模块管理器初始化  |
+--------+---------+
         |
+--------v---------+
| 调度器初始化      |
+--------+---------+
         |
+--------v---------+
| 通信接口初始化    |
+--------+---------+
         |
+--------v---------+
| 注册标准模块      |
+--------+---------+
         |
+--------v---------+
| 启动调度器        |
+--------+---------+
         |
+--------v---------+
| 启动所有模块      |
+------------------+
```

### 5.3 清理流程

清理过程与初始化过程相反：

1. 停止所有模块
2. 停止调度器
3. 移除通信接口
4. 注销所有模块
5. 清理调度器
6. 清理模块管理器
7. 清理状态管理器
8. 清理日志系统

### 5.4 接口定义

```c
/**
 * MoeAI-C系统核心初始化
 * @param debug_mode 是否启用调试模式
 * @return 成功返回0，失败返回错误码
 */
int moeai_core_init(bool debug_mode);

/**
 * MoeAI-C系统核心清理
 */
void moeai_core_exit(void);

/**
 * 注册标准模块
 * @return 成功返回0，失败返回错误码
 */
int moeai_register_modules(void);

/**
 * 注销标准模块
 */
void moeai_unregister_modules(void);
```

## 6. 核心组件之间的关系

核心组件之间的关系如下图所示：

```
+--------------------------------+
|          初始化系统            |
+-------+----------------+-------+
        |                |
+-------v------+  +------v-------+
| 模块管理系统  |  | 状态管理器   |
+-------+------+  +------+-------+
        |                |
        |                |
+-------v----------------v-------+
|         调度器与事件系统        |
+---------------+----------------+
                |
                |
+---------------v----------------+
|            事件循环            |
+--------------------------------+
```

1. **初始化系统**负责协调其他组件的初始化和清理
2. **模块管理系统**和**状态管理器**提供核心功能，相对独立
3. **调度器与事件系统**依赖模块管理系统和状态管理器
4. **事件循环**是调度器的底层实现，处理具体的事件分发

## 7. 核心模块与外部组件的交互

### 7.1 与功能模块的交互

- 通过模块管理系统管理功能模块的生命周期
- 通过调度器向功能模块分发事件
- 功能模块向调度器发布事件
- 状态管理器接收来自功能模块的资源使用情况更新

### 7.2 与通信模块的交互

- 调度器可以将事件转发给通信模块
- 通信模块将用户命令转换为事件或直接调用模块接口
- 状态管理器的信息可通过通信模块暴露给用户空间

### 7.3 与工具模块的交互

- 所有核心组件使用日志系统记录日志
- 调度器使用环形缓冲区存储事件队列
- 所有组件遵循通用定义中的错误码和常量定义

## 8. 核心模块设计原则

### 8.1 模块化与解耦

- 核心组件之间通过明确定义的接口交互
- 组件内部实现对外部隐藏
- 组件间依赖关系清晰，避免循环依赖

### 8.2 鲁棒性与容错

- 所有接口都有完善的错误处理
- 初始化失败时有清理和回滚机制
- 关键数据结构有保护措施（锁、原子操作等）

### 8.3 可扩展性

- 模块系统支持动态注册和卸载
- 事件系统支持自定义事件类型
- 状态管理器支持新增资源类型

### 8.4 性能优化

- 事件处理基于优先级调度
- 关键路径使用无锁或细粒度锁
- 避免不必要的内存分配和复制