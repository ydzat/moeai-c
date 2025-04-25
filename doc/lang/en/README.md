# MoeAI-C

*Read this in other languages: [简体中文](../../../README.md), [Deutsch](../de/README.md)*

**MoeAI-C** is a Linux kernel module-based system-level AI assistant project. It aims to build an intelligent module with resource management, system monitoring, event response, and AI decision-making capabilities. It will support integration with user-space LLM models in the future to achieve dynamic policy control and intelligent system response.

## Features

- System resource monitoring (memory, network, file system)
- Event detection and intelligent decision-making
- Two-way communication between user space and kernel space
- AI model integration (via user space agent)
- Command-line tool support

## Development Plan

The project is currently in the MVP (0.2.0-MVP) stage, with core functionality framework, basic monitoring capabilities, self-test functions and CI/CD testing capabilities implemented. For detailed update history, please refer to [CHANGELOG](../../../CHANGELOG).

## Quick Start

### 1. Clone Repository

```bash
git clone https://gitlab.dongzeyang.top/ydzat/moeai-c.git
# or
git clone https://github.com/ydzat/moeai-c.git
cd moeai-c
```

### 2. Build Project

First, compile the kernel module and command-line tool:

```bash
make all     # Build kernel module
make cli     # Build command-line tool
```

If you're using it for the first time, you can run auto-configuration to detect the system environment:

```bash
make configure    # Automatically create .config.mk configuration file
```

### 3. QEMU Test Environment

MoeAI-C provides two QEMU testing methods, both without modifying the host system:

#### Method 1: Standard Test

```bash
make qemu-test    # Complete QEMU test environment
```

This will:
- Compile the kernel module
- Build the command-line tool
- Create test initramfs
- Start QEMU virtual machine for testing

#### Method 2: CI Automated Testing (Recommended)

```bash
make qemu-ci-test    # Automated testing suitable for CI/CD environments
```

This method is designed for automated environments and executes a complete self-test process.

### 4. Local System Testing

> If you have chosen to run in QEMU, you can skip steps 4/5.

To test in an actual system (requires root permissions, please exercise caution):

```bash
sudo make test    # Load the module and test on the local system
```

### 5. Installation and Uninstallation

Install the module in an actual system (requires root permissions):

```bash
sudo make install    # Install the module to the system
sudo make uninstall  # Uninstall the module from the system
```

### 6. Clean Up

```bash
make clean          # Clean build files
make clean-test     # Clean test environment
```

## Module Usage

After the module is loaded, it can be used in the following ways:

### Command-line Tool

The `moectl` tool provides various functions:

```bash
# View module status
moectl status

# Perform self-test
moectl selftest

# View memory monitoring information
moectl mem-status

# View help information
moectl help
```

### procfs Interface

The module provides a procfs interface for viewing status and control:

```bash
# View module status
cat /proc/moeai/status

# Execute control command
echo "command_name" > /proc/moeai/control

# View detailed help
cat /proc/moeai/help
```

## Advanced Configuration

### Custom Compilation Options

You can customize compilation options by creating or editing the `.config.mk` file:

```bash
# Example .config.mk file
KERNEL_DIR=/path/to/kernel/source
QEMU_KERNEL_SRC=/path/to/qemu/kernel
DEBUG=1  # Enable debug mode
```

### Debug Mode

Enabling debug mode provides more log information:

```bash
make DEBUG=1 all    # Compile with debug mode enabled
```

## Troubleshooting

### Module Loading Failure

If the module cannot be loaded, possible causes and solutions:

1. **Version Mismatch** - Ensure the module is compatible with the current kernel version:
   ```bash
   make qemu-check    # Check compatibility
   ```

2. **Missing Dependencies** - Install necessary development packages:
   ```bash
   # For Debian/Ubuntu systems
   sudo apt install linux-headers-$(uname -r) build-essential
   
   # For RHEL/Fedora systems
   sudo dnf install kernel-devel kernel-headers gcc make
   ```

3. **Signing Issues** - If the kernel enforces signed modules, you may need to disable secure boot or sign the module

### QEMU Testing Issues

1. **"No working init found" Error** - Ensure initramfs is created correctly:
   ```bash
   make clean-test    # Clean test environment
   make qemu-test     # Recreate test environment
   ```

2. **Cannot Find moectl Tool** - Ensure CLI tool has been compiled:
   ```bash
   make cli           # Recompile CLI tool
   ```

3. **procfs Interface Does Not Exist** - Check if the module is loaded correctly:
   ```bash
   # In QEMU environment
   dmesg | grep moeai
   ls -la /proc | grep moeai
   ```

## Development Guide

If you want to contribute code to MoeAI-C, please refer to the following process:

1. Branch naming convention: `feature/your-feature` or `fix/your-fix`
2. Run tests before submitting: `make qemu-ci-test`
3. Follow kernel coding style: `make check`
4. Include detailed description when submitting PR

## Requirements

- Linux kernel version >= 5.4
- GCC 7.0+
- make 4.0+
- QEMU 4.0+ (for testing only)
- Kernel development package (matching target kernel version)
- Optional: busybox (for creating test environment)

## Project Architecture

MoeAI-C uses a modular design, mainly containing the following components:

- **Core System** (`src/core/`) - Provides basic framework and initialization logic
- **Functional Modules** (`src/modules/`) - Implements various functions, such as memory monitoring
- **IPC Interface** (`src/ipc/`) - Includes procfs and netlink communication
- **Utility Library** (`src/utils/`) - Provides common functions like logging, buffers, etc.
- **Command-line Tool** (`cli/`) - Provides user-friendly management interface

For details, please refer to [design.md](./design.md) to understand the detailed project architecture and design.

## License

This project is under the **GNU GENERAL PUBLIC LICENSE Version 2** license. See the [LICENSE](../../../LICENSE) file for details.

## Contributions

Contributions are welcome through the following methods:
- Submit Issues to report bugs or suggest new features
- Submit Pull Requests to improve code
- Improve documentation and test cases

## Maintainer

- @ydzat - Project creator and main maintainer

## Future Vision

The vision of MoeAI-C is to become a key component of intelligent Linux system operations through deep integration of kernel-level and AI capabilities, achieving the following objectives:

### 1. Autonomous Resource Management

- Dynamically adjust system resource allocation based on historical data and load patterns
- Intelligently predict system bottlenecks and take preventive measures
- Provide resource guarantee mechanisms for critical applications and services

### 2. Intelligent Security Protection

- Monitor system behavior in real-time and identify abnormal activities and potential threats
- Identify suspicious operations based on historical attack patterns
- Generate and optimize adaptive firewall rules

### 3. Performance Optimization

- Identify system performance hotspots and provide targeted optimization
- Intelligent I/O scheduling and cache optimization
- Application performance analysis and bottleneck elimination recommendations

### 4. Developer Ecosystem

- Provide an extensible modular architecture allowing third-party development of specialized functional modules
- API interface support for integration with other intelligent systems
- Developer toolchain and visualization analysis platform

### 5. Explainability and Transparency

- All automated decisions provide detailed explanations and rationales
- Comprehensive logging system recording all system intervention behaviors
- Provide intervention tracing and rollback mechanisms

MoeAI-C will progressively realize this vision, creating a next-generation Linux system kernel assistant that is both secure, reliable, and intelligently efficient.