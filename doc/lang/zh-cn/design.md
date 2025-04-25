# MoeAI-C 项目设计文档

## 📌 项目概述

**MoeAI-C** 是一个以 Linux 内核模块为核心的系统级 AI 助手项目，目标是构建一个具备资源管理、系统监控、事件响应与 AI 决策能力的智能模块，未来支持与用户态 LLM 模型联动，实现动态策略控制与系统智能化响应。

---

## 📁 项目结构总览

```text
moeai-c/
├── include/                         # 所有对外头文件接口
│   ├── core/                        # 内核核心结构定义（如 Module、Event 等）
│   │   ├── module.h
│   │   ├── scheduler.h
│   │   └── state.h
│   ├── modules/                     # 各功能模块的接口（按功能划分）
│   │   ├── mem_monitor.h
│   │   ├── net_guard.h
│   │   └── fs_logger.h
│   ├── ipc/                         # 内核与用户态通信结构
│   │   ├── netlink_proto.h
│   │   └── procfs_interface.h
│   ├── utils/                       # 通用数据结构、宏、日志定义
│   │   ├── logger.h
│   │   ├── ring_buffer.h
│   │   └── common_defs.h
│   └── data/                        # 状态快照、历史记录数据结构
│       ├── stats.h
│       ├── snapshot.h
│       └── history.h

├── src/                             # 内核模块源码实现
│   ├── core/                        # 初始化、模块管理、事件调度
│   │   ├── init.c
│   │   ├── module_loader.c
│   │   ├── scheduler.c
│   │   └── event_loop.c
│   ├── modules/                     # 具体模块实现
│   │   ├── mem_monitor.c
│   │   ├── net_guard.c
│   │   └── fs_logger.c
│   ├── ipc/                         # 通信实现（与用户态交互）
│   │   ├── netlink.c
│   │   └── procfs.c
│   ├── utils/                       # 通用函数实现
│   │   ├── logger.c
│   │   └── ring_buffer.c
│   ├── data/                        # 数据存储实现
│   │   ├── stats.c
│   │   ├── snapshot.c
│   │   └── history.c
│   └── main.c                       # 模块入口/退出（init_module/cleanup_module）

├── cli/                             # 用户态命令行工具（类似 systemctl/dnf）
│   ├── moectl.c                     # 主命令工具（moectl status / clean）
│   └── parser.c                     # 参数解析

├── agent/                           # 用户态 AI 助手（守护进程）
│   ├── main.py                      # 主守护进程，启动 LLM 接口
│   ├── handler/
│   │   ├── event_handler.py         # 接收内核事件，做出响应
│   │   ├── net_guard_handler.py
│   │   └── memory_policy.py
│   └── model/
│       ├── openai_wrapper.py        # OpenAI 接口封装
│       └── local_llm.py             # llama.cpp / ggml 模型管理

├── test/                            # 单元测试 / 模块测试
│   ├── test_runner.c
│   ├── test_mem.c
│   ├── test_utils.c
│   └── mocks/                       # 可模拟内核行为的桩代码
│       └── fake_kernel.c

├── build/                           # 构建输出（.ko 文件等，Makefile 自动创建）

├── Makefile                         # 主构建脚本（可编译 .ko + CLI + 测试）
├── README.md                        # 项目介绍与使用方法
├── LICENSE                          # 项目许可证（如 BSD / MIT / LGPL）
├── .gitignore                       # 忽略文件配置
└── .gitlab-ci.yml                   # GitLab CI/CD 流程（构建 + 测试）

```

---

## 🔧 核心模块职责

| *区域 \| 功能重点 \| 支持内容* |
| -------------------- |

include/ | 接口与抽象层 | 跨模块/组件头文件统一管理

src/ | 核心实现层 | Linux 模块主逻辑，包括调度、模块注册、通信等

data/ | 状态快照、策略记录 | 支持 AI 模型决策用数据、历史采样

ipc/ | 用户态交互接口 | 支持 Netlink、/proc、共享内存等

cli/ | 终端命令工具 | 支持 moectl 样式，适配命令行调用

