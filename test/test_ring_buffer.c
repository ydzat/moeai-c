#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "../include/utils/ring_buffer.h"
#include "../include/utils/lang.h"

/* 测试环境初始化函数 */
static int __init test_ring_buffer_init(void)
{
    struct moeai_ring_buffer *rb;
    int ret, i, data;
    bool is_empty, is_full;
    size_t size, capacity;
    
    pr_info("%s\n", get_string(LANG_TEST_RB_START));
    
    /* 测试1: 创建环形缓冲区 */
    rb = moeai_ring_buffer_create(10, sizeof(int));
    if (!rb) {
        pr_err("%s\n", get_string(LANG_TEST_RB_CREATE_FAIL));
        return -ENOMEM;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_CREATE_PASS));
    
    /* 测试2: 初始状态检查 */
    is_empty = moeai_ring_buffer_is_empty(rb);
    is_full = moeai_ring_buffer_is_full(rb);
    size = moeai_ring_buffer_size(rb);
    capacity = moeai_ring_buffer_capacity(rb);
    
    if (!is_empty || is_full || size != 0 || capacity != 10) {
        pr_err("%s\n", get_string(LANG_TEST_RB_INIT_STATE_FAIL),
               is_empty, is_full, size, capacity);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_INIT_STATE_PASS));
    
    /* 测试3: 写入数据 */
    for (i = 0; i < 5; i++) {
        data = i + 1;
        ret = moeai_ring_buffer_write(rb, &data);
        if (ret != 0) {
            pr_err("%s\n", get_string(LANG_TEST_RB_WRITE_FAIL), ret);
            moeai_ring_buffer_destroy(rb);
            return ret;
        }
    }
    
    size = moeai_ring_buffer_size(rb);
    if (size != 5) {
        pr_err("%s\n", get_string(LANG_TEST_RB_WRITE_PASS), 5, size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_WRITE_PASS), 5);
    
    /* 测试4: 读取数据 */
    for (i = 0; i < 3; i++) {
        ret = moeai_ring_buffer_read(rb, &data);
        if (ret != 0 || data != i + 1) {
            pr_err("%s\n", get_string(LANG_TEST_RB_READ_FAIL),
                   i + 1, data, ret);
            moeai_ring_buffer_destroy(rb);
            return -EINVAL;
        }
    }
    
    size = moeai_ring_buffer_size(rb);
    if (size != 2) {
        pr_err("%s\n", get_string(LANG_TEST_RB_READ_PASS), 3, size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_READ_PASS), 3);
    
    /* 测试5: 写入更多数据直到缓冲区满 */
    for (i = 0; i < 8; i++) {
        data = 100 + i;
        ret = moeai_ring_buffer_write(rb, &data);
        if (ret != 0) {
            pr_err("%s\n", get_string(LANG_TEST_RB_FILL_FAIL), ret);
            moeai_ring_buffer_destroy(rb);
            return ret;
        }
    }
    
    is_full = moeai_ring_buffer_is_full(rb);
    size = moeai_ring_buffer_size(rb);
    if (!is_full || size != 10) {
        pr_err("%s\n", get_string(LANG_TEST_RB_FILL_PASS), is_full, size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_FILL_PASS));
    
    /* 测试6: 写入溢出（覆盖旧数据） */
    for (i = 0; i < 3; i++) {
        data = 200 + i;
        ret = moeai_ring_buffer_write(rb, &data);
        if (ret != 0) {
            pr_err("%s\n", get_string(LANG_TEST_RB_OVERFLOW_FAIL), ret);
            moeai_ring_buffer_destroy(rb);
            return ret;
        }
    }
    
    /* 测试7: 验证溢出后的数据 */
    ret = moeai_ring_buffer_read(rb, &data);
    if (ret != 0 || data != 103) {
        pr_err("%s\n", get_string(LANG_TEST_RB_OVERFLOW_PASS), data);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_OVERFLOW_PASS));
    
    /* 测试8: 清空缓冲区 */
    moeai_ring_buffer_clear(rb);
    is_empty = moeai_ring_buffer_is_empty(rb);
    size = moeai_ring_buffer_size(rb);
    if (!is_empty || size != 0) {
        pr_err("%s\n", get_string(LANG_TEST_RB_CLEAR_FAIL),
               is_empty, size);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_CLEAR_PASS));
    
    /* 测试9: 空缓冲区读取 */
    ret = moeai_ring_buffer_read(rb, &data);
    if (ret != -ENODATA) {
        pr_err("%s\n", get_string(LANG_TEST_RB_EMPTY_READ_FAIL), ret);
        moeai_ring_buffer_destroy(rb);
        return -EINVAL;
    }
    pr_info("%s\n", get_string(LANG_TEST_RB_EMPTY_READ_PASS));
    
    /* 清理资源 */
    moeai_ring_buffer_destroy(rb);
    pr_info("%s\n", get_string(LANG_TEST_RB_ALL_PASS));
    
    return 0;
}

/* 测试环境清理函数 */
static void __exit test_ring_buffer_exit(void)
{
    pr_info("%s\n", get_string(LANG_TEST_RB_CLEANUP));
}

module_init(test_ring_buffer_init);
module_exit(test_ring_buffer_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("@ydzat");
MODULE_DESCRIPTION("MoeAI-C 环形缓冲区测试");
MODULE_VERSION("0.1");
