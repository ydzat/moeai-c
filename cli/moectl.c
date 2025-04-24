/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: cli/moectl.c
 * 描述: 命令行控制工具
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* 命令类型枚举 */
typedef enum {
    CMD_HELP,
    CMD_STATUS,
    CMD_RECLAIM,
    CMD_SET_THRESHOLD,
    CMD_SET_INTERVAL,
    CMD_SET_AUTORECLAIM,
} moeai_cmd_type;

/* 命令结构体 */
struct moeai_cmd {
    moeai_cmd_type type;
    int value;
    const char *str_value;
};

/* procfs 路径 */
#define MOEAI_PROCFS_STATUS  "/proc/moeai/status"
#define MOEAI_PROCFS_CONTROL "/proc/moeai/control"
#define MOEAI_PROCFS_LOG     "/proc/moeai/log"

/**
 * 显示帮助信息
 */
static void show_help(void)
{
    printf("MoeAI-C 命令行控制工具\n");
    printf("用法: moectl <命令> [<参数>]\n\n");
    printf("可用命令:\n");
    printf("  status            显示当前系统状态\n");
    printf("  reclaim           触发内存回收\n");
    printf("  set threshold N   设置内存监控阈值为N%%\n");
    printf("  set interval N    设置检查间隔为N毫秒\n");
    printf("  set autoreclaim on|off  设置自动回收开关\n");
    printf("  help              显示此帮助信息\n");
}

/**
 * 解析命令行参数
 * @argc: 参数数量
 * @argv: 参数数组
 * @cmd: 存储解析结果的命令结构体
 * @return: 成功返回0，失败返回负值
 */
static int parse_args(int argc, char *argv[], struct moeai_cmd *cmd)
{
    if (argc < 2) {
        cmd->type = CMD_HELP;
        return 0;
    }
    
    /* 解析主命令 */
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
        } 
        else if (strcmp(argv[2], "interval") == 0) {
            cmd->type = CMD_SET_INTERVAL;
            cmd->value = atoi(argv[3]);
        }
        else if (strcmp(argv[2], "autoreclaim") == 0) {
            cmd->type = CMD_SET_AUTORECLAIM;
            cmd->str_value = argv[3];
        }
        else {
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

/**
 * 读取状态信息
 * @return: 成功返回0，失败返回负值
 */
static int read_status(void)
{
    FILE *fp;
    char buffer[4096];
    size_t bytes_read;
    
    /* 打开状态文件 */
    fp = fopen(MOEAI_PROCFS_STATUS, "r");
    if (!fp) {
        perror("无法打开状态文件");
        return -1;
    }
    
    /* 读取并显示内容 */
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    
    fclose(fp);
    return 0;
}

/**
 * 读取日志信息
 * @return: 成功返回0，失败返回负值
 */
static int read_log(void)
{
    FILE *fp;
    char buffer[4096];
    size_t bytes_read;
    
    /* 打开日志文件 */
    fp = fopen(MOEAI_PROCFS_LOG, "r");
    if (!fp) {
        perror("无法打开日志文件");
        return -1;
    }
    
    /* 读取并显示内容 */
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    
    fclose(fp);
    return 0;
}

/**
 * 发送命令
 * @cmd: 要发送的命令字符串
 * @return: 成功返回0，失败返回负值
 */
static int send_command(const char *cmd)
{
    FILE *fp;
    
    /* 打开控制文件 */
    fp = fopen(MOEAI_PROCFS_CONTROL, "w");
    if (!fp) {
        perror("无法打开控制文件");
        return -1;
    }
    
    /* 写入命令 */
    fprintf(fp, "%s", cmd);
    
    fclose(fp);
    return 0;
}

/**
 * 主函数
 */
int main(int argc, char *argv[])
{
    struct moeai_cmd cmd = {0};
    char cmd_buf[256];
    int ret;
    
    /* 解析命令行参数 */
    ret = parse_args(argc, argv, &cmd);
    if (ret) {
        show_help();
        return EXIT_FAILURE;
    }
    
    /* 执行命令 */
    switch (cmd.type) {
    case CMD_HELP:
        show_help();
        break;
        
    case CMD_STATUS:
        return (read_status() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        
    case CMD_RECLAIM:
        printf("执行内存回收...\n");
        if (send_command("reclaim") != 0)
            return EXIT_FAILURE;
        printf("内存回收完成，查看状态:\n");
        return (read_status() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        
    case CMD_SET_THRESHOLD:
        printf("设置内存监控阈值为 %d%%...\n", cmd.value);
        snprintf(cmd_buf, sizeof(cmd_buf), "set threshold %d", cmd.value);
        return (send_command(cmd_buf) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        
    case CMD_SET_INTERVAL:
        printf("设置检查间隔为 %d ms...\n", cmd.value);
        snprintf(cmd_buf, sizeof(cmd_buf), "set interval %d", cmd.value);
        return (send_command(cmd_buf) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        
    case CMD_SET_AUTORECLAIM:
        printf("设置自动回收为 %s...\n", cmd.str_value);
        snprintf(cmd_buf, sizeof(cmd_buf), "set autoreclaim %s", cmd.str_value);
        return (send_command(cmd_buf) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}