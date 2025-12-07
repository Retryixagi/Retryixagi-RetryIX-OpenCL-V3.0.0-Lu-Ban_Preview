#ifndef RETRYIX_MEMORY_H
#define RETRYIX_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 記憶體類型定義
typedef enum {
    RETRYIX_MEM_READ_ONLY = 0x1,
    RETRYIX_MEM_WRITE_ONLY = 0x2,
    RETRYIX_MEM_READ_WRITE = 0x4,
    RETRYIX_MEM_HOST_PTR = 0x8,
    RETRYIX_MEM_ALLOC_HOST_PTR = 0x10,
    RETRYIX_MEM_COPY_HOST_PTR = 0x20,
    RETRYIX_MEM_PERSISTENT = 0x40,
    RETRYIX_MEM_ZERO_COPY = 0x80
} retryix_memory_flags_t;

// === 公開 API ===

/**
 * 通用記憶體分配
 * @param size 分配大小
 * @param flags 記憶體標誌
 * @param debug_name 調試名稱
 * @return 分配的記憶體指針，失敗返回 NULL
 */
void* retryix_memory_alloc(size_t size, retryix_memory_flags_t flags, const char* debug_name);

/**
 * 記憶體釋放
 * @param ptr 要釋放的記憶體指針
 * @return 0 成功，-1 失敗
 */
int retryix_memory_free(void* ptr);

/**
 * 記憶體統計報告
 */
void retryix_memory_print_stats(void);

/**
 * 記憶體完整性檢查
 * @return 錯誤數量，0 表示通過
 */
int retryix_memory_validate(void);

/**
 * 清理記憶體管理器
 */
void retryix_memory_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_MEMORY_H
