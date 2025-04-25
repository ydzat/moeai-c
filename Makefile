################################################################################
# MoeAI-C - 智能内核助手模块 / Intelligent Kernel Assistant Module
#
# 文件/File: Makefile
# 描述/Description: 项目构建系统 / Project Build System
#
# 版权所有/Copyright © 2025 @ydzat
################################################################################

# 加载可选的本地配置（如果存在）
-include .config.mk

# 语言支持 / Language Support
# 如果配置文件中没有设置语言，则根据系统环境检测
ifndef MOEAI_LANG
    SHELL_LANG := $(shell echo $$LANG)
    ifneq ($(findstring zh_CN,$(SHELL_LANG)),)
        MOEAI_LANG := zh
    else
        MOEAI_LANG := en
    endif
endif

# 加载对应语言的消息文件
-include lang/$(MOEAI_LANG)/messages.mk

# 模块名称
obj-m := moeai.o
moeai-objs := src/main.o \
              src/core/version.o \
              src/modules/mem_monitor.o \
              src/ipc/procfs.o \
              src/utils/logger.o \
              src/utils/ring_buffer.o \
              src/utils/lang.o

# 内核模块编译 - 更智能地检测内核源码路径
ifeq ($(origin KERNEL_DIR),undefined)
    KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
    ifeq ($(wildcard $(KERNEL_DIR)),)
        $(warning $(MSG_WARNING_NO_KERNEL_DIR))
    endif
endif
KBUILD_EXTRA_SYMBOLS ?= 

# QEMU 测试相关配置
INIT_SCRIPT ?= $(INITRAMFS_DIR)/init
INITRAMFS_IMAGE ?= $(INITRAMFS_DIR).cpio.gz
OUTPUT_IMAGE = $(INITRAMFS_IMAGE)
INITRAMFS_MODULE_PATH ?= $(INITRAMFS_DIR)/lib/modules

# 添加测试环境目录
TESTENV_DIR := $(PWD)/testenv
TESTENV_INITRAMFS_DIR := $(TESTENV_DIR)/initramfs
TESTENV_INITRAMFS_IMAGE := $(TESTENV_DIR)/initramfs.cpio.gz
MINIMAL_INITRAMFS = $(TESTENV_DIR)/minimal_initramfs.cpio.gz

# 更新QEMU相关路径
QEMU_SCRIPT_DIR = $(PWD)/scripts/qemu
QEMU_SCRIPT ?= $(QEMU_SCRIPT_DIR)/run_qemu.sh
QEMU_CONFIG ?= $(QEMU_SCRIPT_DIR)/config/default.conf
QEMU_GEN_INITRAMFS ?= $(QEMU_SCRIPT_DIR)/gen_initramfs.sh
QEMU_INIT_TEMPLATE ?= $(QEMU_SCRIPT_DIR)/init_scripts/minimal_init.sh
QEMU_CI_CONFIG ?= $(QEMU_SCRIPT_DIR)/config/ci.conf
QEMU_CI_INIT ?= $(QEMU_SCRIPT_DIR)/init_scripts/ci_init.sh

# 更智能地检测QEMU内核源码路径
ifeq ($(origin QEMU_KERNEL_SRC),undefined)
    # 如果未定义，尝试使用KERNEL_DIR
    ifneq ($(wildcard $(KERNEL_DIR)),)
        QEMU_KERNEL_SRC ?= $(KERNEL_DIR)
    else
        QEMU_KERNEL_SRC ?= ../../linux-6.5.7
        $(warning $(MSG_WARNING_NO_QEMU_KERNEL))
        $(warning $(MSG_WARNING_TEST_FAIL))
    endif
endif
QEMU_KERNEL_VERSION ?= $(shell if [ -f "$(QEMU_KERNEL_SRC)/include/config/kernel.release" ]; then cat "$(QEMU_KERNEL_SRC)/include/config/kernel.release"; elif [ -f "$(QEMU_KERNEL_SRC)/include/generated/utsrelease.h" ]; then grep -o '".*"' "$(QEMU_KERNEL_SRC)/include/generated/utsrelease.h" | tr -d '"'; else echo "6.5.7"; fi)

