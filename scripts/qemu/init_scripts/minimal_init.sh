#!/bin/sh
###
 # @Author: @ydzat
 # @Date: 2025-04-25 14:32:08
 # @LastEditors: @ydzat
 # @LastEditTime: 2025-04-25 16:14:33
 # @Description: 
### 
# 最小化init脚本，用于基本测试

mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev 2>/dev/null || echo "devtmpfs mount failed"

# 设置PATH确保能找到命令
export PATH=/bin:/sbin:/usr/bin:/usr/sbin

echo "==============================================="
echo "MoeAI-C 内核模块测试环境"
echo "最小化initramfs启动"
echo "==============================================="

echo "加载 MoeAI-C 模块..."
insmod /lib/modules/moeai.ko || echo "模块加载失败"
sleep 1

# 检查模块是否成功加载
if grep -q moeai /proc/modules; then
    echo "模块已成功加载到内核"
else
    echo "警告: 模块可能未正确加载，检查dmesg输出:"
    dmesg | tail -n 10
fi

# 检查procfs接口
echo "检查procfs接口..."
if [ -d "/proc/moeai" ]; then
    echo "找到procfs目录: /proc/moeai"
    ls -la /proc/moeai/
elif [ -d "/proc/moeai-c" ]; then
    echo "找到procfs目录: /proc/moeai-c"
    ls -la /proc/moeai-c/
else
    echo "警告: 找不到procfs接口目录，列出所有procfs条目:"
    ls -la /proc | grep -E "moe|moeai"
fi

# 尝试使用moectl工具
echo "尝试运行moectl工具..."
if [ -f "/bin/moectl" ]; then
    echo "找到moectl工具，执行自检测试:"
    /bin/moectl selftest || echo "moectl selftest失败"
elif [ -f "/usr/bin/moectl" ]; then
    echo "找到moectl工具，执行自检测试:"
    /usr/bin/moectl selftest || echo "moectl selftest失败"
else
    echo "警告: 找不到moectl工具，请确认它被正确复制到测试环境"
    find / -name moectl -type f 2>/dev/null || echo "没有找到moectl"
fi

echo "==============================================="
echo "进入交互式shell"
exec /bin/sh