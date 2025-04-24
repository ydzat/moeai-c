# MoeAI-C 通信机制设计

## 概述

MoeAI-C的通信机制设计旨在建立内核模块与用户空间之间的高效、可靠的通信通道。通信系统位于`src/ipc/`目录下，主要由以下两个组件组成：

1. procfs接口 (`procfs.c`)
2. Netlink协议接口 (`netlink.c`)

这两种机制各有优势，分别适用于不同场景：procfs适合简单的命令和状态查询，而Netlink适合高频、复杂的结构化数据交换。

## 1. procfs接口 (`procfs.c`)

### 1.1 职责

- 创建和管理`/proc/moeai`目录及其子条目
- 提供基于文件操作的命令接口
- 支持状态信息查询
- 提供日志内容读取接口
- 实现简单的用户空间控制机制

### 1.2 目录结构

```
/proc/moeai/
  ├── status         # 系统状态信息
  ├── log            # 系统日志
  ├── control        # 命令控制接口
  └── modules/       # 模块相关信息目录
      ├── mem_monitor
      ├── net_guard
      └── fs_logger
```

### 1.3 核心数据结构

```c
/* procfs条目信息结构 */
struct moeai_procfs_entry {
    const char *name;                   /* 条目名称 */
    struct proc_dir_entry *entry;       /* procfs条目指针 */
    struct proc_dir_entry *parent;      /* 父目录 */
    int (*show)(struct seq_file *, void *);  /* 显示回调 */
    ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *); /* 写入回调 */
    void *data;                         /* 条目私有数据 */
};

/* procfs命令处理器结构 */
struct moeai_procfs_cmd_handler_entry {
    char name[MOEAI_MAX_NAME_LEN];           /* 处理器名称 */
    moeai_procfs_cmd_handler_t handler;      /* 处理函数 */
    void *data;                              /* 回调数据 */
    struct list_head list;                   /* 链表节点 */
};
```

### 1.4 接口定义

```c
/**
 * 初始化procfs接口
 * @return 成功返回0，失败返回错误码
 */
int moeai_procfs_init(void);

/**
 * 清理procfs接口
 */
void moeai_procfs_exit(void);

/**
 * 注册procfs命令处理器
 * @param name 处理器名称
 * @param handler 处理函数
 * @param data 回调数据
 * @return 成功返回0，失败返回错误码
 */
int moeai_procfs_register_cmd_handler(
    const char *name,
    moeai_procfs_cmd_handler_t handler,
    void *data
);

/**
 * 注销procfs命令处理器
 * @param name 处理器名称
 * @return 成功返回0，失败返回错误码
 */
int moeai_procfs_unregister_cmd_handler(const char *name);

/**
 * 创建新的procfs条目
 * @param name 条目名称
 * @param parent 父目录，NULL表示根目录
 * @param mode 文件权限模式
 * @param show 显示回调函数
 * @param write 写入回调函数
 * @param data 回调数据
 * @return 成功返回proc_dir_entry指针，失败返回NULL
 */
struct proc_dir_entry *moeai_procfs_create_entry(
    const char *name,
    struct proc_dir_entry *parent,
    umode_t mode,
    int (*show)(struct seq_file *, void *),
    ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *),
    void *data
);

/**
 * 删除procfs条目
 * @param name 条目名称
 * @param parent 父目录，NULL表示根目录
 */
void moeai_procfs_remove_entry(const char *name, struct proc_dir_entry *parent);
```

### 1.5 命令处理流程

```
       用户空间                                  内核空间
+----------------------+                  +------------------------+
| echo "cmd args" >    |                  |                        |
| /proc/moeai/control  +----------------->| moeai_procfs_write     |
+----------------------+                  |                        |
                                          |       解析命令         |
                                          |                        |
                                          |  查找命令处理器        |
                                          |                        |
+----------------------+                  | 调用对应命令处理器     |
| cat /proc/moeai/     |                  |                        |
| status               |<-----------------+ 更新结果状态           |
+----------------------+                  +------------------------+
```

### 1.6 状态查询流程

1. 用户通过`cat /proc/moeai/status`读取系统状态
2. 内核调用对应的`show`回调函数
3. 回调函数使用`seq_file`接口输出格式化状态信息
4. 信息通过`read`系统调用返回给用户空间

### 1.7 模块特定接口

每个功能模块都可以在`/proc/moeai/modules/`下创建自己的条目，用于特定状态查询和控制。模块负责注册和管理自己的procfs条目，但应遵循统一的命名和格式约定。

## 2. Netlink协议接口 (`netlink.c`)

### 2.1 职责

- 建立内核与用户空间之间的高效通信通道
- 支持双向异步通信
- 传递结构化消息
- 处理事件通知和指令接收
- 提供高性能、低延迟的通信机制

### 2.2 协议定义

