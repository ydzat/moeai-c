# MoeAI-C

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

- Linux 内核头文件
- GCC 编译器
- CMocka 测试库 (用于测试)

### 构建步骤

```bash
# 构建所有组件（内核模块、CLI工具和测试）
make all

# 仅构建内核模块
make module

# 仅构建命令行工具
make cli

# 仅构建测试程序
make tests

# 清理构建文件
make clean
```

### 安装模块

```bash
sudo insmod build/moeai.ko
```

## 使用方法

### CLI 工具

```bash
# 查看系统状态
build/moectl status

# 触发内存清理
build/moectl clean

# 保护特定进程
build/moectl protect [进程名]
```

## 项目结构

请参阅 [design.md](design.md) 以了解详细的项目架构与设计。

## 开发计划

项目当前处于开发初期阶段，按照 design.md 中的阶段性目标进行实现。

## 许可证

本项目采用 [MIT/BSD] 许可证 - 参见 [LICENSE](LICENSE) 文件了解详情。