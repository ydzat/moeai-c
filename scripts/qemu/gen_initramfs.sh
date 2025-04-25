#!/bin/bash

# @Author: @ydzat
# @Date: 2025-04-25
# @LastEditors: @ydzat
# @Description: Generate minimal initramfs

# Set working directory and output path
WORK_DIR="${1:-/tmp/moeai_initramfs}"
OUTPUT_IMAGE="${2:-$(pwd)/minimal_initramfs.cpio.gz}"
INIT_TEMPLATE="${3:-$(dirname $0)/init_scripts/minimal_init.sh}"

# Module and tool paths
MODULE_PATH="$(pwd)/moeai.ko"
MOECTL_PATH="$(pwd)/build/bin/moectl"

echo "Generating minimal initramfs..."
echo "Working directory: $WORK_DIR"
echo "Output file: $OUTPUT_IMAGE"
echo "Using init template: $INIT_TEMPLATE"

# Ensure module is built
if [ ! -f "$MODULE_PATH" ]; then
    echo "Error: Module file not found: $MODULE_PATH"
    echo "Please build the module first!"
    exit 1
fi

# Ensure moectl tool is built
if [ ! -f "$MOECTL_PATH" ]; then
    echo "Warning: moectl tool not found: $MOECTL_PATH"
    echo "Attempting to build moectl..."
    if [ -f "$(pwd)/cli/moectl.c" ]; then
        mkdir -p $(pwd)/build/bin
        gcc -Wall -I$(pwd)/include $(pwd)/cli/moectl.c -o $MOECTL_PATH -static
        if [ ! -f "$MOECTL_PATH" ]; then
            echo "Error: Failed to build moectl tool!"
        else
            echo "Successfully built moectl tool"
        fi
    else
        echo "Error: Cannot find moectl source code!"
    fi
fi

# Clean working directory (if exists)
if [ -d "$WORK_DIR" ]; then
    rm -rf "$WORK_DIR"
fi

# Create initramfs directory structure
mkdir -p "$WORK_DIR"/{bin,dev,proc,sys,lib/modules,sbin,root,usr/bin}

# Create basic device nodes
mknod -m 622 "$WORK_DIR/dev/console" c 5 1 2>/dev/null || echo "Warning: Failed to create console device node"
mknod -m 622 "$WORK_DIR/dev/tty0" c 4 0 2>/dev/null || echo "Warning: Failed to create tty0 device node"
mknod -m 622 "$WORK_DIR/dev/tty1" c 4 1 2>/dev/null || echo "Warning: Failed to create tty1 device node"
mknod -m 622 "$WORK_DIR/dev/null" c 1 3 2>/dev/null || echo "Warning: Failed to create null device node"

# Copy init script template
if [ -f "$INIT_TEMPLATE" ]; then
    cp "$INIT_TEMPLATE" "$WORK_DIR/init"
    chmod +x "$WORK_DIR/init"
else
    echo "Warning: Init template file not found, creating default init script"
    cat > "$WORK_DIR/init" << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys

echo "MoeAI-C test environment starting"
echo "============================================"
echo "Attempting to load kernel module..."
insmod /lib/modules/moeai.ko || echo "Module load failed"

echo "Running module self-test..."
if [ -f "/proc/moeai/control" ]; then
    echo "selftest" > /proc/moeai/control
    echo "Self-test results:"
    cat /proc/moeai/selftest
else
    echo "Cannot access procfs interface, self-test failed"
    echo "Checking /proc directory:"
    ls -la /proc | grep -E "moeai|moe"
fi

echo "============================================"
echo "Tests completed, starting shell..."
exec /bin/sh
EOF
    chmod +x "$WORK_DIR/init"
fi

# Copy moectl command line tool
if [ -f "$MOECTL_PATH" ]; then
    echo "Copying moectl tool to initramfs..."
    cp "$MOECTL_PATH" "$WORK_DIR/bin/moectl"
    chmod +x "$WORK_DIR/bin/moectl"
    
    # Also put in /usr/bin as backup
    cp "$MOECTL_PATH" "$WORK_DIR/usr/bin/moectl"
    chmod +x "$WORK_DIR/usr/bin/moectl"
else
    echo "Warning: moectl tool not found, cannot copy!"
fi

# Find busybox in system
BUSYBOX_PATH=$(which busybox 2>/dev/null)
if [ -n "$BUSYBOX_PATH" ] && [ -x "$BUSYBOX_PATH" ]; then
    echo "Found busybox: $BUSYBOX_PATH"
    # Copy busybox to initramfs
    cp "$BUSYBOX_PATH" "$WORK_DIR/bin/busybox"
    chmod +x "$WORK_DIR/bin/busybox"
    
    # Create symlinks for basic commands
    cd "$WORK_DIR/bin"
    for cmd in sh ls cat echo grep mount insmod lsmod rmmod dmesg; do
        ln -sf busybox "$cmd"
    done
    cd - > /dev/null
    
    echo "Created symlinks for basic commands"
