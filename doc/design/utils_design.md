# MoeAI-C 工具与辅助组件设计

## 概述

工具模块提供MoeAI-C项目中各组件所需的通用功能，如日志记录、数据缓冲等基础设施。工具模块位于`src/utils/`目录下，包含以下主要组件：

1. 日志系统 (`logger.c`)
2. 环形缓冲区 (`ring_buffer.c`) 
3. 通用定义 (`common_defs.h`)

这些工具模块为整个系统提供基础功能支持，是其他所有模块的基础依赖。

## 1. 日志系统 (`logger.c`)

### 1.1 职责

- 提供统一的日志记录与查看机制
- 支持多个日志级别（DEBUG、INFO、WARN、ERROR、FATAL）
- 按模块分类日志输出
- 提供环形缓冲区存储最近日志
- 支持通过procfs查看日志

### 1.2 核心数据结构

```c
/* 日志上下文结构 */
struct moeai_logger_ctx {
    moeai_logger_config_t config;      /* 日志配置 */
    moeai_ring_buffer_t *log_buffer;   /* 日志环形缓冲区 */
    rwlock_t config_lock;              /* 配置读写锁 */
    spinlock_t buffer_lock;            /* 缓冲区访问锁 */
    atomic_t initialized;              /* 初始化标志 */
};

/* 日志条目结构 */
struct moeai_log_entry {
    uint64_t timestamp;                /* 时间戳 (纳秒) */
    moeai_log_level_t level;           /* 日志级别 */
    char module[32];                   /* 模块名称 */
    char message[256];                 /* 日志消息 */
};
```

### 1.3 接口定义

```c
/**
 * 初始化日志系统
 * @param debug_mode 是否启用调试模式
 * @return 成功返回0，失败返回错误码
 */
int moeai_logger_init(bool debug_mode);

/**
 * 清理日志系统
 */
void moeai_logger_exit(void);

/**
 * 记录日志
 * @param level 日志级别
 * @param module 模块名称
 * @param fmt 格式字符串
 * @param ... 可变参数
 */
void moeai_log(moeai_log_level_t level, const char *module, const char *fmt, ...);

/**
 * 创建procfs日志接口
 * @return 成功返回0，失败返回错误码
 */
int moeai_logger_create_procfs(void);

/**
 * 移除procfs日志接口
 */
void moeai_logger_remove_procfs(void);

/**
 * 设置日志配置
 * @param config 配置结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_logger_set_config(const moeai_logger_config_t *config);

/**
 * 获取日志配置
 * @param config 用于存储配置的结构体指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_logger_get_config(moeai_logger_config_t *config);

/**
 * 清空日志缓冲区
 * @return 成功返回0，失败返回错误码
 */
int moeai_logger_clear(void);

/**
 * 复制最近的日志条目
 * @param entries 日志条目数组
 * @param max_entries 最大条目数
 * @param actual_entries 实际复制的条目数
 * @return 成功返回0，失败返回错误码
 */
int moeai_logger_get_recent_logs(
    struct moeai_log_entry *entries,
    size_t max_entries,
    size_t *actual_entries
);
```

### 1.4 日志级别

日志系统支持以下日志级别，按严重性递增排序：

1. **DEBUG**：调试信息，仅在调试模式下显示
2. **INFO**：一般信息，系统正常运行状态
3. **WARN**：警告信息，可能存在潜在问题
4. **ERROR**：错误信息，操作失败但系统仍可运行
5. **FATAL**：致命错误，系统无法继续运行

### 1.5 日志输出流程

1. 调用者通过`MOEAI_DEBUG/INFO/WARN/ERROR/FATAL`宏记录日志
2. 日志系统根据当前配置的最低日志级别决定是否处理该日志
3. 格式化日志消息，添加时间戳、级别和模块信息
4. 如果配置了控制台输出，则通过`printk`输出到内核日志
5. 如果配置了缓冲区输出，则将日志条目写入环形缓冲区
6. 用户态可通过`/proc/moeai/log`查看最近日志

## 2. 环形缓冲区 (`ring_buffer.c`)

### 2.1 职责

- 提供高效的FIFO缓冲区实现
- 支持线程安全的单生产者-单消费者模式
- 固定大小，无动态扩展，适合内核环境
- 支持任意类型数据存储

### 2.2 核心数据结构