# 使用内核的自身编译系统
all:
	@echo "$(MSG_BUILD_MODULE)"
	make -C $(KERNEL_DIR) M=$(PWD) modules

# 清理构建
clean:
	@echo "$(MSG_CLEAN_BUILD)"
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f build/bin/moectl

# 安装模块
install:
	@echo "$(MSG_INSTALL_MODULE)"
	make -C $(KERNEL_DIR) M=$(PWD) modules_install
	/sbin/depmod -a

# 卸载模块
uninstall:
	@echo "$(MSG_UNINSTALL_MODULE)"
	rm -f /lib/modules/$(shell uname -r)/extra/moeai.ko
	/sbin/depmod -a

# 测试模块加载
test: all cli selftest
	@echo "$(MSG_TEST_MODULE)"
	-sudo rmmod moeai 2>/dev/null || true
	sudo insmod moeai.ko
	@echo "$(MSG_VIEW_OUTPUT)"
	sudo dmesg | tail -n 20
	@echo "$(MSG_RUNNING_SELFTEST)"
	sudo build/bin/moectl selftest || echo "$(MSG_SELFTEST_FAIL)"
	@echo "$(MSG_REMOVE_MODULE)"
	-sudo rmmod moeai

# 仅运行自检
selftest: cli
	@echo "$(MSG_RUN_SELFTEST)"
	@if [ -f "build/bin/moectl" ]; then \
		echo "$(MSG_EXEC_SELFTEST)"; \
		sudo ./build/bin/moectl selftest; \
	else \
		echo "$(MSG_NO_MOECTL)"; \
		exit 1; \
	fi

# CLI工具构建
cli: mkdir
	@echo "$(MSG_BUILD_CLI)"
	$(CC) -Wall -I$(PWD)/include -I$(PWD)/include/utils -I$(PWD)/lang/en -I$(PWD)/lang/zh cli/moectl.c src/utils/lang.c -o build/bin/moectl -static
	@echo "$(MSG_CLI_COMPLETE)"

# 运行代码风格检查
check:
	@echo "$(MSG_RUN_CODE_CHECK)"
	@for f in $(shell find src include cli -name "*.c" -o -name "*.h" | grep -v ".mod.c") ; do \
		echo "$(MSG_CHECK_FILE) $$f"; \
		$(CHECK_PATCH) --no-tree --strict --file $$f || true; \
	done

# 为 QEMU 环境构建模块（使用指定的内核源码）
qemu-build:
	@echo "$(MSG_QEMU_BUILD)"
	@if [ ! -d "$(QEMU_KERNEL_SRC)" ]; then \
		echo "$(MSG_QEMU_ERROR_DIR): $(QEMU_KERNEL_SRC)"; \
		echo "$(MSG_QEMU_SET_DIR)"; \
		exit 1; \
	fi
	@echo "$(MSG_QEMU_USING_KERNEL): $(QEMU_KERNEL_SRC)"
	make -C $(QEMU_KERNEL_SRC) M=$(PWD) modules

# 清理QEMU测试环境，恢复init文件
qemu-cleanup:
	@echo "$(MSG_QEMU_CLEANUP)"
	@if [ -f "$(INIT_SCRIPT).bak" ]; then \
		echo "$(MSG_QEMU_RESTORE_INIT)"; \
		mv -f $(INIT_SCRIPT).bak $(INIT_SCRIPT); \
		echo "$(MSG_QEMU_RESTORED)"; \
	else \
		echo "$(MSG_QEMU_NO_BACKUP)"; \
	fi

