#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// retryix_svm_types.h
// Contains SVM-specific plain types and POD structs. This header deliberately
// does NOT define core error codes or the global `retryix_result_t` type
// (those live in `retryix_core.h`). Keep these types minimal so they can be
// included from many translation units without causing symbol/type clashes.

// SVM 設定
typedef struct {
    bool enable_statistics;
    bool enable_thread_safety;
    bool enable_debugging;
    bool enable_memory_pool;
} retryix_svm_config_t;

// SVM 等級
typedef enum {
    RETRYIX_SVM_LEVEL_NONE = 0,
    RETRYIX_SVM_LEVEL_COARSE_GRAIN = 1,
    RETRYIX_SVM_LEVEL_FINE_GRAIN = 2,
    RETRYIX_SVM_LEVEL_FINE_GRAIN_SYSTEM = 3,
    RETRYIX_SVM_LEVEL_EMULATED = 10
} retryix_svm_level_t;

// SVM 標誌
typedef enum {
    RETRYIX_SVM_FLAG_READ_WRITE = 0x01,
    RETRYIX_SVM_FLAG_READ_ONLY = 0x02,
    RETRYIX_SVM_FLAG_WRITE_ONLY = 0x04,
    RETRYIX_SVM_FLAG_ATOMIC = 0x08,
    RETRYIX_SVM_FLAG_FINE_GRAIN = 0x10,
    RETRYIX_SVM_FLAG_COARSE_GRAIN = 0x20,
    RETRYIX_SVM_FLAG_HOST_PTR = 0x40,
    RETRYIX_SVM_FLAG_ZERO_COPY = 0x80
} retryix_svm_flags_t;

// SVM 分配資訊 (POD)
typedef struct {
    void* ptr;
    size_t size;
    size_t actual_size;
    retryix_svm_flags_t flags;
    retryix_svm_level_t level;
    bool is_mapped;
    uint64_t allocation_id;
} retryix_svm_allocation_t;

// SVM 統計資訊
typedef struct {
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t active_allocations;
    size_t current_allocated_bytes;
    size_t peak_allocated_bytes;
    size_t total_allocated_bytes;
    size_t pool_size;
    size_t pool_used;
    double fragmentation_ratio;
    retryix_svm_level_t active_level;
} retryix_svm_stats_t;

// SVM 能力查詢資訊
typedef struct {
    retryix_svm_level_t max_level;
    uint64_t opencl_capabilities;
    size_t max_allocation_size;
    size_t required_alignment;
    bool supports_atomics;
    bool supports_migration;
    char device_name[256];
    char device_version[64];
} retryix_svm_device_info_t;

// Logging callback type (simple integer log level)
typedef void (*retryix_svm_log_callback_t)(int level, const char* message, void* user_data);

