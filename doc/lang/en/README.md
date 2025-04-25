# MoeAI-C

*Read this in other languages: [简体中文](../../../README.md), [Deutsch](../de/README.md)*

## Project Introduction

**MoeAI-C** is a Linux kernel module-based system-level AI assistant project. It aims to build an intelligent module with resource management, system monitoring, event response, and AI decision-making capabilities. It will support integration with user-space LLM models in the future to achieve dynamic policy control and intelligent system response.

## Features

- System resource monitoring (memory, network, file system)
- Event detection and intelligent decision-making
- Two-way communication between user space and kernel space
- AI model integration (via user space agent)
- Command-line tool support

## Build and Installation

### Dependencies

- Linux kernel headers (`linux-headers-$(uname -r)`)
- GCC compiler and development tools (`build-essential`)
- CMocka test library (optional, for unit testing)

### Build Steps

```bash
# Create necessary directory structure
make dirs

# Build kernel module
make all

# Build command line tool only
make cli

# Quick test module (load and unload)
make test

# Clean build files
make clean
```

### Installing the Module

Method 1: Temporary installation (lost after reboot)
```bash
sudo insmod moeai.ko
```

Method 2: Permanent installation
```bash
# Install module to system
sudo make install

# Load module
sudo modprobe moeai
```

### Uninstalling the Module

Method 1: Temporary uninstall
```bash
sudo rmmod moeai
```

Method 2: Permanent uninstall
```bash
sudo make uninstall
```

## Usage

### Checking Module Status

```bash
# Check if module is loaded
lsmod | grep moeai

# View module logs
dmesg | grep moeai

# Check procfs interface
ls -la /proc/moeai/
cat /proc/moeai/status
```

### CLI Tool

The `moectl` command-line tool provides an intuitive way to interact with the kernel module:

```bash
# View system status
build/bin/moectl status

# Set memory monitoring threshold
build/bin/moectl set threshold <percentage>

# Trigger memory reclamation
build/bin/moectl reclaim

# View help information
build/bin/moectl --help
```

### Viewing Logs

```bash
# View recent module logs
cat /proc/moeai/log
```

## Project Structure

Please refer to [design.md](design.md) for detailed project architecture and design.

## Development Plan

The project is currently in the MVP (0.1.0-MVP) stage, with core functionality framework and basic monitoring capabilities implemented. For detailed update history, please refer to [CHANGELOG](CHANGELOG).

## Future Vision

MoeAI-C's vision is to become a key component of intelligent Linux system operations through deep integration of kernel-level and AI capabilities, achieving the following objectives:

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

## License

This project is licensed under the [MIT/BSD] License - see the [LICENSE](LICENSE) file for details.