agent/ | AI 执行层（用户态） | 接收事件 → AI 模型推理 → 发回控制策略

test/ | 自动测试支持 | 支持 mock、模块级测试，CI/CD 可用

---

## 📐 通信机制设计

- MVP 阶段采用 `/proc/moeai` 实现用户态与内核之间命令与状态交互
- 后期将集成 Netlink 实现结构化消息传递，支持事件上报与策略下发
- 可扩展支持共享内存机制传递高频采样数据
- 用户态 `agent` 通过监听事件总线或查询接口进行双向数据交互

---

## 🧠 AI 与数据流集成方案

- 内核模块负责事件检测（如内存超限、可疑端口连接等）并推送状态信息
- 用户态 `agent/` 接收事件后调用外部 AI 模型进行推理
- 模型包括：
  - OpenAI 接口（通过 API 请求）
  - 本地部署模型（如 llama.cpp，运行于 agent/main.py）
- AI 分析结果通过 Netlink 或 `/proc/moeai` 回传内核模块控制行为（如动态 OOM 策略）

---

## 📢 用户交互接口

- `cli/moectl` 支持命令格式：
  - `moectl status` ：输出当前系统分析状态
  - `moectl clean` ：触发内存释放逻辑
  - `moectl protect firefox` ：将进程 firefox 标记为高优先级
- `moectl` 将通过 procfs 写入指令，未来可迁移为 Netlink 命令封装

---

## 📋 日志系统设计

- `utils/logger.c` 实现统一日志接口（支持 printk / 缓冲 / 用户态回读）
- 日志可通过 `/proc/moeai_log` 被用户态 CLI 工具或 agent 拉取
- 支持日志等级控制（DEBUG / INFO / WARN / ERROR）
- 日志结构使用 ring buffer 存储，确保不会阻塞内核主流程

---

## 🛠 构建与测试

- Makefile 管理所有模块、工具与测试的构建
  - 支持构建 `.ko` 模块（基于内核头）
  - 支持用户态测试构建与运行
- 测试采用 Unity 或 CMocka 框架编写
- GitLab CI 自动执行构建与测试流程，输出构建产物

---

## ✅ 待实现阶段性目标（里程碑）

1. **构建目录结构与初始化文件**
   - 创建所有目录、基础 `.c/.h` 框架文件
   - 初始化 Makefile 和构建环境

2. **设计代码文件的功能职责**
   - 为每个模块（core/module/ipc/cli/agent）明确目标功能
   - 明确每个 `.c` 文件的函数接口和结构体数据

3. **确定代码文件之间的关系**
   - 参数传递：使用结构体打包参数、使用统一命令结构调度
   - 跨模块引用头文件路径设计（统一 include/...）

4. **定义核心组件之间的交互机制**
   - agent 触发 CLI 命令 → CLI 写入 /proc 或发送 Netlink
   - 内核模块接收消息 → 调用模块处理器 → 状态更新或反馈

5. **编写调用顺序用例（系统运行流程图）**
   - 系统初始化 → 模块注册 → 事件检测 → 事件分发 → 数据处理 → 决策反馈 → 用户态响应

6. **日志与调试接口准备**
   - 支持 `logger()` 接口输出到 ring buffer
   - 提供 `cat /proc/moeai_log` 查看最近日志

7. **构建基础测试框架**
   - 初始化 CMocka / Unity 测试入口
   - 提供至少一个模块（如 mem_monitor）的单元测试示例

8. **集成 GitLab CI/CD 初始模板**
   - 构建模块 `.ko` + CLI 工具
   - 执行测试并产出覆盖率报告

9. **撰写模块化扩展约定与代码规范文档**
   - 明确未来添加模块的模板结构与注册方式
   - 统一代码风格（缩进、注释、函数前缀等）

---

## 📄 协议与部署

- 项目开源协议暂定为 MIT/BSD（根据 AI 组件引用最终确定）
- 支持在 GitLab 上部署 Runner，执行 CI/CD 全流程
- 未来支持自动部署模块、同步 AI 权限与策略模型配置

---

