#!/bin/sh
# CI environment specific init script - simplified version
# Automated testing: load module, run self-test, report results, exit

# Minimal environment compatible version

# First mount basic filesystems
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev 2>/dev/null

# Simple logging function that doesn't depend on external commands
log() {
    echo "$@"
}

log "==============================================="
log "MoeAI-C CI Test Environment"
log "Automated test sequence starting"
log "==============================================="

# Set exit status codes
SUCCESS=0
FAILURE=1
ERROR=2
STATUS=$SUCCESS

# Correct procfs path
PROCFS_ROOT="/proc/moeai"

# Function to ensure shutdown happens no matter what
shutdown_qemu() {
    log "[init] Shutting down QEMU virtual machine..."
    # Enable sysrq
    echo 1 > /proc/sys/kernel/sysrq 2>/dev/null
    # Sync filesystems
    echo s > /proc/sysrq-trigger 2>/dev/null
    # Wait for sync to complete
    sleep 1
    # Power off immediately
    echo o > /proc/sysrq-trigger 2>/dev/null
    
    # Should never reach here
    while true; do
        sleep 60
    done
}

# Simplified timeout handler (busybox's sh usually supports this)
{
    # Force shutdown after 30 seconds
    sleep 30
    log "[Timeout] Test timed out, forcing shutdown..."
    shutdown_qemu
} &

# Set language environment before loading module
# Language function loader
get_msg() {
    key=$1
    # Get message from language system, fallback to English if failed
    msg=$(lang_get "$key" 2>/dev/null || echo "$key")
    echo "$msg"
}

log "$(get_msg LANG_INIT_SETTING_ENGLISH)"
export MOEAI_LANG=en

log "$(get_msg LANG_TEST_LOADING_MODULE)"
if insmod /lib/modules/moeai.ko language=en 2>/dev/null; then
    log "$(get_msg LANG_TEST_MODULE_LOADED)"
    
    # Check initialization messages (without using tail)
    log "Checking dmesg initialization messages:"
    dmesg | grep -E "MoeAI|moeai" 2>/dev/null || log "No relevant initialization messages found"
else
    log "[FAILED] Module load failed"
    STATUS=$FAILURE
fi

    log "[TEST] Verifying procfs interface..."
if [ -d "$PROCFS_ROOT" ]; then
    log "Found procfs root directory: $PROCFS_ROOT"
    ls -la $PROCFS_ROOT 2>/dev/null || log "Failed to list procfs directory contents"
    
    if [ -f "${PROCFS_ROOT}/status" ] && [ -f "${PROCFS_ROOT}/control" ]; then
        log "[PASSED] procfs interface available"
        log "status file contents:"
        cat ${PROCFS_ROOT}/status 2>/dev/null || log "Failed to read status file"
    else
        log "[FAILED] procfs interface files incomplete"
        STATUS=$FAILURE
    fi
else
    log "[FAILED] procfs root directory ($PROCFS_ROOT) not found"
    log "Checking all contents under /proc:"
    ls -la /proc | grep -E "moe|ai" 2>/dev/null || log "No relevant procfs directory found"
    STATUS=$FAILURE
fi

log "[TEST] Running module self-test..."
if [ -f "${PROCFS_ROOT}/control" ]; then
    # Attempt to run self-test
    log "Writing selftest to control file..."
    echo "selftest" > ${PROCFS_ROOT}/control 2>/dev/null
    # Wait for self-test to complete
    sleep 2
    if [ -f "${PROCFS_ROOT}/selftest" ]; then
        log "[SELF-TEST RESULTS]"
        cat ${PROCFS_ROOT}/selftest 2>/dev/null || log "Failed to read selftest results"
        if grep -q "FAIL" ${PROCFS_ROOT}/selftest 2>/dev/null; then
            log "[FAILED] Self-test reported errors"
            STATUS=$FAILURE
        else
            log "[PASSED] Self-test completed with no critical errors"
        fi
    else
        log "[FAILED] Could not read self-test results, selftest file missing"
        STATUS=$FAILURE
    fi
else
    log "[ERROR] Could not trigger self-test, control file missing"
    STATUS=$ERROR
fi

# Unload module
if grep -q "moeai" /proc/modules 2>/dev/null; then
    log "[TEST] Unloading module..."
    if rmmod moeai 2>/dev/null; then
        log "[PASSED] Module unloaded successfully"
    else
        log "[WARNING] Module unload failed"
    fi
else
    log "[WARNING] Module not loaded, skipping unload test"
fi

log "----------------------------------------"
log "Testing completed, status code: $STATUS"

# Show results summary
log "==============================================="
log "Test results summary:"
dmesg | grep -E "MoeAI|moeai|error|fail" 2>/dev/null | head -n 15 || log "No relevant logs found"
log "==============================================="

log "[init] Test sequence completed, shutting down in 3 seconds..."
sleep 3

# 关闭QEMU
shutdown_qemu
