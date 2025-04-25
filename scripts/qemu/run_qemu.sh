#!/bin/bash

# @Author: @ydzat
# @Date: 2025-04-25
# @LastEditors: @ydzat
# @Description: MoeAI-C QEMU 启动脚本

# 记录原始工作目录
ORIGINAL_PWD=$(pwd)

# 配置文件支持
CONFIG_FILE="$1"
if [ -n "$CONFIG_FILE" ] && [ -f "$CONFIG_FILE" ]; then
    echo "使用配置文件: $CONFIG_FILE"
    source "$CONFIG_FILE"
else
    echo "使用默认配置"
fi

# 解析环境变量中的路径 (处理${PWD}和${QEMU_KERNEL_SRC}等变量)
eval_path() {
    local path="$1"
    # 替换${PWD}为原始工作目录
    path="${path/\$\{PWD\}/$ORIGINAL_PWD}"
    # 替换$PWD为原始工作目录
    path="${path/\$PWD/$ORIGINAL_PWD}"
    # 解析其他可能的环境变量
    eval echo "$path"
}

# 默认配置，可以被配置文件覆盖
HDA="${HDA:-}"
HDB="${HDB:-}"
SHARED="${SHARED:-../share}"
KERNEL="${KERNEL:-../linux-6.5.7/arch/x86/boot/bzImage}"
INITRD="${INITRD:-../initramfs.cpio.gz}"
CMDLINE="${CMDLINE:-rdinit=/init console=ttyS0 root=/dev/ram0 init=/init}"
FLAGS="${FLAGS:---enable-kvm}"

# 解析路径中的环境变量
if [ -n "$KERNEL" ]; then
    KERNEL=$(eval_path "$KERNEL")
fi

if [ -n "$INITRD" ]; then
    INITRD=$(eval_path "$INITRD")
fi

if [ -n "$SHARED" ]; then
    SHARED=$(eval_path "$SHARED")
fi

if [ -n "$HDA" ]; then
    HDA=$(eval_path "$HDA")
fi

if [ -n "$HDB" ]; then
    HDB=$(eval_path "$HDB")
fi

# 如果设置了共享目录路径，则启用共享目录
if [ -n "$SHARED" ] && [ -d "$SHARED" ]; then
    VIRTFS="--virtfs local,path=${SHARED},mount_tag=share,security_model=passthrough,id=share"
else
    VIRTFS=""
fi

echo "启动 QEMU..."
echo "内核: $KERNEL"
echo "initramfs: $INITRD"
echo "内核参数: $CMDLINE"

# 检查文件是否存在
if [ ! -f "$KERNEL" ]; then
    echo "错误: 内核镜像不存在: $KERNEL"
    exit 1
fi

if [ ! -f "$INITRD" ]; then
    echo "错误: initramfs文件不存在: $INITRD"
    exit 1
fi

# 调试输出，检查initramfs内容
if [ -x "$(command -v zcat)" ] && [ -x "$(command -v cpio)" ]; then
    echo "检查initramfs内容..."
    TMP_DIR=$(mktemp -d)
    cd $TMP_DIR
    zcat "$INITRD" | cpio -t 2>/dev/null | grep init || echo "警告: 在initramfs中未找到init文件!"
    zcat "$INITRD" | cpio -t 2>/dev/null | grep -i moeai || echo "警告: 在initramfs中未找到moeai.ko模块!"
    cd - > /dev/null
    rm -rf $TMP_DIR
fi

# 启动 QEMU
exec qemu-system-x86_64 ${FLAGS} \
    ${HDA:+"-drive file=$HDA,format=raw"} \
    ${HDB:+"-drive file=$HDB,format=raw"} \
    ${VIRTFS} \
    -net user -net nic \
    -nographic \
    -boot c -m 1G \
    -kernel "${KERNEL}" \
    -initrd "${INITRD}" \
    -append "${CMDLINE}"