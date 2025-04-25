# MoeAI-C 中文消息定义
# 此文件包含 Makefile 中使用的所有中文文本

# 基本命令消息
MSG_BUILD_MODULE = 编译内核模块...
MSG_CLEAN_BUILD = 清理构建文件...
MSG_INSTALL_MODULE = 安装模块...
MSG_UNINSTALL_MODULE = 卸载模块...
MSG_TEST_MODULE = 加载测试模块...
MSG_BUILD_CLI = 构建CLI工具...
MSG_CLI_COMPLETE = CLI工具构建完成
MSG_MODULE_SUCCESS = 内核模块构建完成

# QEMU 相关消息
MSG_QEMU_BUILD = 为 QEMU 环境构建模块（内核版本: $(QEMU_KERNEL_VERSION))...
MSG_QEMU_ERROR_DIR = 错误: QEMU 内核源码目录不存在
MSG_QEMU_SET_DIR = 请设置 QEMU_KERNEL_SRC 为正确的内核源码路径
MSG_QEMU_USING_KERNEL = 使用内核源码
# QEMU 清理相关消息
MSG_QEMU_CLEANUP = 清理QEMU测试环境...
MSG_QEMU_TEST_RUN = 运行标准QEMU测试（使用与CI测试相同的方法）...
MSG_PREPARE_TEST_DIR = 准备测试环境目录...
MSG_QEMU_TEST_ENV_READY = 测试环境准备完成
MSG_QEMU_PACK = 打包测试环境initramfs...
MSG_QEMU_PACKED = initramfs已打包到
MSG_QEMU_MINIMAL = 创建最小化测试initramfs...
MSG_MINIMAL_CREATED = 最小化initramfs已创建
MSG_USE_TEST_CMD = 使用以下命令测试
MSG_CI_TEST_RUN = 运行CI环境自动化测试...
MSG_CLEAN_TEST_ENV = 清理测试环境...
MSG_TEST_ENV_CLEANED = 测试环境已清理
MSG_START_QEMU = 启动 QEMU...
MSG_QEMU_COMPLETED = QEMU 测试已完成
MSG_MANUAL_QEMU = 手动模式启动 QEMU...
MSG_CHECK_COMPAT = 检查 initramfs 和内核模块兼容性...
MSG_QEMU_KERNEL_VER = QEMU 内核版本
MSG_HOST_KERNEL_VER = 当前主机内核版本
MSG_MODULE_INFO = 模块信息
MSG_NO_MODULE_INFO = 无法获取模块信息
MSG_MODULE_NOT_EXIST = 模块文件不存在，请先构建
MSG_INITRAMFS_EXISTS = initramfs 文件存在
MSG_INITRAMFS_NOT_EXISTS = initramfs 文件不存在

# 配置相关消息
MSG_START_CONFIG = 启动自动配置...
MSG_CONFIG_CREATED = 已创建配置文件
MSG_CHECK_CONFIG = 请检查配置是否正确，如有必要，请手动编辑
MSG_CONFIG_EXISTS = 配置文件已存在
MSG_CONFIG_DELETE = 如需重新配置，请先删除此文件
MSG_DETECTED_ZH = 已检测到中文环境，设置默认语言为中文
MSG_DETECTED_EN = 检测到非中文环境，设置默认语言为英文

# CI 和目录相关消息
MSG_CI_BUILD = CI 构建：在 CI 环境中构建项目...
MSG_COMPILE_TOOLS = 编译工具函数...
MSG_COMPILE_FILE = 编译
MSG_COMPILE_FAIL = 编译失败，但继续执行
MSG_KERNEL_HEADERS = 注意：完整模块编译需要在有内核头文件的环境中进行

# 帮助信息（仅包括部分关键项，完整帮助内容在help目标中定义）
MSG_HELP_TITLE = MoeAI-C 项目 Makefile 帮助
MSG_HELP_BASIC = 基本目标
MSG_HELP_QEMU = QEMU 测试目标
MSG_HELP_OTHER = 其他目标
MSG_HELP_CONFIG = 配置变量

# 自检相关消息
MSG_RUN_SELFTEST = 运行自检工具...
MSG_EXEC_SELFTEST = 执行 moectl selftest 命令
MSG_SELFTEST_FAIL = 自检失败
MSG_NO_MOECTL = ERROR: moectl 工具未构建，请先运行 'make cli'

