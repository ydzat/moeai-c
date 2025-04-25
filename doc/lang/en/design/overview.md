# MoeAI-C Overall Design Document

## Document Purpose

This document aims to detail the module responsibility division and interrelationships between components of the MoeAI-C project, fulfilling the requirements of milestones 2 and 3 in design.md:
- Clarify target functionality for each module
- Specify function interfaces and struct data for each `.c` file
- Determine parameter passing methods
- Define cross-module header file path design

## Document Structure

This document is organized according to the following structure:
1. Overall Architecture Overview
2. Module Responsibility Allocation
3. Component Communication and Interaction
4. Detailed Interface Design

For more detailed implementation of each component, please refer to the corresponding sub-documents:
- [Core Module Design](./core_design.md)
- [Functional Module Design](./modules_design.md)
- [Communication Mechanism Design](./communication_design.md)
- [Tools and Auxiliary Component Design](./utils_design.md)
- [Data Processing Design](./data_design.md)

## 1. Overall Architecture Overview

The MoeAI-C project adopts a modular design, divided into the following main parts:

```
            +--------------------+
            |    User Layer      |
            | (CLI / Applications)|
            +----------^---------+
                       |
            +----------v---------+
            |   User-space Agent  |
            | (agent / daemon)    |
            +----------^---------+
                       |
+----------------------------------------------+
|           Kernel-space MoeAI-C Module        |
|  +--------+    +--------+    +---------+    |
|  |  Core  |<-->|Function|<-->|  Comm   |    |
|  | Layer  |    | Layer  |    |  Layer  |    |
|  +--------+    +--------+    +---------+    |
|  |        |    |        |    |         |    |
|  | Module |    | Memory |    | Procfs  |    |
|  | Mgmt   |    | Monitor|    | Netlink |    |
|  | Event  |    | Network|    |         |    |
|  | Sched  |    | FileSystem|  |         |    |
|  +--------+    +--------+    +---------+    |
|                     ^                       |
|                     |                       |
|            +--------v---------+             |
|            |Tools & Data Layer|             |
|            |Logger/Ring Buffer|             |
|            |Stats/Snapshot/History|         |
|            +------------------+             |
+----------------------------------------------+
```

## 2. Module Responsibility Allocation

### 2.1 Core Module (`src/core/`)

- **init.c**: Module initialization sequence control
- **module_loader.c**: Dynamic management of functional submodules
- **scheduler.c**: Event scheduling and handling framework
- **event_loop.c**: Event loop and scheduled task management

### 2.2 Functional Modules (`src/modules/`)

- **mem_monitor.c**: Memory state monitoring and management
- **net_guard.c**: Network activity monitoring and protection
- **fs_logger.c**: File system activity monitoring and logging

### 2.3 Communication Module (`src/ipc/`)

- **procfs.c**: procfs file system interface implementation
- **netlink.c**: Netlink communication protocol implementation

### 2.4 Tools Module (`src/utils/`)

- **logger.c**: Unified logging system implementation
- **ring_buffer.c**: Ring buffer data structure

### 2.5 Data Module (`src/data/`)

- **stats.c**: Real-time statistical data collection
- **snapshot.c**: System state snapshot management
- **history.c**: Historical data recording and analysis

### 2.6 User-space Components

- **CLI Tools** (`cli/`): Command-line interface tools
- **Intelligent Agent** (`agent/`): User-space AI assistant implementation

## 3. Component Communication and Interaction

### 3.1 Internal Communication Methods

- **Between Core Modules**: Function calls + shared data structures
- **Core and Functional Modules**: Callback registration through module interfaces
- **Functional and Data Modules**: Function calls
- **Communication and Other Modules**: Callback registration + event notification

### 3.2 External Communication Methods

- **Kernel and CLI Tools**: procfs interface
- **Kernel and Intelligent Agent**: procfs + future Netlink extension

### 3.3 Data Transfer Plans

- **Data Encapsulation**: Structures defined in header files, referenced by modules
- **Event Transfer**: Unified event structure, including type, priority, and data pointer
- **Command Transfer**: String commands + parameters, processed by a unified parser
- **State Sharing**: Via central state manager

## 4. Detailed Interface Design

For detailed interface design of each module, please refer to the corresponding sub-design documents. The following are general interface design principles (following Linux kernel coding style):

### 4.1 Function Naming Conventions

- Use lowercase letters, with words separated by underscores
- Module prefix: `moeai_<module_name>_`
- Common operation verbs: `init`, `exit`, `start`, `stop`, `handle`, `get`, `set`
- Function names should be descriptive, avoiding abbreviations (unless they are recognized abbreviations)
- For example: `moeai_mem_monitor_init()`, `moeai_logger_write()`

### 4.2 Structure Naming Conventions

- Structure names use lowercase letters, with words separated by underscores
- Module prefix: `moeai_<module_name>_`
- Do not use Hungarian notation or type suffixes (like `_t`)
- Structure tags are the same as type names (Linux kernel style)
- For example:
```c
struct moeai_event {
    /* members */
};
```

### 4.3 Header File Inclusion Rules

- Header file inclusion order:
  1. The corresponding header file for the current file (if any)
  2. Kernel header files (<linux/...>)
  3. Standard library header files
  4. Header files from other modules
- Project header files use relative paths (`#include "utils/logger.h"`)
- Each group of header files should be separated by a blank line

### 4.4 Error Handling Strategy

- Function return values: Return 0 on success, negative error code on failure (using standard Linux kernel error codes like -EINVAL, -ENOMEM, etc.)
- Pointer functions: Return valid pointer on success, NULL or ERR_PTR on failure
- Use `IS_ERR` and `PTR_ERR` macros to handle error pointers
- Errors should be logged to the logging system
- Allocated resources should be cleaned up to avoid memory leaks
- Error paths should be clearly marked (using goto statements for cleanup code)

### 4.5 Code Formatting Standards

- Use tabs for indentation, width of 8 characters
- A line of code should not exceed 80 characters
- Placement of braces:
  - Opening brace on the same line as the control statement
  - Closing brace on a separate line
  - Braces can be omitted for single-statement if/for/while
- Multiple assignment statements should be vertically aligned
- Comment style:
  - Use `/*...*/` for multi-line comments
  - Use `//` for single-line comments