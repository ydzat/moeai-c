/**
 * MoeAI-C - Intelligent Kernel Assistant Module
 * 
 * File: cli/moectl.c
 * Description: Command Line Control Tool
 * 
 * Copyright © 2025 @ydzat
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include "../include/utils/lang.h"
#include "../include/utils/string_ids.h"

/* Initialize language system */
static void init_language() {
    const char *lang_env = getenv("MOEAI_LANG");
    if (!lang_env) {
        lang_env = "en"; // Default to English
    }
    lang_init(lang_env);
}

/* 命令类型枚举 */
typedef enum {
    CMD_HELP,
    CMD_STATUS,
    CMD_RECLAIM,
    CMD_SET_THRESHOLD,
    CMD_SET_INTERVAL,
    CMD_SET_AUTORECLAIM,
    CMD_SELFTEST,     /* 新增: 自检命令 */
    CMD_LOG           /* 新增: 日志查看命令 */
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
#define MOEAI_PROCFS_SELFTEST "/proc/moeai/selftest"  /* 新增: 自检接口 */

/**
 * Show help information
 */
static void show_help(void)
{
    printf("%s\n", lang_get(LANG_CLI_HELP_HEADER));
    printf("%s\n", lang_get(LANG_CLI_HELP_USAGE));
    printf("\n%s\n", lang_get(LANG_CLI_HELP_AVAIL_CMDS));
    printf("%s\n", lang_get(LANG_CLI_CMD_STATUS));
    printf("%s\n", lang_get(LANG_CLI_CMD_RECLAIM));
    printf("%s\n", lang_get(LANG_CLI_CMD_SET_THRESHOLD));
    printf("%s\n", lang_get(LANG_CLI_CMD_SET_INTERVAL));
    printf("%s\n", lang_get(LANG_CLI_CMD_SET_AUTORECLAIM));
    printf("%s\n", lang_get(LANG_CLI_CMD_SELFTEST));
    printf("%s\n", lang_get(LANG_CLI_CMD_LOG));
    printf("%s\n", lang_get(LANG_CLI_CMD_HELP));
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
            fprintf(stderr, "%s\n", lang_get(LANG_CLI_ERR_INSUFFICIENT_ARGS));
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
            char *msg = lang_getf(LANG_CLI_ERR_UNKNOWN_SET_CMD, argv[2]);
            if (msg) {
                fprintf(stderr, "%s", msg);
                free(msg);
            }
            return -1;
        }
    }
    else if (strcmp(argv[1], "help") == 0) {
        cmd->type = CMD_HELP;
    }
    else if (strcmp(argv[1], "selftest") == 0) {
        cmd->type = CMD_SELFTEST;
    }
    else if (strcmp(argv[1], "log") == 0) {
        cmd->type = CMD_LOG;
    }
    else {
        char *msg = lang_getf(LANG_CLI_ERR_UNKNOWN_CMD, argv[1]);
        if (msg) {
            fprintf(stderr, "%s", msg);
            free(msg);
        }
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
    
    /* Open status file */
    fp = fopen(MOEAI_PROCFS_STATUS, "r");
    if (!fp) {
        fprintf(stderr, "Error: %s (%s)\n", lang_get(LANG_CLI_ERR_OPEN_STATUS), strerror(errno));
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
        perror(lang_get(LANG_CLI_ERR_OPEN_LOG));
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
        perror(lang_get(LANG_CLI_ERR_OPEN_CONTROL));
        return -1;
    }
    
    /* 写入命令 */
    fprintf(fp, "%s", cmd);
    
    fclose(fp);
    return 0;
}

/**
 * 读取自检结果信息
 * @return: 成功返回0，失败返回负值
 */
static int read_selftest(void)
{
    FILE *fp;
    char buffer[8192];  /* 使用更大的缓冲区，自检结果可能较长 */
    size_t bytes_read;
    
    /* 先触发自检程序 */
    if (send_command("selftest") != 0) {
        fprintf(stderr, "%s\n", lang_get(LANG_CLI_ERR_TRIGGER_SELFTEST));
        return -1;
    }
    
    /* 等待一小段时间，确保自检完成 */
    usleep(500000);  /* 等待500毫秒 */
    
    /* 打开自检结果文件 */
    fp = fopen(MOEAI_PROCFS_SELFTEST, "r");
    if (!fp) {
        perror(lang_get(LANG_CLI_ERR_OPEN_SELFTEST));
        return -1;
    }
    
    /* 读取并显示内容 */
    printf("%s\n", lang_get(LANG_CLI_MSG_SELFTEST_RESULT));
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    printf("================================\n");
    
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
    
    /* 初始化语言设置 */
    init_language();
    
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
        printf("%s\n", lang_get(LANG_CLI_MSG_MEM_RECLAIM));
        if (send_command("reclaim") != 0)
            return EXIT_FAILURE;
        printf("%s\n", lang_get(LANG_CLI_MSG_RECLAIM_COMPLETE));
        return (read_status() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        
    case CMD_SET_THRESHOLD: {
        char *msg = lang_getf(LANG_CLI_MSG_SET_THRESHOLD, cmd.value);
        if (msg) {
            printf("%s\n", msg);
            free(msg);
        }
        snprintf(cmd_buf, sizeof(cmd_buf), "set threshold %d", cmd.value);
        return (send_command(cmd_buf) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
        
    case CMD_SET_INTERVAL: {
        char *msg = lang_getf(LANG_CLI_MSG_SET_INTERVAL, cmd.value);
        if (msg) {
            printf("%s\n", msg);
            free(msg);
        }
        snprintf(cmd_buf, sizeof(cmd_buf), "set interval %d", cmd.value);
        return (send_command(cmd_buf) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
        
    case CMD_SET_AUTORECLAIM: {
        char *msg = lang_getf(LANG_CLI_MSG_SET_AUTORECLAIM, cmd.str_value);
        if (msg) {
            printf("%s\n", msg);
            free(msg);
        }
        snprintf(cmd_buf, sizeof(cmd_buf), "set autoreclaim %s", cmd.str_value);
        return (send_command(cmd_buf) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
        
    case CMD_SELFTEST:
        return (read_selftest() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;  /* 新增: 执行自检 */
        
    case CMD_LOG:
        return (read_log() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
