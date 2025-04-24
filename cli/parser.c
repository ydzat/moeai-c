/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: cli/parser.c
 * 描述: 命令行参数解析工具
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* 错误码 */
#define PARSER_OK           0
#define PARSER_ERROR       -1
#define PARSER_INVALID_ARG -2

/* 最大参数数量 */
#define MAX_ARGS 16

/* 全局参数表 */
static char *parsed_args[MAX_ARGS];
static int arg_count = 0;

/**
 * 解析命令行字符串
 * @param command 命令字符串
 * @return 解析后的参数个数，发生错误则返回负值
 */
int parse_command_line(const char *command) {
    char *buf, *token, *saveptr;
    int count = 0;
    
    /* 输入检查 */
    if (!command)
        return PARSER_INVALID_ARG;
    
    /* 重置参数计数 */
    arg_count = 0;
    
    /* 复制命令行，因为strtok会修改字符串 */
    buf = strdup(command);
    if (!buf)
        return PARSER_ERROR;
    
    /* 解析第一个参数(命令名) */
    token = strtok_r(buf, " \t\n", &saveptr);
    if (!token) {
        free(buf);
        return 0;  /* 空命令 */
    }
    
    /* 保存命令名 */
    parsed_args[count++] = strdup(token);
    
    /* 解析剩余参数 */
    while ((token = strtok_r(NULL, " \t\n", &saveptr)) != NULL && count < MAX_ARGS) {
        parsed_args[count++] = strdup(token);
    }
    
    /* 更新参数计数 */
    arg_count = count;
    
    /* 释放临时缓冲区 */
    free(buf);
    
    return count;
}

/**
 * 获取解析后的参数数量
 * @return 参数数量
 */
int get_arg_count(void) {
    return arg_count;
}

/**
 * 获取指定位置的参数
 * @param index 参数索引(从0开始)
 * @return 参数字符串，索引无效则返回NULL
 */
const char *get_arg(int index) {
    if (index < 0 || index >= arg_count)
        return NULL;
    
    return parsed_args[index];
}

/**
 * 释放解析后的参数
 */
void free_parsed_args(void) {
    int i;
    
    for (i = 0; i < arg_count; i++) {
        if (parsed_args[i]) {
            free(parsed_args[i]);
            parsed_args[i] = NULL;
        }
    }
    
    arg_count = 0;
}

/**
 * 检查字符串是否仅包含数字
 * @param str 要检查的字符串
 * @return 如果仅包含数字返回1，否则返回0
 */
int is_numeric(const char *str) {
    if (!str || *str == '\0')
        return 0;
    
    while (*str) {
        if (!isdigit(*str))
            return 0;
        str++;
    }
    
    return 1;
}

/**
 * 解析整数参数
 * @param arg 参数字符串
 * @param value 用于存储解析结果的指针
 * @return 成功返回PARSER_OK，失败返回错误码
 */
int parse_int_arg(const char *arg, int *value) {
    char *endptr;
    
    if (!arg || !value)
        return PARSER_INVALID_ARG;
    
    *value = (int)strtol(arg, &endptr, 10);
    
    if (*endptr != '\0')
        return PARSER_ERROR;  /* 无效的整数 */
    
    return PARSER_OK;
}