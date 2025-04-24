# MoeAI-C MVP 技术实现指南

本文档提供 MoeAI-C MVP 各核心组件的具体实现指南，包括代码结构、算法细节和关键实现点。

## 1. 核心模块实现

### 1.1 模块入口与退出 (main.c)

在 MVP 阶段，`main.c` 将实现以下功能：

```c
// 模块初始化函数
static int __init moeai_init(void)
{
    int ret = 0;
    
    pr_info("MoeAI-C: 内核助手模块正在初始化...\n");
    
    // 初始化日志系统
    ret = moeai_logger_init(debug_mode);
    if (ret) {
        pr_err("MoeAI-C: 日志系统初始化失败，错误码: %d\n", ret);
        return ret;
    }
    
    // 初始化内存监控模块
    ret = moeai_mem_monitor_init();
    if (ret) {
        pr_err("MoeAI-C: 内存监控模块初始化失败，错误码: %d\n", ret);
        goto err_mem_monitor;
    }
    
    // 初始化procfs接口
    ret = moeai_procfs_init();
    if (ret) {
        pr_err("MoeAI-C: procfs接口初始化失败，错误码: %d\n", ret);
        goto err_procfs;
    }
    
    // 启动内存监控
    ret = moeai_mem_monitor_start();
    if (ret) {
        pr_err("MoeAI-C: 内存监控启动失败，错误码: %d\n", ret);
        goto err_mem_start;
    }
    
    pr_info("MoeAI-C: 初始化完成，模块已就绪\n");
    return 0;

err_mem_start:
    moeai_procfs_exit();
err_procfs:
    moeai_mem_monitor_exit();
err_mem_monitor:
    moeai_logger_exit();
    return ret;
}

// 模块退出函数
static void __exit moeai_exit(void)
{
    pr_info("MoeAI-C: 模块正在卸载...\n");
    
    // 停止内存监控
    moeai_mem_monitor_stop();
    
    // 清理procfs接口
    moeai_procfs_exit();
    
    // 清理内存监控模块
    moeai_mem_monitor_exit();
    
    // 清理日志系统
    moeai_logger_exit();
    
    pr_info("MoeAI-C: 模块已成功卸载\n");
}
```

### 1.2 简化的核心初始化 (core/init.c)

MVP阶段将实现一个精简的核心初始化：

```c
// 核心初始化函数
int moeai_core_init(bool debug_mode)
{
    int ret = 0;
    
    // 设置调试模式
    moeai_debug_mode = debug_mode;
    
    // 打印版本信息
    moeai_version_info(version_buf, sizeof(version_buf));
    pr_info("%s\n", version_buf);
    
    return 0;
}

// 核心清理函数
void moeai_core_exit(void)
{
    // MVP阶段无需清理其他资源
}
```

## 2. 内存监控实现

### 2.1 内存状态获取与分析

