# MoeAI-C English Message Definitions
# This file contains all English text used in Makefile

# Basic command messages
MSG_BUILD_MODULE = Building kernel module...
MSG_CLEAN_BUILD = Cleaning build files...
MSG_INSTALL_MODULE = Installing module...
MSG_UNINSTALL_MODULE = Uninstalling module...
MSG_TEST_MODULE = Loading test module...
MSG_BUILD_CLI = Building CLI tool...
MSG_CLI_COMPLETE = CLI tool build complete
MSG_MODULE_SUCCESS = Kernel module build complete

# QEMU related messages
MSG_QEMU_BUILD = Building module for QEMU environment (kernel version: $(QEMU_KERNEL_VERSION))...
MSG_QEMU_ERROR_DIR = Error: QEMU kernel source directory does not exist
MSG_QEMU_SET_DIR = Please set QEMU_KERNEL_SRC to correct kernel source path
MSG_QEMU_USING_KERNEL = Using kernel source
# QEMU cleanup messages
MSG_QEMU_CLEANUP = Cleaning up QEMU test environment...
MSG_QEMU_TEST_RUN = Running standard QEMU test (using same method as CI tests)...
MSG_PREPARE_TEST_DIR = Preparing test environment directory...
MSG_QEMU_TEST_ENV_READY = Test environment ready
MSG_QEMU_PACK = Packing test environment initramfs...
MSG_QEMU_PACKED = initramfs packed to
MSG_QEMU_MINIMAL = Creating minimal test initramfs...
MSG_MINIMAL_CREATED = Minimal initramfs created
MSG_USE_TEST_CMD = Use following command to test
MSG_CI_TEST_RUN = Running CI environment automated tests...
MSG_CLEAN_TEST_ENV = Cleaning test environment...
MSG_TEST_ENV_CLEANED = Test environment cleaned
MSG_START_QEMU = Starting QEMU...
MSG_QEMU_COMPLETED = QEMU test completed
MSG_MANUAL_QEMU = Starting QEMU in manual mode...
MSG_CHECK_COMPAT = Checking initramfs and kernel module compatibility...
MSG_QEMU_KERNEL_VER = QEMU kernel version
MSG_HOST_KERNEL_VER = Current host kernel version
MSG_MODULE_INFO = Module info
MSG_NO_MODULE_INFO = Cannot get module info
MSG_MODULE_NOT_EXIST = Module file does not exist, please build first
MSG_INITRAMFS_EXISTS = initramfs file exists
MSG_INITRAMFS_NOT_EXISTS = initramfs file does not exist

# Configuration messages
MSG_START_CONFIG = Starting auto-configuration...
MSG_CONFIG_CREATED = Configuration file created
MSG_CHECK_CONFIG = Please check configuration and edit manually if needed
MSG_CONFIG_EXISTS = Configuration file exists
MSG_CONFIG_DELETE = Delete this file if you need to reconfigure
MSG_DETECTED_ZH = Detected Chinese environment, setting default language to Chinese
MSG_DETECTED_EN = Detected non-Chinese environment, setting default language to English

# CI and directory messages
MSG_CI_BUILD = CI build: Building project in CI environment...
MSG_COMPILE_TOOLS = Compiling tool functions...
MSG_COMPILE_FILE = Compiling
MSG_COMPILE_FAIL = Compilation failed but continuing
MSG_KERNEL_HEADERS = Note: Full module compilation requires kernel headers

# Help information
MSG_HELP_TITLE = MoeAI-C Project Makefile Help
MSG_HELP_BASIC = Basic targets
MSG_HELP_QEMU = QEMU test targets
MSG_HELP_OTHER = Other targets
MSG_HELP_CONFIG = Configuration variables

# Self-test messages
MSG_RUN_SELFTEST = Running self-test tool...
MSG_EXEC_SELFTEST = Executing moectl selftest command
MSG_SELFTEST_FAIL = Self-test failed
MSG_NO_MOECTL = ERROR: moectl tool not built, please run 'make cli' first

