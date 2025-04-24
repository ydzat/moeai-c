/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: include/utils/ring_buffer.h
 * 描述: 环形缓冲区实现，用于日志系统和数据收集
 * 
 * 版权所有 © 2025 @ydzat
 */

#ifndef _MOEAI_RING_BUFFER_H
#define _MOEAI_RING_BUFFER_H

#include <linux/types.h>

struct moeai_ring_buffer;

/* 环形缓冲区API */
struct moeai_ring_buffer *moeai_ring_buffer_create(size_t capacity, size_t item_size);
void moeai_ring_buffer_destroy(struct moeai_ring_buffer *rb);
int moeai_ring_buffer_write(struct moeai_ring_buffer *rb, const void *item);
int moeai_ring_buffer_read(struct moeai_ring_buffer *rb, void *item);
int moeai_ring_buffer_read_batch(struct moeai_ring_buffer *rb, void *items, size_t max_items, size_t *actual_items);
void moeai_ring_buffer_clear(struct moeai_ring_buffer *rb);
size_t moeai_ring_buffer_count(struct moeai_ring_buffer *rb);
bool moeai_ring_buffer_is_empty(struct moeai_ring_buffer *rb);
bool moeai_ring_buffer_is_full(struct moeai_ring_buffer *rb);

#endif /* _MOEAI_RING_BUFFER_H */