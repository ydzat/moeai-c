# MoeAI-C Project Design Document

## 📌 Project Overview

**MoeAI-C** is a system-level AI assistant project based on Linux kernel modules. It aims to build an intelligent module with resource management, system monitoring, event response, and AI decision-making capabilities. It will support integration with user-space LLM models in the future to achieve dynamic policy control and intelligent system responses.

---

## 📁 Project Structure Overview

```text
moeai-c/
├── include/                         # All public header interfaces
│   ├── core/                        # Core kernel structure definitions (Module, Event, etc.)
│   │   ├── module.h
│   │   ├── scheduler.h
│   │   └── state.h
│   ├── modules/                     # Interfaces for various functional modules
│   │   ├── mem_monitor.h
│   │   ├── net_guard.h
│   │   └── fs_logger.h
│   ├── ipc/                         # Kernel-userspace communication structures
│   │   ├── netlink_proto.h
│   │   └── procfs_interface.h
│   ├── utils/                       # Common data structures, macros, log definitions
│   │   ├── logger.h
│   │   ├── ring_buffer.h
│   │   └── common_defs.h
│   └── data/                        # Status snapshots, history record data structures
│       ├── stats.h
│       ├── snapshot.h
│       └── history.h

├── src/                             # Kernel module source implementation
│   ├── core/                        # Initialization, module management, event scheduling
│   │   ├── init.c
│   │   ├── module_loader.c
│   │   ├── scheduler.c
│   │   └── event_loop.c
│   ├── modules/                     # Specific module implementations
│   │   ├── mem_monitor.c
│   │   ├── net_guard.c
│   │   └── fs_logger.c
│   ├── ipc/                         # Communication implementation (interaction with userspace)
│   │   ├── netlink.c
│   │   └── procfs.c
│   ├── utils/                       # Common function implementations
│   │   ├── logger.c
│   │   └── ring_buffer.c
│   ├── data/                        # Data storage implementation
│   │   ├── stats.c
│   │   ├── snapshot.c
│   │   └── history.c
│   └── main.c                       # Module entry/exit (init_module/cleanup_module)

├── cli/                             # User-space command-line tools (similar to systemctl/dnf)
│   ├── moectl.c                     # Main command tool (moectl status / clean)
│   └── parser.c                     # Parameter parsing

├── agent/                           # User-space AI assistant (daemon)
│   ├── main.py                      # Main daemon, starts LLM interface
│   ├── handler/
│   │   ├── event_handler.py         # Receives kernel events, responds
│   │   ├── net_guard_handler.py
│   │   └── memory_policy.py
│   └── model/
│       ├── openai_wrapper.py        # OpenAI interface wrapper
│       └── local_llm.py             # llama.cpp / ggml model management

├── test/                            # Unit tests / Module tests
│   ├── test_runner.c
│   ├── test_mem.c
│   ├── test_utils.c
│   └── mocks/                       # Stub code that can simulate kernel behavior
│       └── fake_kernel.c

├── build/                           # Build output (.ko files etc., automatically created by Makefile)

├── Makefile                         # Main build script (can compile .ko + CLI + tests)
├── README.md                        # Project introduction and usage instructions
├── LICENSE                          # Project license (e.g., BSD / MIT / LGPL)
├── .gitignore                       # Ignore file configuration
└── .gitlab-ci.yml                   # GitLab CI/CD workflow (build + test)

```

---

## 🔧 Core Module Responsibilities

| *Area \| Functional Focus \| Support Content* |
| -------------------- |

include/ | Interface and abstraction layer | Cross-module/component header file unified management

src/ | Core implementation layer | Linux module main logic, including scheduling, module registration, communication, etc.

data/ | Status snapshots, policy records | Data and historical samples for AI model decision support

ipc/ | User-space interaction interface | Support for Netlink, /proc, shared memory, etc.

cli/ | Terminal command tools | Support for moectl style, compatible with command-line calls

agent/ | AI execution layer (user-space) | Receive events → AI model inference → Send back control policies

test/ | Automated testing support | Support for mocks, module-level testing, usable for CI/CD

---

## 📐 Communication Mechanism Design

- MVP phase uses `/proc/moeai` to implement command and status interaction between user-space and kernel
- Later phases will integrate Netlink for structured message passing, supporting event reporting and policy distribution
- Expandable support for shared memory mechanism to transfer high-frequency sampling data
- User-space `agent` interacts bidirectionally through event bus monitoring or query interfaces

---

## 🧠 AI and Data Flow Integration Plan

- Kernel module is responsible for event detection (such as memory overflow, suspicious port connections, etc.) and pushing status information
- User-space `agent/` receives events and calls external AI models for inference
- Models include:
  - OpenAI interface (via API requests)
  - Locally deployed models (such as llama.cpp, running in agent/main.py)
- AI analysis results are transmitted back to the kernel module to control behavior (such as dynamic OOM policies) through Netlink or `/proc/moeai`

---

## 📢 User Interaction Interface

- `cli/moectl` supports command formats:
  - `moectl status`: Output current system analysis status
  - `moectl clean`: Trigger memory release logic
  - `moectl protect firefox`: Mark the Firefox process as high priority
- `moectl` will write instructions through procfs, with future migration to Netlink command encapsulation

---

## 📋 Logging System Design

- `utils/logger.c` implements a unified logging interface (supporting printk / buffer / user-space readback)
- Logs can be pulled by user-space CLI tools or agent via `/proc/moeai_log`
- Support for log level control (DEBUG / INFO / WARN / ERROR)
- Log structure uses ring buffer storage to ensure it doesn't block the kernel main process

---

## 🛠 Building and Testing

- Makefile manages the building of all modules, tools, and tests
  - Supports building `.ko` modules (based on kernel headers)
  - Supports user-space test building and execution
- Tests are written using Unity or CMocka frameworks
- GitLab CI automatically executes build and test workflows, outputs build artifacts

---

## ✅ Milestone Goals to be Implemented

1. **Build directory structure and initialization files**
   - Create all directories, basic `.c/.h` framework files
   - Initialize Makefile and build environment

2. **Design functional responsibilities of code files**
   - Clearly define target functionality for each module (core/module/ipc/cli/agent)
   - Clarify function interfaces and struct data for each `.c` file

3. **Determine relationships between code files**
   - Parameter passing: Use structs to package parameters, use unified command structures for scheduling
   - Cross-module header file path design (unified include/...)

4. **Define interaction mechanisms between core components**
   - agent triggers CLI commands → CLI writes to /proc or sends Netlink
   - Kernel module receives messages → calls module processors → status updates or feedback

5. **Write call sequence use cases (system operation flow chart)**
   - System initialization → module registration → event detection → event distribution → data processing → decision feedback → user-space response

6. **Prepare logging and debugging interfaces**
   - Support `logger()` interface output to ring buffer
   - Provide `cat /proc/moeai_log` to view recent logs

7. **Build basic testing framework**
   - Initialize CMocka / Unity test entry points
   - Provide at least one module (such as mem_monitor) unit test example

8. **Integrate GitLab CI/CD initial templates**
   - Build module `.ko` + CLI tools
   - Execute tests and produce coverage reports

9. **Write modular extension conventions and code specification documents**
   - Clearly define template structures and registration methods for future module additions
   - Standardize code style (indentation, comments, function prefixes, etc.)

---

## 📄 Protocol and Deployment

- Project open source protocol is tentatively set as MIT/BSD (to be finalized based on AI component references)
- Support for deploying Runner on GitLab to execute the complete CI/CD workflow
- Future support for automatic module deployment, synchronization of AI permissions and policy model configurations

---