# Test related messages
MSG_VIEW_OUTPUT = Viewing test output...
MSG_RUNNING_SELFTEST = Running self-test...
MSG_REMOVE_MODULE = Unloading test module...

# Code check messages
MSG_RUN_CODE_CHECK = Running code style check...
MSG_CHECK_FILE = Checking

# Init script messages
MSG_INIT_MODULE_START = [init] MoeAI-C test environment starting
MSG_INIT_SEPARATOR = ============================================
MSG_INIT_LOAD_MODULE = [init] Attempting to load moeai module...
MSG_INIT_LOAD_FAIL = [init] Module load failed
MSG_INIT_ENSURE_LOADED = [init] Ensuring module is loaded...
MSG_INIT_MODULE_LOADED = [init] Module successfully loaded
MSG_INIT_RUN_SELFTEST = [init] Executing moectl selftest command...
MSG_INIT_SELFTEST_FAIL = [init] Self-test failed
MSG_INIT_MODULE_NOT_LOADED = [init] moeai module not loaded, skipping self-test
MSG_INIT_TEST_COMPLETE = [init] Test complete, starting shell...

# Help information
MSG_HELP_BASIC_TARGETS = Basic targets:
MSG_HELP_ALL = all        - Build kernel module (moeai.ko)
MSG_HELP_CLI = cli        - Build command line tool (moectl)
MSG_HELP_CLEAN = clean      - Clean all build files
MSG_HELP_INSTALL = install    - Install module to system
MSG_HELP_UNINSTALL = uninstall  - Uninstall module from system
MSG_HELP_TEST = test       - Test module loading on local system
MSG_HELP_CHECK = check      - Run code style checks

# QEMU test targets in help
MSG_HELP_QEMU_TEST = Full QEMU test process (build+copy+pack+run)
MSG_HELP_QEMU_BUILD = Build module specifically for QEMU environment (QEMU_KERNEL_SRC=path)
MSG_HELP_QEMU_COPY = Copy module to initramfs directory
MSG_HELP_QEMU_PACK = Pack initramfs as cpio.gz image
MSG_HELP_QEMU_RUN = Run QEMU virtual machine
MSG_HELP_QEMU_CHECK = Check QEMU environment compatibility
MSG_HELP_QEMU_MINIMAL = Create minimal initramfs for testing

# Other targets in help
MSG_HELP_MKDIR = Create required build directories
MSG_HELP_DIRS = Create complete project structure
MSG_HELP_CI_BUILD = GitLab CI specific build target
MSG_HELP_HELP = Show this help information

# Configuration variables in help
MSG_HELP_QEMU_SRC = Kernel source path for QEMU (default: ../../linux-6.5.7)
MSG_HELP_QEMU_VER = QEMU kernel version (default: 6.5.7)
MSG_HELP_INITRAMFS_DIR = initramfs directory path (default: ../../initramfs)
MSG_HELP_INITRAMFS_IMAGE = Output initramfs image path (default: ../../initramfs.cpio.gz)
MSG_HELP_QEMU_SCRIPT = QEMU startup script (default: ../../scripts/run_qemu_externKernel.sh)

# Configuration file comments
MSG_CONFIG_AUTO_GENERATED = Auto-generated configuration file - can be manually edited
MSG_CONFIG_GENERATED_BY = Generated by 'make configure' at %s
MSG_CONFIG_LANG_SETTING = Detected system language settings

# Warning messages
MSG_WARNING_NO_KERNEL_DIR = Cannot find kernel source directory, please set KERNEL_DIR=/path/to/kernel/source
MSG_WARNING_NO_QEMU_KERNEL = QEMU_KERNEL_SRC not set, using default: $(QEMU_KERNEL_SRC)
MSG_WARNING_TEST_FAIL = If test fails, please set correct kernel source path
MSG_WARNING_DEVTMPFS = devtmpfs mount failed