```c
/* Netlink协议类型 */
#define MOEAI_NETLINK_FAMILY     "MOEAI"
#define MOEAI_NETLINK_VERSION    1
#define MOEAI_NETLINK_MULTICAST  1

/* 消息类型 */
enum moeai_netlink_msg_types {
    MOEAI_MSG_UNSPEC = 0,        /* 未定义消息 */
    MOEAI_MSG_EVENT,             /* 事件通知 */
    MOEAI_MSG_COMMAND,           /* 命令请求 */
    MOEAI_MSG_RESPONSE,          /* 响应消息 */
    MOEAI_MSG_STATUS,            /* 状态更新 */
    MOEAI_MSG_DATA,              /* 数据传输 */
    __MOEAI_MSG_MAX
};
#define MOEAI_MSG_MAX (__MOEAI_MSG_MAX - 1)
```

### 2.3 核心数据结构

```c
/* Netlink消息头部 */
struct moeai_nl_msg_hdr {
    u16 msg_type;                /* 消息类型 */
    u16 flags;                   /* 标志位 */
    u32 seq;                     /* 序列号 */
    u32 pid;                     /* 发送者进程ID */
};

/* Netlink事件消息 */
struct moeai_nl_event {
    struct moeai_nl_msg_hdr hdr;     /* 消息头部 */
    u32 event_type;                  /* 事件类型 */
    u32 event_priority;              /* 事件优先级 */
    u32 timestamp_sec;               /* 时间戳(秒) */
    u32 timestamp_nsec;              /* 时间戳(纳秒) */
    char source[32];                 /* 事件源 */
    u32 data_len;                    /* 数据长度 */
    char data[0];                    /* 数据(变长) */
};

/* Netlink命令消息 */
struct moeai_nl_command {
    struct moeai_nl_msg_hdr hdr;     /* 消息头部 */
    u32 command_id;                  /* 命令ID */
    u32 args_len;                    /* 参数长度 */
    char args[0];                    /* 参数(变长) */
};

/* Netlink响应消息 */
struct moeai_nl_response {
    struct moeai_nl_msg_hdr hdr;     /* 消息头部 */
    u32 status;                      /* 状态码 */
    u32 command_id;                  /* 对应的命令ID */
    u32 data_len;                    /* 数据长度 */
    char data[0];                    /* 数据(变长) */
};

/* Netlink状态消息 */
struct moeai_nl_status {
    struct moeai_nl_msg_hdr hdr;     /* 消息头部 */
    u32 system_state;                /* 系统状态 */
    u32 resource_usage[MOEAI_RESOURCE_MAX]; /* 资源使用率 */
    u32 timestamp_sec;               /* 时间戳(秒) */
    u32 timestamp_nsec;              /* 时间戳(纳秒) */
};
```

### 2.4 接口定义

```c
/**
 * 初始化Netlink接口
 * @return 成功返回0，失败返回错误码
 */
int moeai_netlink_init(void);

/**
 * 清理Netlink接口
 */
void moeai_netlink_exit(void);

/**
 * 发送事件消息到用户空间
 * @param event 事件结构指针
 * @return 成功返回0，失败返回错误码
 */
int moeai_netlink_send_event(const moeai_event_t *event);

/**
 * 发送状态消息到用户空间
 * @return 成功返回0，失败返回错误码
 */
int moeai_netlink_send_status(void);

/**
 * 发送响应消息到用户空间
 * @param pid 目标进程ID
 * @param seq 序列号
 * @param command_id 命令ID
 * @param status 状态码
 * @param data 响应数据
 * @param data_len 数据长度
 * @return 成功返回0，失败返回错误码
 */
int moeai_netlink_send_response(u32 pid, u32 seq, u32 command_id, u32 status, 
                               const void *data, u32 data_len);

/**
 * 注册命令处理器
 * @param command_id 命令ID
 * @param handler 处理函数
 * @param data 回调数据
 * @return 成功返回0，失败返回错误码
 */
int moeai_netlink_register_command(u32 command_id, 
                                  int (*handler)(const struct moeai_nl_command *cmd, void *data),
                                  void *data);

/**
 * 注销命令处理器
 * @param command_id 命令ID
 * @return 成功返回0，失败返回错误码
 */
int moeai_netlink_unregister_command(u32 command_id);
```

### 2.5 通信流程

```
       用户空间                                  内核空间
+----------------------+                  +------------------------+
|                      |                  |                        |
|  创建Netlink套接字    |                  |  注册Netlink协议族     |
|                      |                  |                        |
+----------+-----------+                  +------------+-----------+
           |                                          |
+----------v-----------+                  +------------v-----------+
|                      |                  |                        |
|    发送命令消息      +----------------->+  接收并解析命令        |
|                      |                  |                        |
+----------+-----------+                  +------------+-----------+
           |                                          |
+----------v-----------+                  +------------v-----------+
|                      |                  |                        |
|    接收响应消息      +<-----------------+  发送处理结果          |
|                      |                  |                        |
+----------+-----------+                  +------------+-----------+
           |                                          |
+----------v-----------+                  +------------v-----------+
|                      |                  |                        |
|    接收事件通知      +<-----------------+  发送事件通知          |
|                      |                  |                        |
+----------------------+                  +------------------------+
```