```c
/* 环形缓冲区结构 */
struct moeai_ring_buffer {
    void *buffer;              /* 缓冲区数据指针 */
    size_t size;               /* 缓冲区总大小（字节） */
    size_t element_size;       /* 每个元素的大小（字节） */
    size_t capacity;           /* 缓冲区容量（元素数量） */
    size_t head;               /* 头指针（写入位置） */
    size_t tail;               /* 尾指针（读取位置） */
    bool is_full;              /* 缓冲区是否已满标志 */
    spinlock_t lock;           /* 自旋锁，保护并发访问 */
};
```

### 2.3 接口定义

```c
/**
 * 创建环形缓冲区
 * @param capacity 缓冲区容量（元素数量）
 * @param element_size 每个元素的大小（字节）
 * @return 成功返回缓冲区指针，失败返回NULL
 */
moeai_ring_buffer_t *moeai_ring_buffer_create(size_t capacity, size_t element_size);

/**
 * 销毁环形缓冲区，释放所有资源
 * @param rb 环形缓冲区指针
 */
void moeai_ring_buffer_destroy(moeai_ring_buffer_t *rb);

/**
 * 向环形缓冲区写入元素
 * @param rb 环形缓冲区指针
 * @param element 元素指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_ring_buffer_write(moeai_ring_buffer_t *rb, const void *element);

/**
 * 从环形缓冲区读取元素
 * @param rb 环形缓冲区指针
 * @param element 用于存储读取元素的缓冲区指针
 * @return 成功返回0，失败（缓冲区为空）返回错误码
 */
int moeai_ring_buffer_read(moeai_ring_buffer_t *rb, void *element);

/**
 * 清空环形缓冲区
 * @param rb 环形缓冲区指针
 */
void moeai_ring_buffer_clear(moeai_ring_buffer_t *rb);

/**
 * 检查环形缓冲区是否为空
 * @param rb 环形缓冲区指针
 * @return 为空返回true，否则返回false
 */
bool moeai_ring_buffer_is_empty(const moeai_ring_buffer_t *rb);

/**
 * 检查环形缓冲区是否已满
 * @param rb 环形缓冲区指针
 * @return 已满返回true，否则返回false
 */
bool moeai_ring_buffer_is_full(const moeai_ring_buffer_t *rb);

/**
 * 获取环形缓冲区中当前元素数量
 * @param rb 环形缓冲区指针
 * @return 当前元素数量
 */
size_t moeai_ring_buffer_size(const moeai_ring_buffer_t *rb);

/**
 * 获取环形缓冲区的总容量
 * @param rb 环形缓冲区指针
 * @return 缓冲区的总容量（元素数量）
 */
size_t moeai_ring_buffer_capacity(const moeai_ring_buffer_t *rb);
```

### 2.4 实现细节

环形缓冲区的实现注重以下几点：

1. **内存效率**：预分配固定大小的内存，没有动态调整
2. **时间效率**：所有基本操作（读、写、检查）的时间复杂度都是O(1)
3. **线程安全**：使用自旋锁保护关键操作，但锁粒度尽可能小
4. **溢出处理**：缓冲区满时，新写入会覆盖最旧的数据（FIFO策略）
5. **特殊情况**：处理空/满状态的边界条件

## 3. 通用定义 (`common_defs.h`)

### 3.1 职责

- 定义全局常量和宏
- 提供版本信息
- 定义通用错误码
- 控制功能开关
- 提供兼容性支持和调试辅助

### 3.2 主要内容