```c
// 获取内存状态
int moeai_mem_monitor_get_stats(struct moeai_mem_stats *stats)
{
    struct sysinfo info;
    
    if (!stats)
        return -EINVAL;
    
    // 获取系统信息
    si_meminfo(&info);
    
    // 记录时间戳
    ktime_get_real_ts64(&stats->timestamp);
    
    // 填充内存信息
    stats->total_ram = info.totalram * (PAGE_SIZE / 1024);
    stats->free_ram = info.freeram * (PAGE_SIZE / 1024);
    stats->available_ram = si_mem_available() * (PAGE_SIZE / 1024);
    stats->cached_ram = global_node_page_state(NR_FILE_PAGES) * (PAGE_SIZE / 1024) -
                        total_swapcache_pages() * (PAGE_SIZE / 1024);
    stats->swap_total = info.totalswap * (PAGE_SIZE / 1024);
    stats->swap_free = info.freeswap * (PAGE_SIZE / 1024);
    
    // 计算百分比
    if (stats->total_ram > 0)
        stats->mem_usage_percent = 100 - (stats->available_ram * 100 / stats->total_ram);
    else
        stats->mem_usage_percent = 0;
    
    if (stats->swap_total > 0)
        stats->swap_usage_percent = 100 - (stats->swap_free * 100 / stats->swap_total);
    else
        stats->swap_usage_percent = 0;
    
    return 0;
}

// 内存状态检查任务
static void moeai_mem_check_task(struct timer_list *t)
{
    struct moeai_mem_monitor_private *priv = from_timer(priv, t, check_timer);
    struct moeai_mem_stats stats;
    int ret;
    
    // 获取当前内存状态
    ret = moeai_mem_monitor_get_stats(&stats);
    if (ret)
        goto reschedule;
    
    // 更新当前统计信息
    spin_lock(&priv->stats_lock);
    memcpy(&priv->current_stats, &stats, sizeof(stats));
    spin_unlock(&priv->stats_lock);
    
    // 检查阈值并采取行动
    if (stats.mem_usage_percent >= priv->config.emergency_threshold) {
        pr_warn("MoeAI-C: 内存使用率(%u%%)超过紧急阈值(%u%%)\n",
                stats.mem_usage_percent, priv->config.emergency_threshold);
        
        if (priv->config.auto_reclaim)
            moeai_mem_reclaim(MOEAI_MEM_RECLAIM_AGGRESSIVE);
            
    } else if (stats.mem_usage_percent >= priv->config.critical_threshold) {
        pr_info("MoeAI-C: 内存使用率(%u%%)超过临界阈值(%u%%)\n",
                stats.mem_usage_percent, priv->config.critical_threshold);
                
        if (priv->config.auto_reclaim)
            moeai_mem_reclaim(MOEAI_MEM_RECLAIM_MODERATE);
            
    } else if (stats.mem_usage_percent >= priv->config.warn_threshold) {
        pr_info("MoeAI-C: 内存使用率(%u%%)超过警告阈值(%u%%)\n",
                stats.mem_usage_percent, priv->config.warn_threshold);
                
        if (priv->config.auto_reclaim)
            moeai_mem_reclaim(MOEAI_MEM_RECLAIM_GENTLE);
    }

reschedule:
    // 重新调度检查任务
    if (priv->monitoring_active) {
        mod_timer(&priv->check_timer, 
                 jiffies + msecs_to_jiffies(priv->config.check_interval_ms));
    }
}
```

### 2.2 内存回收机制

```c
// 执行内存回收
long moeai_mem_reclaim(enum moeai_mem_reclaim_policy policy)
{
    long reclaimed = 0;
    
    switch (policy) {
    case MOEAI_MEM_RECLAIM_GENTLE:
        // 仅释放文件缓存
        pr_info("MoeAI-C: 执行温和内存回收\n");
        reclaimed = drop_caches_sysctl_handler(NULL, 1, NULL, 0);
        break;
        
    case MOEAI_MEM_RECLAIM_MODERATE:
        // 释放所有可回收页面
        pr_info("MoeAI-C: 执行中等强度内存回收\n");
        reclaimed = drop_caches_sysctl_handler(NULL, 3, NULL, 0);
        break;
        
    case MOEAI_MEM_RECLAIM_AGGRESSIVE:
        // 强制内存紧急回收，可能触发OOM
        pr_info("MoeAI-C: 执行积极内存回收\n");
        reclaimed = drop_caches_sysctl_handler(NULL, 3, NULL, 0);
        // 执行内存压缩
        compact_nodes();
        break;
        
    default:
        pr_err("MoeAI-C: 无效的回收策略: %d\n", policy);
        return -EINVAL;
    }
    
    return reclaimed;
}
```

## 3. procfs接口实现

### 3.1 基本procfs目录和文件创建

```c
// 初始化procfs接口
int moeai_procfs_init(void)
{
    struct proc_dir_entry *root = NULL;
    
    // 创建主目录
    root = proc_mkdir(MOEAI_PROCFS_ROOT, NULL);
    if (!root) {
        pr_err("MoeAI-C: 无法创建procfs根目录\n");
        return -ENOMEM;
    }
    
    // 保存根目录指针
    moeai_procfs_root = root;
    
    // 创建状态文件
    status_entry = proc_create(MOEAI_PROCFS_STATUS, 0444, root, 
                             &moeai_procfs_status_fops);
    if (!status_entry) {
        pr_err("MoeAI-C: 无法创建status文件\n");
        goto err_status;
    }
    
    // 创建控制文件
    control_entry = proc_create(MOEAI_PROCFS_CONTROL, 0222, root,
                              &moeai_procfs_control_fops);
    if (!control_entry) {
        pr_err("MoeAI-C: 无法创建control文件\n");
        goto err_control;
    }
    
    // 创建日志文件
    log_entry = proc_create(MOEAI_PROCFS_LOG, 0444, root,
                          &moeai_procfs_log_fops);
    if (!log_entry) {
        pr_err("MoeAI-C: 无法创建log文件\n");
        goto err_log;
    }
    
    pr_info("MoeAI-C: procfs接口创建成功\n");
    return 0;
    
err_log:
    proc_remove(control_entry);
err_control:
    proc_remove(status_entry);
err_status:
    proc_remove(root);
    moeai_procfs_root = NULL;
    return -ENOMEM;
}
```