### 2.6 多播组通信

除了点对点通信外，Netlink还支持多播组通信，可用于向多个用户空间进程广播事件通知：

1. 用户空间进程通过`setsockopt`加入特定多播组
2. 内核通过`nlmsg_multicast`向多播组发送消息
3. 所有加入该组的进程都能接收到消息

多播组定义：

```c
/* 多播组ID */
enum moeai_netlink_groups {
    MOEAI_GROUP_NONE = 0,
    MOEAI_GROUP_EVENTS,         /* 事件通知组 */
    MOEAI_GROUP_STATUS,         /* 状态更新组 */
    __MOEAI_GROUP_MAX
};
#define MOEAI_GROUP_MAX (__MOEAI_GROUP_MAX - 1)
```

## 3. 两种通信机制的比较

### 3.1 procfs接口

优势：
- 简单直接，易于调试
- 基于文件系统，可使用标准文件操作
- 不需要特殊的用户态库
- 适合简单命令和状态查询

劣势：
- 不支持异步通信
- 性能较低，不适合频繁交互
- 不适合传输大量结构化数据
- 内核空间无法主动通知用户空间

### 3.2 Netlink接口

优势：
- 支持双向异步通信
- 高性能，低延迟
- 适合结构化数据传输
- 支持多播和广播
- 内核可主动通知用户空间

劣势：
- 实现复杂度高
- 需要专门的用户态库
- 调试相对困难
- 消息格式需要精心设计

## 4. 通信机制实现策略

### 4.1 分阶段实现

MoeAI-C项目将按以下顺序实现通信机制：

1. **第一阶段**：实现基础procfs接口
   - 提供状态查询和简单命令执行
   - 支持基本日志查看功能

2. **第二阶段**：增强procfs接口
   - 添加模块特定接口
   - 完善命令处理机制

3. **第三阶段**：实现Netlink接口
   - 设计消息格式和协议
   - 实现基础通信机制

4. **第四阶段**：完善Netlink功能
   - 实现事件通知系统
   - 支持多播和复杂数据交换

### 4.2 用户空间工具适配

- CLI工具(`moectl`)将首先使用procfs接口
- 后期将同时支持procfs和Netlink
- 用户态守护进程(agent)将主要使用Netlink接口

## 5. 通信机制与其他组件的交互

### 5.1 与核心模块的交互

- 通过事件系统接收和发送事件
- 提供命令处理器接口，转发命令到相应模块
- 输出系统状态信息

### 5.2 与功能模块的交互

- 功能模块注册自己的procfs条目和命令处理器
- 功能模块通过通信系统接收用户命令
- 功能模块可以通过通信系统向用户空间发送事件通知

### 5.3 与用户空间组件的交互

- `moectl`通过读写procfs文件执行命令和查询状态
- 用户态代理通过Netlink接收事件和发送命令
- Web接口和GUI工具可通过用户态库间接与内核通信

## 6. 安全考虑

### 6.1 访问控制

- procfs接口使用适当的文件权限控制访问
- 敏感操作需要root权限
- 命令接口进行参数验证和边界检查

### 6.2 输入验证

- 所有来自用户空间的输入都经过严格验证
- 防止缓冲区溢出和其他安全漏洞
- 限制命令长度和参数数量

### 6.3 资源保护

- 防止DOS攻击，限制消息频率和大小
- 内存使用限制，防止资源耗尽
- 异常处理，确保系统稳定性

## 7. 未来扩展

### 7.1 共享内存通信

未来可实现共享内存机制，适用于高频大容量数据交换：

```c
/**
 * 创建共享内存区域
 * @param name 共享内存名称
 * @param size 大小（字节）
 * @return 成功返回内存地址，失败返回NULL
 */
void *moeai_shmem_create(const char *name, size_t size);

/**
 * 映射共享内存到用户空间
 * @param name 共享内存名称
 * @return 成功返回文件描述符，失败返回负数错误码
 */
int moeai_shmem_export(const char *name);
```

### 7.2 eBPF通信机制

未来可集成eBPF，提供更灵活的用户空间与内核交互方式：

```c
/**
 * 注册eBPF处理程序
 * @param prog_fd eBPF程序文件描述符
 * @param event_type 事件类型
 * @return 成功返回0，失败返回错误码
 */
int moeai_bpf_register_handler(int prog_fd, int event_type);
```

### 7.3 REST API网关

远期规划中，可以实现用户态REST API网关，允许通过HTTP API控制MoeAI-C系统：

```
                        +-------------------+
                        |     REST API      |
                        +--------+----------+
                                 |
+----------------+      +--------v----------+
|   Web界面      +----->+   用户态守护进程   |
+----------------+      +--------+----------+
                                 |
                        +--------v----------+
                        |  Netlink/procfs   |
                        +--------+----------+
                                 |
                        +--------v----------+
                        |    内核模块       |
                        +-------------------+
```