```c
/* 模块版本信息 */
#define MOEAI_MAJOR_VERSION    0
#define MOEAI_MINOR_VERSION    1
#define MOEAI_PATCH_VERSION    0
#define MOEAI_VERSION_STR     "0.1.0"

/* 通用错误码定义 */
#define MOEAI_SUCCESS           0      /* 成功 */
#define MOEAI_ERROR_GENERIC    -1      /* 通用错误 */
#define MOEAI_ERROR_NOMEM      -2      /* 内存分配失败 */
#define MOEAI_ERROR_INVAL      -3      /* 无效参数 */
#define MOEAI_ERROR_BUSY       -4      /* 资源忙 */
#define MOEAI_ERROR_TIMEOUT    -5      /* 操作超时 */
#define MOEAI_ERROR_NOENT      -6      /* 不存在的实体 */
#define MOEAI_ERROR_IO         -7      /* IO错误 */
#define MOEAI_ERROR_AGAIN      -8      /* 需要重试 */

/* 模块功能宏控制 */
#define MOEAI_FEATURE_MEMORY_MONITOR    1   /* 内存监控功能 */
#define MOEAI_FEATURE_NETWORK_GUARD     1   /* 网络防护功能 */
#define MOEAI_FEATURE_FS_LOGGER         1   /* 文件系统日志功能 */
#define MOEAI_FEATURE_PROCFS            1   /* procfs接口功能 */
#define MOEAI_FEATURE_NETLINK           0   /* netlink接口功能（未实现） */

/* 默认配置值 */
#define MOEAI_DEFAULT_BUFFER_SIZE       4096
#define MOEAI_MAX_PATH_LEN              256
#define MOEAI_MAX_NAME_LEN              64
#define MOEAI_MAX_CMD_LEN               128
#define MOEAI_MAX_MODULES               16
#define MOEAI_MAX_EVENTS                64

/* 内核版本兼容宏 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
#define TIMESPEC64_TO_TIMESPEC(ts, ts64) ({ (ts)->tv_sec = (ts64)->tv_sec; (ts)->tv_nsec = (ts64)->tv_nsec; })
#define TIMESPEC_TO_TIMESPEC64(ts64, ts) ({ (ts64)->tv_sec = (ts)->tv_sec; (ts64)->tv_nsec = (ts)->tv_nsec; })
#endif

/* 内存分配与释放跟踪宏 */
#ifdef CONFIG_MOEAI_MEM_DEBUG
#define moeai_kmalloc(size, flags) ({ \
    void *__ptr = kmalloc(size, flags); \
    pr_debug("MoeAI-C: kmalloc(%zu) = %p at %s:%d\n", \
             (size_t)size, __ptr, __FILE__, __LINE__); \
    __ptr; \
})
#define moeai_kfree(ptr) do { \
    pr_debug("MoeAI-C: kfree(%p) at %s:%d\n", ptr, __FILE__, __LINE__); \
    kfree(ptr); \
} while (0)
#else
#define moeai_kmalloc(size, flags) kmalloc(size, flags)
#define moeai_kfree(ptr) kfree(ptr)
#endif

/* 调试宏 */
#ifdef CONFIG_MOEAI_DEBUG
#define MOEAI_DEBUG_PRINT(fmt, ...) \
    pr_debug("MoeAI-C [%s:%d]: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define MOEAI_DEBUG_PRINT(fmt, ...) do {} while (0)
#endif
```

## 4. 工具组件之间的关系

1. **日志系统** 依赖于环形缓冲区存储日志记录
2. **日志系统** 使用通用定义中的错误码和功能开关
3. **环形缓冲区** 使用通用定义中的内存分配宏
4. 所有其他模块都依赖这些工具组件

## 5. 工具组件与其他模块的关系

### 5.1 依赖方向

```
+-------------------+
| 功能模块/核心模块  |
+-------------------+
          |
          | 依赖于
          v
+-------------------+
|     工具模块      |
+-------------------+
```

所有其他模块都依赖于工具模块，而工具模块不依赖于其他模块。这种单向依赖关系使得工具模块可以独立开发、测试和维护，同时也简化了整体系统的依赖树。

### 5.2 调用关系

其他模块通过工具模块提供的API来使用工具功能：

1. 核心模块负责初始化和管理工具模块的生命周期
2. 所有模块通过日志接口记录运行状态和错误信息
3. 数据密集型模块（如监控模块）使用环形缓冲区存储采集数据
4. 通信模块使用日志系统提供日志查看功能

## 6. 工具组件的设计准则

MoeAI-C项目的工具组件设计遵循以下准则：

### 6.1 通用性

- 工具组件应当提供通用功能，不包含特定业务逻辑
- 接口设计应该足够灵活，满足不同场景需求
- 支持多种数据类型和使用模式

### 6.2 效率性

- 最小化CPU和内存开销
- 避免不必要的内存分配和复制
- 关键路径上的操作应高度优化

### 6.3 可靠性

- 健壮错误处理，不崩溃、不死锁
- 边界情况处理（如资源耗尽、极限值）
- 提供清晰的错误码和诊断信息

### 6.4 可测试性

- 接口设计便于单元测试
- 支持DEBUG模式提供额外诊断信息
- 统一的错误报告机制

## 7. 未来扩展

工具模块计划在未来版本中扩展以下功能：

1. **配置系统**：统一的配置管理机制
2. **内存池**：高效的内存分配器
3. **计时器工具**：性能分析和超时管理
4. **哈希表实现**：高效的键值对存储
5. **字符串操作**：安全的字符串处理函数

这些扩展将保持与现有工具模块相同的设计理念和编码风格，同时提供更多工具来支持系统的发展。