# QEMU 测试相关目标
qemu-test: qemu-build
	@echo "$(MSG_QEMU_TEST_RUN)"
	@mkdir -p $(TESTENV_DIR)
	@chmod +x $(QEMU_GEN_INITRAMFS)
	@$(QEMU_GEN_INITRAMFS) "$(TESTENV_DIR)/test_init_fs" "$(TESTENV_DIR)/initramfs.cpio.gz" "$(QEMU_INIT_TEMPLATE)"
	@chmod +x $(QEMU_SCRIPT)
	@$(QEMU_SCRIPT) $(QEMU_CONFIG)

# 准备测试环境目录结构
prepare-testenv:
	@echo "$(MSG_PREPARE_TEST_DIR)"
	@mkdir -p $(TESTENV_INITRAMFS_DIR)/{bin,dev,proc,sys,lib/modules,sbin,root,usr/bin}
	@touch $(TESTENV_DIR)/.initialized

# 修改qemu-prepare目标，使用项目内的测试环境
qemu-prepare: cli prepare-testenv
	@echo "$(MSG_QEMU_PREPARE)"
	@echo "$(MSG_COPY_MODULE)"
	@cp -v moeai.ko $(TESTENV_INITRAMFS_DIR)/lib/modules/
	@echo "$(MSG_COPY_CLI)"
	@cp -v build/bin/moectl $(TESTENV_INITRAMFS_DIR)/usr/bin/
	@chmod 755 $(TESTENV_INITRAMFS_DIR)/usr/bin/moectl
	@cp -v build/bin/moectl $(TESTENV_INITRAMFS_DIR)/bin/
	@chmod 755 $(TESTENV_INITRAMFS_DIR)/bin/moectl
	@echo "$(MSG_CREATE_INIT)"
	@rm -f $(TESTENV_INITRAMFS_DIR)/init
	@echo "#!/bin/sh" > $(TESTENV_INITRAMFS_DIR)/init
	@echo "mount -t proc none /proc" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "mount -t sysfs none /sys" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "mount -t devtmpfs none /dev 2>/dev/null || echo \"$(MSG_WARNING_DEVTMPFS)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "echo \"$(MSG_INIT_MODULE_START)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "echo \"$(MSG_INIT_SEPARATOR)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "echo \"$(MSG_INIT_LOAD_MODULE)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "insmod /lib/modules/moeai.ko || echo \"$(MSG_INIT_LOAD_FAIL)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "echo \"$(MSG_INIT_ENSURE_LOADED)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "sleep 2" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "if lsmod | grep -q moeai; then" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "  echo \"$(MSG_INIT_MODULE_LOADED)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "  echo \"$(MSG_INIT_RUN_SELFTEST)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "  /bin/moectl selftest || echo \"$(MSG_INIT_SELFTEST_FAIL)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "else" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "  echo \"$(MSG_INIT_MODULE_NOT_LOADED)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "fi" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "echo \"$(MSG_INIT_SEPARATOR)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "echo \"$(MSG_INIT_TEST_COMPLETE)\"" >> $(TESTENV_INITRAMFS_DIR)/init
	@echo "exec /bin/sh" >> $(TESTENV_INITRAMFS_DIR)/init
	@chmod +x $(TESTENV_INITRAMFS_DIR)/init
	@echo "$(MSG_QEMU_TEST_ENV_READY)"

# 修改qemu-pack目标，使用项目内的测试环境
qemu-pack: qemu-prepare
	@echo "$(MSG_QEMU_PACK)"
	@cd $(TESTENV_INITRAMFS_DIR) && \
	find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > $(TESTENV_INITRAMFS_IMAGE)
	@echo "$(MSG_QEMU_PACKED): $(TESTENV_INITRAMFS_IMAGE)"

# 修改qemu-minimal目标使用新的测试环境目录
qemu-minimal:
	@echo "$(MSG_QEMU_MINIMAL)"
	@mkdir -p $(TESTENV_DIR)
	@chmod +x $(QEMU_GEN_INITRAMFS)
	@$(QEMU_GEN_INITRAMFS) "$(TESTENV_DIR)/minimal_init_fs" "$(MINIMAL_INITRAMFS)" "$(QEMU_INIT_TEMPLATE)"
	@echo "$(MSG_MINIMAL_CREATED): $(MINIMAL_INITRAMFS)"
	@echo "$(MSG_USE_TEST_CMD):"
	@echo "$(QEMU_SCRIPT) $(QEMU_CONFIG)"