# 增加测试相关消息
MSG_VIEW_OUTPUT = 查看测试输出...
MSG_RUNNING_SELFTEST = 运行自检...
MSG_REMOVE_MODULE = 卸载测试模块...

# 代码检查相关消息
MSG_RUN_CODE_CHECK = 运行代码风格检查...
MSG_CHECK_FILE = 检查

# 初始化脚本相关消息
MSG_INIT_MODULE_START = [init] MoeAI-C 测试环境启动
MSG_INIT_SEPARATOR = ============================================
MSG_INIT_LOAD_MODULE = [init] 尝试加载 moeai 模块...
MSG_INIT_LOAD_FAIL = [init] 模块加载失败
MSG_INIT_ENSURE_LOADED = [init] 确保模块加载完成...
MSG_INIT_MODULE_LOADED = [init] 模块已成功加载
MSG_INIT_RUN_SELFTEST = [init] 执行 moectl selftest 命令...
MSG_INIT_SELFTEST_FAIL = [init] 自检失败
MSG_INIT_MODULE_NOT_LOADED = [init] moeai模块未加载，跳过自检
MSG_INIT_TEST_COMPLETE = [init] 测试完成，启动shell...

# 帮助信息
MSG_HELP_BASIC_TARGETS = 基本目标:
MSG_HELP_ALL = all        - 构建内核模块 (moeai.ko)
MSG_HELP_CLI = cli        - 构建命令行工具 (moectl)
MSG_HELP_CLEAN = clean      - 清理所有构建文件
MSG_HELP_INSTALL = install    - 安装模块到系统
MSG_HELP_UNINSTALL = uninstall  - 从系统卸载模块
MSG_HELP_TEST = test       - 在本地系统测试模块加载
MSG_HELP_CHECK = check      - 运行代码风格检查

# 帮助信息中QEMU测试目标
MSG_HELP_QEMU_TEST = 完整 QEMU 测试流程 (构建+复制+打包+运行)
MSG_HELP_QEMU_BUILD = 为 QEMU 环境特别构建模块 (QEMU_KERNEL_SRC=路径)
MSG_HELP_QEMU_COPY = 复制模块到 initramfs 目录
MSG_HELP_QEMU_PACK = 打包 initramfs 为 cpio.gz 映像
MSG_HELP_QEMU_RUN = 运行 QEMU 虚拟机
MSG_HELP_QEMU_CHECK = 检查 QEMU 环境兼容性
MSG_HELP_QEMU_MINIMAL = 创建最小化 initramfs 用于测试

# 帮助信息中其他目标
MSG_HELP_MKDIR = 创建构建所需的目录
MSG_HELP_DIRS = 创建完整项目结构
MSG_HELP_CI_BUILD = GitLab CI 专用构建目标
MSG_HELP_HELP = 显示此帮助信息

# 帮助信息中配置变量
MSG_HELP_QEMU_SRC = QEMU 中运行的内核源码路径 (默认: ../../linux-6.5.7)
MSG_HELP_QEMU_VER = QEMU 内核版本 (默认: 6.5.7)
MSG_HELP_INITRAMFS_DIR = initramfs 目录路径 (默认: ../../initramfs)
MSG_HELP_INITRAMFS_IMAGE = 输出的 initramfs 镜像路径 (默认: ../../initramfs.cpio.gz)
MSG_HELP_QEMU_SCRIPT = QEMU 启动脚本 (默认: ../../scripts/run_qemu_externKernel.sh)

# 配置文件注释
MSG_CONFIG_AUTO_GENERATED = 自动生成的配置文件 - 可以手动编辑
MSG_CONFIG_GENERATED_BY = 由 'make configure' 在 %s 生成
MSG_CONFIG_LANG_SETTING = 检测到的系统语言设置

# 警告消息
MSG_WARNING_NO_KERNEL_DIR = 无法找到内核源码目录，请设置 KERNEL_DIR=/path/to/kernel/source
MSG_WARNING_NO_QEMU_KERNEL = 未设置QEMU_KERNEL_SRC，使用默认值: $(QEMU_KERNEL_SRC)
MSG_WARNING_TEST_FAIL = 如果测试失败，请设置正确的内核源码路径
MSG_WARNING_DEVTMPFS = devtmpfs mount failed