# MoeAI-C 智能内核助手模块

*其他语言版本: [English](doc/lang/en/README.md), [Deutsch](doc/lang/de/README.md)*

**MoeAI-C** 是一个以 Linux 内核模块为核心的系统级 AI 助手项目，目标是构建一个具备资源管理、系统监控、事件响应与 AI 决策能力的智能模块，未来支持与用户态 LLM 模型联动，实现动态策略控制与系统智能化响应。

## 功能概述

- 系统资源监控（内存、网络、文件系统）
- 事件检测与智能决策
- 用户态与内核态双向通信
- AI 模型集成（通过用户态代理）
- 命令行工具支持

## 开发计划

项目当前处于 MVP (0.2.0-MVP) 阶段，实现了核心功能框架与基础监控能力，并添加了自检和CI/CD测试功能。详细更新历史请参阅 [CHANGELOG](CHANGELOG)。

## 快速开始

### 1. 克隆仓库

```bash
git clone https://gitlab.dongzeyang.top/ydzat/moeai-c.git
# 或
git clone https://github.com/ydzat/moeai-c.git
cd moeai-c
```

### 2. 构建项目

首先编译内核模块和命令行工具：

```bash
make all     # 构建内核模块
make cli     # 构建命令行工具
```

如果你是首次使用，可以运行自动配置来检测系统环境：

```bash
make configure    # 自动创建.config.mk配置文件
```

### 3. QEMU测试环境

MoeAI-C提供了两种QEMU测试方式，都不需要修改主机系统：

#### 方法1：标准测试

```bash
make qemu-test    # 完整的QEMU测试环境
```

这将：
- 编译内核模块
- 构建命令行工具
- 创建测试用initramfs
- 启动QEMU虚拟机进行测试

#### 方法2：CI自动化测试（推荐）

```bash
make qemu-ci-test    # 适用于CI/CD环境的自动化测试
```

这种方式专为自动化环境设计，执行完整的自检流程。

### 4. 本地系统测试

> 如果你已经选择在QEMU中运行，那么就可以跳过4/5步。

如需在实际系统中测试（需要root权限，请谨慎操作）：

```bash
sudo make test    # 在本机系统上加载模块并测试
```

### 5. 安装与卸载

在实际系统中安装模块（需要root权限）：

```bash
sudo make install    # 安装模块到系统
sudo make uninstall  # 从系统卸载模块
```

### 6. 清理

```bash
make clean          # 清理构建文件
make clean-test     # 清理测试环境
```

## 模块使用方法

模块加载后，可以通过以下方式使用：

### 命令行工具

`moectl` 工具提供了多种功能：

```bash
# 查看模块状态
moectl status

# 执行自检
moectl selftest

# 查看内存监控信息
moectl mem-status

# 查看帮助信息
moectl help
```

### procfs接口

模块提供了procfs接口用于查看状态和控制：

```bash
# 查看模块状态
cat /proc/moeai/status

# 执行控制命令
echo "command_name" > /proc/moeai/control

# 查看详细帮助
cat /proc/moeai/help
```

## 高级配置

### 多语言支持

MoeAI-C 支持中文和英文两种语言，系统会在首次运行 `make configure` 时自动检测您的系统语言环境并设置默认语言。

**切换语言的方法**：

1. **临时切换**（仅对当前命令有效）：
   ```bash
   # 使用中文
   MOEAI_LANG=zh make help
   MOEAI_LANG=zh ./build/bin/moectl help

   # 使用英文
   MOEAI_LANG=en make help
   MOEAI_LANG=en ./build/bin/moectl help
   ```

2. **永久切换**（修改配置文件）：
   ```bash
   # 编辑配置文件
   echo "MOEAI_LANG=zh" >> .config.mk  # 使用中文
   # 或
   echo "MOEAI_LANG=en" >> .config.mk  # 使用英文
   ```

3. **重新配置**：
   ```bash
   rm .config.mk
   make configure
   ```

### 自定义编译选项

可以通过创建或编辑 `.config.mk` 文件来自定义编译选项：

```bash
# 示例 .config.mk 文件
KERNEL_DIR=/path/to/kernel/source
QEMU_KERNEL_SRC=/path/to/qemu/kernel
DEBUG=1  # 启用调试模式
```

### 调试模式

启用调试模式可获取更多日志信息：

```bash
make DEBUG=1 all    # 启用调试模式编译
```

## 常见问题排查

### 模块加载失败

如果模块无法加载，可能的原因和解决方法：

1. **版本不匹配** - 确保模块与当前内核版本兼容：
   ```bash
   make qemu-check    # 检查兼容性
   ```

2. **缺少依赖** - 安装必要的开发包：
   ```bash
   # 对于Debian/Ubuntu系统
   sudo apt install linux-headers-$(uname -r) build-essential
   
   # 对于RHEL/Fedora系统
   sudo dnf install kernel-devel kernel-headers gcc make
   ```

3. **签名问题** - 如果内核强制要求签名模块，可能需要禁用安全启动或签名模块

### QEMU测试问题

1. **"No working init found"错误** - 确保initramfs正确创建：
   ```bash
   make clean-test    # 清理测试环境
   make qemu-test     # 重新创建测试环境
   ```

2. **找不到moectl工具** - 确保CLI工具已编译：
   ```bash
   make cli           # 重新编译CLI工具
   ```

3. **procfs接口不存在** - 检查模块是否正确加载：
   ```bash
   # 在QEMU环境中
   dmesg | grep moeai
   ls -la /proc | grep moeai
   ```

## 开发指南

如果您想为MoeAI-C贡献代码，请参考以下流程：

1. 分支命名规范：`feature/your-feature` 或 `fix/your-fix`
2. 提交前请运行测试：`make qemu-ci-test`
3. 遵循内核编码风格：`make check`
4. 提交PR时附上详细描述

## 环境要求

- Linux内核版本 >= 5.4
- GCC 7.0+
- make 4.0+
- QEMU 4.0+ (仅用于测试)
- 内核开发包（与目标内核版本匹配）
- 可选：busybox（用于创建测试环境）

## 项目架构

MoeAI-C采用模块化设计，主要包含以下组件：

- **核心系统** (`src/core/`) - 提供基础框架和初始化逻辑
- **功能模块** (`src/modules/`) - 实现各种功能，如内存监控
- **IPC接口** (`src/ipc/`) - 包括procfs和netlink通信
- **工具库** (`src/utils/`) - 提供日志、缓冲区等通用功能
- **命令行工具** (`cli/`) - 提供用户友好的管理接口

详细内容请参阅 [design.md](./doc/lang/zh-cn/design.md) 以了解详细的项目架构与设计。

## 许可证

本项目采用**GNU GENERAL PUBLIC LICENSE Version 2**协议。详情请参阅[LICENSE](./LICENSE)文件。

## 贡献

欢迎通过以下方式贡献：
- 提交Issue报告bug或建议新功能
- 提交Pull Request改进代码
- 完善文档和测试用例

## 维护者

- @ydzat - 项目创建者和主要维护者

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