### 3.2 状态文件实现

```c
// 状态文件的show回调
static int moeai_procfs_status_show(struct seq_file *seq, void *v)
{
    struct moeai_mem_stats stats;
    char version_buf[256];
    int ret;
    
    // 获取版本信息
    moeai_version_info(version_buf, sizeof(version_buf));
    seq_printf(seq, "%s\n\n", version_buf);
    
    // 获取内存状态
    ret = moeai_mem_monitor_get_stats(&stats);
    if (ret) {
        seq_printf(seq, "Error: 无法获取内存状态 (错误码: %d)\n", ret);
        return 0;
    }
    
    // 输出内存状态
    seq_puts(seq, "内存状态:\n");
    seq_printf(seq, "  总物理内存: %lu KB\n", stats.total_ram);
    seq_printf(seq, "  空闲内存: %lu KB\n", stats.free_ram);
    seq_printf(seq, "  可用内存: %lu KB\n", stats.available_ram);
    seq_printf(seq, "  缓存内存: %lu KB\n", stats.cached_ram);
    seq_printf(seq, "  内存使用率: %u%%\n", stats.mem_usage_percent);
    seq_printf(seq, "  交换空间总量: %lu KB\n", stats.swap_total);
    seq_printf(seq, "  空闲交换空间: %lu KB\n", stats.swap_free);
    seq_printf(seq, "  交换空间使用率: %u%%\n", stats.swap_usage_percent);
    
    return 0;
}
```

### 3.3 控制文件实现

```c
// 控制文件的write回调
static ssize_t moeai_procfs_control_write(struct file *file, const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    char buf[MOEAI_MAX_CMD_LEN];
    size_t buf_size = min(count, sizeof(buf) - 1);
    
    // 复制用户数据
    if (copy_from_user(buf, user_buf, buf_size))
        return -EFAULT;
    
    // 确保字符串结束
    buf[buf_size] = '\0';
    
    // 处理命令
    if (strncmp(buf, "reclaim", 7) == 0) {
        // 执行内存回收
        long reclaimed = moeai_mem_reclaim(MOEAI_MEM_RECLAIM_MODERATE);
        pr_info("MoeAI-C: 执行内存回收，释放了 %ld KB\n", reclaimed);
    } 
    else if (strncmp(buf, "set threshold ", 13) == 0) {
        // 设置阈值
        unsigned int threshold;
        if (kstrtouint(buf + 13, 10, &threshold) == 0) {
            struct moeai_mem_monitor_config config;
            moeai_mem_monitor_get_config(&config);
            config.warn_threshold = threshold;
            config.critical_threshold = threshold + 10;
            config.emergency_threshold = threshold + 20;
            moeai_mem_monitor_set_config(&config);
            pr_info("MoeAI-C: 设置内存阈值为 %u%%\n", threshold);
        }
    }
    else {
        pr_warn("MoeAI-C: 未知命令: %s\n", buf);
    }
    
    // 返回处理的数据大小
    return count;
}
```

## 4. 用户态CLI工具实现

### 4.1 基本命令解析

```c
// 解析命令行参数
int parse_args(int argc, char *argv[], struct moeai_cmd *cmd)
{
    if (argc < 2) {
        cmd->type = CMD_HELP;
        return 0;
    }
    
    // 解析主命令
    if (strcmp(argv[1], "status") == 0) {
        cmd->type = CMD_STATUS;
    }
    else if (strcmp(argv[1], "reclaim") == 0) {
        cmd->type = CMD_RECLAIM;
    }
    else if (strcmp(argv[1], "set") == 0) {
        if (argc < 4) {
            fprintf(stderr, "错误: set命令需要更多参数\n");
            return -1;
        }
        
        if (strcmp(argv[2], "threshold") == 0) {
            cmd->type = CMD_SET_THRESHOLD;
            cmd->value = atoi(argv[3]);
        } else {
            fprintf(stderr, "错误: 未知的set子命令: %s\n", argv[2]);
            return -1;
        }
    }
    else if (strcmp(argv[1], "help") == 0) {
        cmd->type = CMD_HELP;
    }
    else {
        fprintf(stderr, "错误: 未知命令: %s\n", argv[1]);
        return -1;
    }
    
    return 0;
}
```