else
    echo "Warning: Busybox not found in system, trying system commands"
    
    # If no busybox, try copying actual system command binaries (not symlinks)
    for cmd in sh ls cat echo grep mount insmod; do
        CMD_PATH=$(which "$cmd" 2>/dev/null)
        if [ -n "$CMD_PATH" ]; then
            # 检查是否是符号链接
            if [ -L "$CMD_PATH" ]; then
                # 如果是符号链接，获取实际的目标文件
                REAL_PATH=$(readlink -f "$CMD_PATH")
                if [ -f "$REAL_PATH" ]; then
                    echo "Copying $cmd ($REAL_PATH) to initramfs"
                    cp "$REAL_PATH" "$WORK_DIR/bin/$cmd"
                    chmod +x "$WORK_DIR/bin/$cmd"
                else
                    echo "Warning: Cannot resolve real path for $cmd"
                fi
            else
                # If not symlink, copy directly
                echo "Copying $cmd to initramfs"
                cp "$CMD_PATH" "$WORK_DIR/bin/$cmd"
                chmod +x "$WORK_DIR/bin/$cmd"
            fi
        else
                echo "Warning: Cannot find command: $cmd"
        fi
    done
    
    # Check if basic shell was copied
    if [ ! -f "$WORK_DIR/bin/sh" ]; then
        echo "Error: Failed to copy basic shell (sh)! Creating simple test script as fallback..."
        cat > "$WORK_DIR/bin/sh" << 'EOF'
#!/bin/echo This environment is not available -
echo "Warning: No shell available in this environment."
echo "Basic initialization and module loading completed, but cannot enter interactive shell."
echo "System will shutdown in 5 seconds..."
sleep 5
echo 1 > /proc/sys/kernel/sysrq
echo o > /proc/sysrq-trigger
EOF
        chmod +x "$WORK_DIR/bin/sh"
    fi
fi

    # Check if statically compiled commands exist
if [ -d "/usr/local/musl/bin" ]; then
    echo "Found musl static toolchain, copying basic commands..."
    for cmd in sh ls cat grep; do
        if [ -f "/usr/local/musl/bin/$cmd" ]; then
            echo "Copying statically compiled $cmd to initramfs"
            cp "/usr/local/musl/bin/$cmd" "$WORK_DIR/bin/"
            chmod +x "$WORK_DIR/bin/$cmd"
        fi
    done
fi

    # Copy shared libraries (if needed and not statically linked)
copy_needed_libs() {
    local binary="$1"
    local dest_dir="$2"
    
    # Check if statically linked
    if ldd "$binary" 2>&1 | grep -q "not a dynamic executable"; then
        echo "Skipping statically linked binary $binary"
        return
    fi
    
    echo "Copying shared libraries needed by $binary"
    for lib in $(ldd "$binary" 2>/dev/null | grep -o '/lib.*\.so[0-9.]*' | sort -u); do
        if [ -f "$lib" ]; then
            # Create target directory
            mkdir -p "$dest_dir$(dirname "$lib")"
            echo "  Copying $lib"
            cp "$lib" "$dest_dir$(dirname "$lib")/"
        fi
    done
}

# Check each non-symlink file in bin directory, copy required libraries
for binary in $(find "$WORK_DIR/bin" -type f -not -name "busybox"); do
    if [ -x "$binary" ]; then
        copy_needed_libs "$binary" "$WORK_DIR"
    fi
done

# Copy module to initramfs
echo "Copying module to initramfs..."
cp "$MODULE_PATH" "$WORK_DIR/lib/modules/"

# Prepare necessary tools for insmod if not using busybox
if [ ! -L "$WORK_DIR/bin/insmod" ] && [ ! -f "$WORK_DIR/bin/insmod" ]; then
    INSMOD_PATH=$(which insmod 2>/dev/null)
    if [ -n "$INSMOD_PATH" ]; then
        echo "Copying insmod to initramfs"
        cp "$INSMOD_PATH" "$WORK_DIR/bin/"
        chmod +x "$WORK_DIR/bin/insmod"
        # Copy libraries needed by insmod
        copy_needed_libs "$INSMOD_PATH" "$WORK_DIR"
    else
        echo "Warning: Cannot find insmod, module loading may fail"
    fi
fi

# Pack initramfs
echo "Packing initramfs..."
cd "$WORK_DIR" && find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > "$OUTPUT_IMAGE"

echo "Initramfs generation completed: $OUTPUT_IMAGE"
exit 0
