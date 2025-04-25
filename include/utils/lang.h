/*
 * @Author: @ydzat
 * @Date: 2025-04-25 18:18:43
 * @LastEditors: @ydzat
 * @LastEditTime: 2025-04-25 18:48:23
 * @Description: 
 */
#ifndef MOEAI_LANG_H
#define MOEAI_LANG_H

#include <linux/string.h>
#include "string_ids.h"

#define DEFAULT_LANG "en"

// Language identifiers
#define LANG_EN 0
#define LANG_ZH 1

// Current language (default to English)
extern int current_lang;

/**
 * Initialize language system
 * @param lang_code Language code from config.mk (en/zh)
 * @return 0 on success, -1 if language not supported
 */
int lang_init(const char *lang_code);

/**
 * Get language string by ID
 * @param string_id The string identifier (e.g. LANG_HELP_HEADER)
 * @return The localized string or NULL if not found
 */
const char *lang_get(int string_id);

/**
 * Get formatted language string
 * @param string_id The string identifier
 * @param ... Format arguments
 * @return Formatted string (caller must kfree)
 */
char *lang_getf(int string_id, ...);

#endif // MOEAI_LANG_H