### 4.2 与procfs交互

```c
// 读取状态信息
int read_status(void)
{
    FILE *fp;
    char buffer[4096];
    size_t bytes_read;
    
    // 打开状态文件
    fp = fopen("/proc/moeai/status", "r");
    if (!fp) {
        perror("无法打开状态文件");
        return -1;
    }
    
    // 读取并显示内容
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    
    fclose(fp);
    return 0;
}

// 发送命令
int send_command(const char *cmd)
{
    FILE *fp;
    
    // 打开控制文件
    fp = fopen("/proc/moeai/control", "w");
    if (!fp) {
        perror("无法打开控制文件");
        return -1;
    }
    
    // 写入命令
    fprintf(fp, "%s", cmd);
    
    fclose(fp);
    return 0;
}
```

## 5. 日志系统实现

### 5.1 环形缓冲区记录日志

```c
// 写入日志
void moeai_log(enum moeai_log_level level, const char *module, const char *fmt, ...)
{
    va_list args;
    struct moeai_log_entry entry;
    char message[256];
    int ret;
    
    // 检查日志级别
    if (level < moeai_logger_ctx.config.min_level)
        return;
        
    // 格式化消息
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    
    // 填充日志条目
    entry.timestamp = ktime_get_real_ns();
    entry.level = level;
    strncpy(entry.module, module, sizeof(entry.module) - 1);
    entry.module[sizeof(entry.module) - 1] = '\0';
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';
    
    // 输出到内核日志
    if (moeai_logger_ctx.config.console_output) {
        const char *level_str;
        
        switch (level) {
        case MOEAI_LOG_DEBUG:
            level_str = "DEBUG";
            break;
        case MOEAI_LOG_INFO:
            level_str = "INFO";
            break;
        case MOEAI_LOG_WARN:
            level_str = "WARN";
            break;
        case MOEAI_LOG_ERROR:
            level_str = "ERROR";
            break;
        case MOEAI_LOG_FATAL:
            level_str = "FATAL";
            break;
        default:
            level_str = "UNKNOWN";
            break;
        }
        
        printk("%s: MoeAI-C [%s] %s\n", level_str, module, message);
    }
    
    // 写入环形缓冲区
    if (moeai_logger_ctx.config.buffer_output && moeai_logger_ctx.log_buffer) {
        spin_lock(&moeai_logger_ctx.buffer_lock);
        ret = moeai_ring_buffer_write(moeai_logger_ctx.log_buffer, &entry);
        spin_unlock(&moeai_logger_ctx.buffer_lock);
        
        if (ret)
            printk(KERN_WARNING "MoeAI-C: 无法写入日志缓冲区，错误码: %d\n", ret);
    }
}
```

### 5.2 procfs日志读取接口

```c
// 日志文件的show回调
static int moeai_procfs_log_show(struct seq_file *seq, void *v)
{
    struct moeai_log_entry *entries;
    size_t count, i;
    const char *level_str;
    struct timespec64 ts;
    
    // 分配临时缓冲区
    entries = kmalloc(sizeof(*entries) * MOEAI_LOG_BUFFER_SIZE, GFP_KERNEL);
    if (!entries)
        return -ENOMEM;
    
    // 获取日志条目
    if (moeai_logger_get_recent_logs(entries, MOEAI_LOG_BUFFER_SIZE, &count)) {
        seq_puts(seq, "Error: 无法获取日志条目\n");
        kfree(entries);
        return 0;
    }
    
    // 遍历并输出日志
    for (i = 0; i < count; i++) {
        // 将纳秒时间戳转换为timespec
        ts = ns_to_timespec64(entries[i].timestamp);
        
        // 获取日志级别字符串
        switch (entries[i].level) {
        case MOEAI_LOG_DEBUG:
            level_str = "DEBUG";
            break;
        case MOEAI_LOG_INFO:
            level_str = "INFO";
            break;
        case MOEAI_LOG_WARN:
            level_str = "WARN";
            break;
        case MOEAI_LOG_ERROR:
            level_str = "ERROR";
            break;
        case MOEAI_LOG_FATAL:
            level_str = "FATAL";
            break;
        default:
            level_str = "UNKNOWN";
            break;
        }
        
        // 输出格式化日志条目
        seq_printf(seq, "[%5lld.%06ld] %-5s [%-8s] %s\n",
                  (long long)ts.tv_sec, ts.tv_nsec / 1000,
                  level_str, entries[i].module, entries[i].message);
    }
    
    kfree(entries);
    return 0;
}
```

