################################################################################
# MoeAI-C - 智能内核助手模块
#
# 文件: Makefile
# 描述: 项目构建系统
#
# 版权所有 © 2025 @ydzat
################################################################################

# 模块名称
obj-m := moeai.o
moeai-objs := src/main.o \
              src/core/version.o \
              src/modules/mem_monitor.o \
              src/ipc/procfs.o \
              src/utils/logger.o \
              src/utils/ring_buffer.o

# 内核模块编译
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KBUILD_EXTRA_SYMBOLS ?= 

# 使用内核的自身编译系统
all:
	@echo "编译内核模块..."
	make -C $(KERNEL_DIR) M=$(PWD) modules

# 清理构建
clean:
	@echo "清理构建文件..."
	make -C $(KERNEL_DIR) M=$(PWD) clean

# 安装模块
install:
	@echo "安装模块..."
	make -C $(KERNEL_DIR) M=$(PWD) modules_install
	/sbin/depmod -a

# 卸载模块
uninstall:
	@echo "卸载模块..."
	rm -f /lib/modules/$(shell uname -r)/extra/moeai.ko
	/sbin/depmod -a

# 测试模块加载
test: all
	@echo "加载测试模块..."
	-sudo rmmod moeai 2>/dev/null || true
	sudo insmod moeai.ko
	@echo "查看测试输出..."
	sudo dmesg | tail -n 10
	@echo "卸载测试模块..."
	-sudo rmmod moeai

# CLI工具目标 (将在未来实现)
cli:
	@echo "构建CLI工具..."
	$(CC) -Wall -Iinclude cli/moectl.c -o build/moectl
	@echo "CLI工具构建完成"

# 创建构建目录
mkdir:
	@mkdir -p build

# 创建必要的目录结构
dirs:
	@mkdir -p src/core src/data src/ipc src/modules src/utils include/core include/data include/ipc include/modules include/utils cli build

.PHONY: all clean install uninstall test cli mkdir dirs