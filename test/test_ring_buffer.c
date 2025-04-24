/**
 * MoeAI-C - 智能内核助手模块
 * 
 * 文件: test/test_ring_buffer.c
 * 描述: 环形缓冲区单元测试
 * 
 * 版权所有 © 2025 @ydzat
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "utils/ring_buffer.h"

/* 测试环境初始化函数 */
static int __init test_ring_buffer_init(void)
{
    struct moeai_ring_buffer *rb;
    int ret, i, data;
    bool is_empty, is_full;
    size_t size, capacity;
    
    pr_info("MoeAI-C: 开始环形缓冲区测试\n");
    
    /* 测试1: 创建环形缓冲区 */
    rb = moeai_ring_buffer_create(10, sizeof(int));
    if (!rb) {
        pr_err("测试失败: 无法创建环形缓冲区\n");
        return -ENOMEM;
    }
    pr_info("测试通过: 环形缓冲区创建成功\n");
    
    /* 测试2: 初始状态检查 */
    is_empty = moeai_ring_buffer_is_empty(rb);
    is_full = moeai_ring_buffer_is_full(rb);
    size = moeai_ring_buffer_size(rb);
    capacity = moeai_ring_buffer_capacity(rb);
    
    if (!is_empty || is_full || size != 0 || capacity != 10) {
        pr_err("测试失败: 初始状态错误 (empty=%d, full=%d, size=%zu, capacity=%zu)\n",
               is_empty, is_full, size, capacity);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("测试通过: 初始状态检查正确\n");
    
    /* 测试3: 写入数据 */
    for (i = 0; i < 5; i++) {
        data = i + 1;
        ret = moeai_ring_buffer_write(rb, &data);
        if (ret != 0) {
            pr_err("测试失败: 写入数据失败，错误码: %d\n", ret);
            moeai_ring_buffer_destroy(rb);
            return ret;
        }
    }
    
    size = moeai_ring_buffer_size(rb);
    if (size != 5) {
        pr_err("测试失败: 写入后大小错误，期望5，实际%zu\n", size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("测试通过: 成功写入5个元素\n");
    
    /* 测试4: 读取数据 */
    for (i = 0; i < 3; i++) {
        ret = moeai_ring_buffer_read(rb, &data);
        if (ret != 0 || data != i + 1) {
            pr_err("测试失败: 读取数据错误，期望%d，实际%d，错误码: %d\n", 
                   i + 1, data, ret);
            moeai_ring_buffer_destroy(rb);
            return -EINVAL;
        }
    }
    
    size = moeai_ring_buffer_size(rb);
    if (size != 2) {
        pr_err("测试失败: 读取后大小错误，期望2，实际%zu\n", size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("测试通过: 成功读取3个元素\n");
    
    /* 测试5: 写入更多数据直到缓冲区满 */
    for (i = 0; i < 8; i++) {
        data = 100 + i;
        ret = moeai_ring_buffer_write(rb, &data);
        if (ret != 0) {
            pr_err("测试失败: 填充缓冲区失败，错误码: %d\n", ret);
            moeai_ring_buffer_destroy(rb);
            return ret;
        }
    }
    
    is_full = moeai_ring_buffer_is_full(rb);
    size = moeai_ring_buffer_size(rb);
    if (!is_full || size != 10) {
        pr_err("测试失败: 缓冲区应该已满 (full=%d, size=%zu)\n", is_full, size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("测试通过: 缓冲区成功填满\n");
    
    /* 测试6: 写入溢出（覆盖旧数据） */
    for (i = 0; i < 3; i++) {
        data = 200 + i;
        ret = moeai_ring_buffer_write(rb, &data);
        if (ret != 0) {
            pr_err("测试失败: 溢出写入失败，错误码: %d\n", ret);
            moeai_ring_buffer_destroy(rb);
            return ret;
        }
    }
    
    /* 测试7: 验证溢出后的数据 */
    ret = moeai_ring_buffer_read(rb, &data);
    if (ret != 0 || data != 103) {
        pr_err("测试失败: 溢出后读取错误，期望103，实际%d\n", data);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("测试通过: 溢出处理正确\n");
    
    /* 测试8: 清空缓冲区 */
    moeai_ring_buffer_clear(rb);
    is_empty = moeai_ring_buffer_is_empty(rb);
    size = moeai_ring_buffer_size(rb);
    if (!is_empty || size != 0) {
        pr_err("测试失败: 清空后缓冲区不为空 (empty=%d, size=%zu)\n", 
               is_empty, size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("测试通过: 缓冲区成功清空\n");
    
    /* 测试9: 空缓冲区读取 */
    ret = moeai_ring_buffer_read(rb, &data);
    if (ret != -ENODATA) {
        pr_err("测试失败: 从空缓冲区读取应该返回-ENODATA，实际%d\n", ret);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("测试通过: 空缓冲区读取正确处理\n");
    
    /* 清理资源 */
    moeai_ring_buffer_destroy(rb);
    pr_info("MoeAI-C: 环形缓冲区测试全部通过!\n");
    
    return 0;
}

/* 测试环境清理函数 */
static void __exit test_ring_buffer_exit(void)
{
    pr_info("MoeAI-C: 环形缓冲区测试清理完成\n");
}

module_init(test_ring_buffer_init);
module_exit(test_ring_buffer_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C 环形缓冲区测试");
MODULE_VERSION("0.1");