## 6. Makefile 修改

修改主Makefile以支持MVP构建：

```makefile
# MoeAI-C Makefile (MVP版本)

# 定义基础变量
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
SRCDIR := $(PWD)/src
INCDIR := $(PWD)/include
BUILDDIR := $(PWD)/build
CLIDIR := $(PWD)/cli

# 内核模块构建目标
MODULE_NAME := moeai

# C编译器选项
CC := gcc
CFLAGS := -Wall -Werror -I$(INCDIR)

# 默认目标
all: module cli

# 内核模块目标
module:
	@echo "构建内核模块: $(MODULE_NAME).ko"
	@mkdir -p $(BUILDDIR)
	$(MAKE) -C $(KDIR) M=$(PWD) src=$(SRCDIR) modules
	@cp $(PWD)/*.ko $(BUILDDIR)/ 2>/dev/null || true
	@echo "内核模块构建完成"

# CLI工具目标
cli: $(BUILDDIR)/moectl

$(BUILDDIR)/moectl: $(CLIDIR)/moectl.c $(CLIDIR)/parser.c
	@echo "构建CLI工具: moectl"
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "CLI工具构建完成"

# 清理目标
clean:
	@echo "清理构建文件"
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -rf $(BUILDDIR)/* *.o *.ko *.mod.* *.symvers *.order .*.cmd .tmp_versions
	@echo "清理完成"

# 内核模块相关规则
obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := \
	src/main.o \
	src/core/init.o \
	src/core/version.o \
	src/modules/mem_monitor.o \
	src/ipc/procfs.o \
	src/utils/logger.o \
	src/utils/ring_buffer.o

.PHONY: all module cli clean help
```

## 7. 测试计划与用例

### 7.1 模块加载测试

```bash
# 测试模块加载
insmod build/moeai.ko
lsmod | grep moeai
dmesg | grep "MoeAI-C"

# 测试模块卸载
rmmod moeai
dmesg | grep "MoeAI-C"
```

### 7.2 内存监控测试

```bash
# 检查内存状态
cat /proc/moeai/status

# 测试内存回收
echo "reclaim" > /proc/moeai/control
cat /proc/moeai/status

# 测试阈值设置
echo "set threshold 70" > /proc/moeai/control
cat /proc/moeai/status
```

### 7.3 CLI工具测试

```bash
# 测试状态命令
./build/moectl status

# 测试内存回收命令
./build/moectl reclaim

# 测试阈值设置命令
./build/moectl set threshold 75
```

## 8. 实现时间线

| 阶段 | 时间 | 里程碑 |
|------|------|--------|
| 第1周 | 第1-3天 | 实现基础模块框架和日志系统 |
|      | 第4-7天 | 完成Makefile和基本构建系统 |
| 第2周 | 第1-3天 | 实现内存状态获取功能 |
|      | 第4-7天 | 实现内存回收机制 |
| 第3周 | 第1-3天 | 实现procfs目录和状态文件 |
|      | 第4-7天 | 实现控制文件和日志文件 |
| 第4周 | 第1-3天 | 实现CLI工具基础功能 |
|      | 第4-7天 | 实现CLI与procfs交互 |
| 第5周 | 第1-3天 | 集成测试和问题修复 |
|      | 第4-7天 | 性能优化和文档完善 |

## 9. 总结

本实现指南详细描述了MoeAI-C MVP各核心组件的技术实现方法。按照此指南实现，可以构建一个功能完整的最小可行产品，展示MoeAI-C的基本能力。在完成MVP后，可以根据实际使用体验逐步扩展系统功能，添加更多高级特性。