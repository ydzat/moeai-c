# MoeAI-C

*其他语言版本: [English](README.en.md), [Deutsch](README.de.md)*

## 项目简介

**MoeAI-C** 是一个以 Linux 内核模块为核心的系统级 AI 助手项目，目标是构建一个具备资源管理、系统监控、事件响应与 AI 决策能力的智能模块，未来支持与用户态 LLM 模型联动，实现动态策略控制与系统智能化响应。

## 功能特性

- 系统资源监控（内存、网络、文件系统）
- 事件检测与智能决策
- 用户态与内核态双向通信
- AI 模型集成（通过用户态代理）
- 命令行工具支持

## 构建与安装

### 依赖项

- Linux 内核头文件（`linux-headers-$(uname -r)`）
- GCC 编译器和开发工具（`build-essential`）
- CMocka 测试库（可选，用于单元测试）

### 构建步骤

```bash
# 创建必要的目录结构
make dirs

# 构建内核模块
make all

# 仅构建命令行工具
make cli

# 快速测试模块（加载并卸载）
make test

# 清理构建文件
make clean
```

### 安装模块

方法 1：临时安装（重启后失效）
```bash
sudo insmod moeai.ko
```

方法 2：永久安装
```bash
# 安装模块到系统
sudo make install

# 加载模块
sudo modprobe moeai
```

### 卸载模块

方法 1：临时卸载
```bash
sudo rmmod moeai
```

方法 2：永久卸载
```bash
sudo make uninstall
```

## 使用方法

### 检查模块状态

```bash
# 检查模块是否已加载
lsmod | grep moeai

# 查看模块日志
dmesg | grep moeai

# 查看 procfs 接口
ls -la /proc/moeai/
cat /proc/moeai/status
```

### CLI 工具

`moectl` 命令行工具提供了直观的方式来与内核模块交互：

```bash
# 查看系统状态
build/bin/moectl status

# 设置内存监控阈值
build/bin/moectl set threshold <百分比值>

# 触发内存回收操作
build/bin/moectl reclaim

# 查看帮助信息
build/bin/moectl --help
```

### 查看日志

```bash
# 查看模块的最近日志
cat /proc/moeai/log
```

## 项目结构

请参阅 [design.md](design.md) 以了解详细的项目架构与设计。

## 开发计划

项目当前处于 MVP (0.1.0-MVP) 阶段，实现了核心功能框架与基础监控能力。详细更新历史请参阅 [CHANGELOG](CHANGELOG)。

## 未来愿景

MoeAI-C 的愿景是成为智能化 Linux 系统运维的关键组件，通过内核层面与 AI 能力的深度融合，实现以下目标：

### 1. 自主资源管理

- 基于历史数据和负载模式，动态调整系统资源分配
- 智能预测系统瓶颈，提前采取预防措施
- 为关键应用和服务提供资源保障机制

### 2. 智能安全防护

- 实时监控系统行为，识别异常活动和潜在威胁
- 基于历史攻击模式识别可疑操作
- 自适应防火墙规则生成与优化

### 3. 性能优化

- 识别系统性能热点并提供针对性优化
- 智能 I/O 调度与缓存优化
- 应用程序性能分析与瓶颈消除建议

### 4. 开发者生态

- 提供可扩展的模块化架构，允许第三方开发专用功能模块
- API 接口支持与其他智能系统集成
- 开发者工具链与可视化分析平台

### 5. 可解释性与透明度

- 所有自动化决策提供详细说明和理由
- 完善的日志系统，记录所有系统干预行为
- 提供干预追溯和回滚机制

MoeAI-C 将逐步实现这一愿景，打造一个既安全可靠又智能高效的下一代 Linux 系统内核助手。

## 许可证

本项目采用 [MIT/BSD] 许可证 - 参见 [LICENSE](LICENSE) 文件了解详情。