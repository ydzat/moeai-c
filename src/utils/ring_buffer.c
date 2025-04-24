/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: src/utils/ring_buffer.c
 * 描述: 环形缓冲区实现
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include "../../include/utils/ring_buffer.h"

/* 环形缓冲区结构定义 */
struct moeai_ring_buffer {
    void *buffer;           /* 实际存储的缓冲区 */
    size_t capacity;        /* 缓冲区容量(项数) */
    size_t item_size;       /* 每个项的大小(字节) */
    size_t head;            /* 头部索引 */
    size_t tail;            /* 尾部索引 */
    size_t count;           /* 当前项数 */
    spinlock_t lock;        /* 自旋锁保护 */
};

/**
 * 创建新的环形缓冲区
 * @capacity: 缓冲区可以容纳的项数
 * @item_size: 每项的字节大小
 * 返回值: 初始化的环形缓冲区或NULL(如果失败)
 */
struct moeai_ring_buffer *moeai_ring_buffer_create(size_t capacity, size_t item_size)
{
    struct moeai_ring_buffer *rb;
    
    if (capacity == 0 || item_size == 0)
        return NULL;
    
    /* 分配环形缓冲区结构 */
    rb = kmalloc(sizeof(struct moeai_ring_buffer), GFP_KERNEL);
    if (!rb)
        return NULL;
    
    /* 分配实际数据缓冲区 */
    rb->buffer = kzalloc(capacity * item_size, GFP_KERNEL);
    if (!rb->buffer) {
        kfree(rb);
        return NULL;
    }
    
    rb->capacity = capacity;
    rb->item_size = item_size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    spin_lock_init(&rb->lock);
    
    return rb;
}

/**
 * 销毁环形缓冲区并释放内存
 * @rb: 要销毁的环形缓冲区
 */
void moeai_ring_buffer_destroy(struct moeai_ring_buffer *rb)
{
    if (!rb)
        return;
    
    if (rb->buffer)
        kfree(rb->buffer);
    
    kfree(rb);
}

/**
 * 向环形缓冲区写入一项
 * @rb: 环形缓冲区
 * @item: 要写入的项
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_ring_buffer_write(struct moeai_ring_buffer *rb, const void *item)
{
    unsigned long flags;
    void *dest;
    
    if (!rb || !item)
        return -EINVAL;
    
    spin_lock_irqsave(&rb->lock, flags);
    
    /* 如果缓冲区已满，覆盖最旧的数据 */
    if (rb->count == rb->capacity) {
        /* 计算目的地址 */
        dest = rb->buffer + (rb->head * rb->item_size);
        /* 复制数据 */
        memcpy(dest, item, rb->item_size);
        /* 移动头部指针 */
        rb->head = (rb->head + 1) % rb->capacity;
        rb->tail = (rb->tail + 1) % rb->capacity;
    } else {
        /* 计算目的地址 */
        dest = rb->buffer + (rb->tail * rb->item_size);
        /* 复制数据 */
        memcpy(dest, item, rb->item_size);
        /* 移动尾部指针并增加计数 */
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->count++;
    }
    
    spin_unlock_irqrestore(&rb->lock, flags);
    return 0;
}

/**
 * 从环形缓冲区读取一项
 * @rb: 环形缓冲区
 * @item: 存储读取项的缓冲区
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_ring_buffer_read(struct moeai_ring_buffer *rb, void *item)
{
    unsigned long flags;
    void *src;
    
    if (!rb || !item)
        return -EINVAL;
    
    spin_lock_irqsave(&rb->lock, flags);
    
    if (rb->count == 0) {
        spin_unlock_irqrestore(&rb->lock, flags);
        return -ENODATA;
    }
    
    /* 计算源地址 */
    src = rb->buffer + (rb->head * rb->item_size);
    /* 复制数据 */
    memcpy(item, src, rb->item_size);
    /* 移动头部指针并减少计数 */
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;
    
    spin_unlock_irqrestore(&rb->lock, flags);
    return 0;
}

/**
 * 从环形缓冲区批量读取多项
 * @rb: 环形缓冲区
 * @items: 存储读取项的缓冲区数组
 * @max_items: 最大要读取的项数
 * @actual_items: 实际读取的项数
 * 返回值: 0表示成功，负值表示错误
 */
int moeai_ring_buffer_read_batch(struct moeai_ring_buffer *rb, void *items, 
                                size_t max_items, size_t *actual_items)
{
    unsigned long flags;
    size_t i, available;
    void *dest, *src;
    
    if (!rb || !items || !actual_items)
        return -EINVAL;
    
    spin_lock_irqsave(&rb->lock, flags);
    
    /* 确定可读取的项数 */
    available = (max_items < rb->count) ? max_items : rb->count;
    *actual_items = available;
    
    /* 批量读取数据 */
    for (i = 0; i < available; i++) {
        src = rb->buffer + (((rb->head + i) % rb->capacity) * rb->item_size);
        dest = (char *)items + (i * rb->item_size);
        memcpy(dest, src, rb->item_size);
    }
    
    /* 更新头部指针和计数 */
    rb->head = (rb->head + available) % rb->capacity;
    rb->count -= available;
    
    spin_unlock_irqrestore(&rb->lock, flags);
    return 0;
}

/**
 * 清空环形缓冲区
 * @rb: 环形缓冲区
 */
void moeai_ring_buffer_clear(struct moeai_ring_buffer *rb)
{
    unsigned long flags;
    
    if (!rb)
        return;
    
    spin_lock_irqsave(&rb->lock, flags);
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    spin_unlock_irqrestore(&rb->lock, flags);
}

/**
 * 获取环形缓冲区当前项数
 * @rb: 环形缓冲区
 * 返回值: 当前项数
 */
size_t moeai_ring_buffer_count(struct moeai_ring_buffer *rb)
{
    unsigned long flags;
    size_t count;
    
    if (!rb)
        return 0;
    
    spin_lock_irqsave(&rb->lock, flags);
    count = rb->count;
    spin_unlock_irqrestore(&rb->lock, flags);
    
    return count;
}

/**
 * 检查环形缓冲区是否为空
 * @rb: 环形缓冲区
 * 返回值: true表示为空，false表示非空
 */
bool moeai_ring_buffer_is_empty(struct moeai_ring_buffer *rb)
{
    return rb ? (moeai_ring_buffer_count(rb) == 0) : true;
}

/**
 * 检查环形缓冲区是否已满
 * @rb: 环形缓冲区
 * 返回值: true表示已满，false表示未满
 */
bool moeai_ring_buffer_is_full(struct moeai_ring_buffer *rb)
{
    unsigned long flags;
    bool is_full;
    
    if (!rb)
        return false;
    
    spin_lock_irqsave(&rb->lock, flags);
    is_full = (rb->count == rb->capacity);
    spin_unlock_irqrestore(&rb->lock, flags);
    
    return is_full;
}