# 新增CI测试目标
qemu-ci-test: qemu-build
	@echo "$(MSG_CI_TEST_RUN)"
	@mkdir -p $(TESTENV_DIR)
	@chmod +x $(QEMU_GEN_INITRAMFS)
	@$(QEMU_GEN_INITRAMFS) "$(TESTENV_DIR)/ci_init_fs" "$(TESTENV_DIR)/minimal_initramfs.cpio.gz" "$(QEMU_CI_INIT)"
	@chmod +x $(QEMU_SCRIPT)
	@$(QEMU_SCRIPT) $(QEMU_CI_CONFIG)

# 清理测试环境
clean-test:
	@echo "$(MSG_CLEAN_TEST_ENV)"
	@rm -rf $(TESTENV_DIR)
	@rm -f $(PWD)/minimal_initramfs.cpio.gz
	@echo "$(MSG_TEST_ENV_CLEANED)"

# 运行 QEMU
qemu-run:
	@echo "$(MSG_START_QEMU)"
	@$(QEMU_SCRIPT) $(QEMU_CONFIG)
	@echo "$(MSG_QEMU_COMPLETED)"

# 添加一个新目标，用于手动运行 QEMU 而不自动测试
qemu-run-manual:
	@echo "$(MSG_MANUAL_QEMU)"
	@$(QEMU_SCRIPT) $(QEMU_CONFIG)

# 检查 initramfs 和内核模块兼容性
qemu-check:
	@echo "$(MSG_CHECK_COMPAT)"
	@echo "$(MSG_QEMU_KERNEL_VER): $(QEMU_KERNEL_VERSION)"
	@echo "$(MSG_HOST_KERNEL_VER): $(shell uname -r)"
	@if [ -f "moeai.ko" ]; then \
		echo "$(MSG_MODULE_INFO):"; \
		modinfo moeai.ko || echo "$(MSG_NO_MODULE_INFO)"; \
	else \
		echo "$(MSG_MODULE_NOT_EXIST)"; \
	fi
	@if [ -f "$(TESTENV_INITRAMFS_IMAGE)" ]; then \
		echo "$(MSG_INITRAMFS_EXISTS): $(TESTENV_INITRAMFS_IMAGE)"; \
	else \
		echo "$(MSG_INITRAMFS_NOT_EXISTS)"; \
	fi

