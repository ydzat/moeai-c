/*
 * @Author: @ydzat
 * @Date: 2025-04-25 18:29:34
 * @LastEditors: @ydzat
 * @LastEditTime: 2025-04-25 19:36:04
 * @Description: Dual-mode (kernel/user) language support
 */
#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/printk.h>
#else
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#endif

#include "../../include/utils/lang.h"

// Include language string arrays
#include "../../lang/en/program_strings.h"
#include "../../lang/zh/program_strings.h"

int current_lang = LANG_EN;

// English strings lookup
static const char *get_en_string(int string_id) {
    if (string_id >= 0 && string_id < STRING_ID_COUNT) {
        return en_strings[string_id];
    }
    return NULL;
}

// Chinese strings lookup
static const char *get_zh_string(int string_id) {
    if (string_id >= 0 && string_id < STRING_ID_COUNT) {
        return zh_strings[string_id];
    }
    return NULL;
}

int lang_init(const char *lang_code) {
    if (!strcmp(lang_code, "en")) {
        current_lang = LANG_EN;
        return 0;
    } else if (!strcmp(lang_code, "zh")) {
        current_lang = LANG_ZH;
        return 0;
    }
    return -1;
}

const char *lang_get(int string_id) {
    const char *str = NULL;
    
    if (current_lang == LANG_ZH) {
        str = get_zh_string(string_id);
    }
    
    // Fallback to English if not found or not Chinese
    if (!str) {
        str = get_en_string(string_id);
    }
    
    return str;
}

char *lang_getf(int string_id, ...) {
    const char *fmt = lang_get(string_id);
    if (!fmt) return NULL;
    
    va_list args;
    va_start(args, string_id);
    
    // Calculate required size
    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);
    
#ifdef __KERNEL__
    char *buf = kmalloc(size, GFP_KERNEL);
#else
    char *buf = malloc(size);
#endif
    if (!buf) return NULL;
    
    va_start(args, string_id);
    vsnprintf(buf, size, fmt, args);
    va_end(args);
    
    return buf;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(lang_init);
EXPORT_SYMBOL(lang_get);
EXPORT_SYMBOL(lang_getf);
#endif