# 自动配置目标
configure:
	@echo "$(MSG_START_CONFIG)"
	@if [ ! -f ".config.mk" ]; then \
		echo "# $(MSG_CONFIG_AUTO_GENERATED)" > .config.mk; \
		echo "# $(shell printf "$(MSG_CONFIG_GENERATED_BY)" "`date`")" >> .config.mk; \
		echo "" >> .config.mk; \
		\
		# 检测系统语言环境 \
		echo "# $(MSG_CONFIG_LANG_SETTING)" >> .config.mk; \
		if locale | grep -iE "LANG=.*zh_CN" > /dev/null; then \
			echo "MOEAI_LANG=zh" >> .config.mk; \
			echo "$(MSG_DETECTED_ZH)"; \
		else \
			echo "MOEAI_LANG=en" >> .config.mk; \
			echo "$(MSG_DETECTED_EN)"; \
		fi; \
		echo "" >> .config.mk; \
		\
		# 查找内核源码路径 \
		if [ -d "/lib/modules/`uname -r`/build" ]; then \
			echo "KERNEL_DIR=/lib/modules/`uname -r`/build" >> .config.mk; \
		else \
			for dir in /usr/src/linux* /usr/src/kernels/*; do \
				if [ -d "$$dir" ] && [ -f "$$dir/Makefile" ]; then \
					echo "KERNEL_DIR=$$dir" >> .config.mk; \
					break; \
				fi; \
			done; \
		fi; \
		\
		# 设置QEMU内核源码为同一路径 \
		echo "QEMU_KERNEL_SRC=\$$(KERNEL_DIR)" >> .config.mk; \
		\
		echo "$(MSG_CONFIG_CREATED): .config.mk"; \
		echo "$(MSG_CHECK_CONFIG)"; \
	else \
		echo "$(MSG_CONFIG_EXISTS): .config.mk"; \
		echo "$(MSG_CONFIG_DELETE)"; \
	fi

# 创建构建目录
mkdir:
	@mkdir -p build/bin build/obj/core build/obj/modules build/obj/ipc build/obj/utils build/obj/data

# 创建必要的目录结构
dirs:
	@mkdir -p src/core src/data src/ipc src/modules src/utils \
		include/core include/data include/ipc include/modules include/utils \
		cli build/bin build/obj doc/design

# GitLab CI 专用目标，避免失败的命令
ci-build:
	@echo "$(MSG_CI_BUILD)"
	@mkdir -p build/obj
	@echo "$(MSG_COMPILE_TOOLS)"
	@for file in src/utils/*.c; do \
		echo "$(MSG_COMPILE_FILE) $$file"; \
		gcc -c $$file -I include -o build/obj/`basename $$file .c`.o || echo "$(MSG_COMPILE_FAIL)"; \
	done
	@echo "$(MSG_KERNEL_HEADERS)"

# 添加 help 目标
help:
	@echo "$(MSG_HELP_TITLE)"
	@echo ""
	@echo "$(MSG_HELP_BASIC_TARGETS)"
	@echo "  $(MSG_HELP_ALL)"
	@echo "  $(MSG_HELP_CLI)"
	@echo "  $(MSG_HELP_CLEAN)"
	@echo "  $(MSG_HELP_INSTALL)"
	@echo "  $(MSG_HELP_UNINSTALL)"
	@echo "  $(MSG_HELP_TEST)"
	@echo "  $(MSG_HELP_CHECK)"
	@echo ""
	@echo "$(MSG_HELP_QEMU):"
	@echo "  qemu-test   - $(MSG_HELP_QEMU_TEST)"
	@echo "  qemu-build  - $(MSG_HELP_QEMU_BUILD)"
	@echo "  qemu-copy   - $(MSG_HELP_QEMU_COPY)"
	@echo "  qemu-pack   - $(MSG_HELP_QEMU_PACK)"
	@echo "  qemu-run    - $(MSG_HELP_QEMU_RUN)"
	@echo "  qemu-check  - $(MSG_HELP_QEMU_CHECK)"
	@echo "  qemu-minimal - $(MSG_HELP_QEMU_MINIMAL)"
	@echo ""
	@echo "$(MSG_HELP_OTHER):"
	@echo "  mkdir      - $(MSG_HELP_MKDIR)"
	@echo "  dirs       - $(MSG_HELP_DIRS)"
	@echo "  ci-build   - $(MSG_HELP_CI_BUILD)"
	@echo "  help       - $(MSG_HELP_HELP)"
	@echo ""
	@echo "$(MSG_HELP_CONFIG):"
	@echo "  QEMU_KERNEL_SRC    - $(MSG_HELP_QEMU_SRC)"
	@echo "  QEMU_KERNEL_VERSION - $(MSG_HELP_QEMU_VER)"
	@echo "  INITRAMFS_DIR      - $(MSG_HELP_INITRAMFS_DIR)"
	@echo "  INITRAMFS_IMAGE    - $(MSG_HELP_INITRAMFS_IMAGE)"
	@echo "  QEMU_SCRIPT        - $(MSG_HELP_QEMU_SCRIPT)"

.PHONY: all clean install uninstall test cli check qemu-test qemu-build qemu-copy qemu-pack qemu-run qemu-check qemu-minimal configure mkdir dirs ci-build help
