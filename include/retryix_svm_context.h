
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "retryix_opencl_compat.h"

// NOTE: avoid including retryix.h here to prevent circular includes.
// This header only needs basic types and SVM types/config.
// OpenCL 類型已在 retryix.h 中定義
#include "retryix_svm_types.h"
#include "retryix_svm_config.h"

// SVM 上下文結構
struct retryix_svm_context {
    uint32_t magic;
    int is_initialized;
    cl_context cl_context;
    cl_device_id device;
    cl_command_queue queue;
    retryix_svm_level_t level;
    retryix_svm_flags_t default_flags;
    retryix_svm_config_t config;
    retryix_svm_stats_t stats;
    size_t total_allocated;
    size_t peak_allocated;
    size_t allocation_count;
    retryix_svm_level_t svm_level;
    void* pool;
    size_t pool_size;
    size_t pool_used;
    // 回調
    retryix_svm_log_callback_t log_cb;
    void* log_cb_user;
    void* error_cb;
    void* error_cb_user;
    // 內部管理
    uint64_t next_alloc_id;
    bool supports_atomic_svm;
    // 原子操作統計
    void* atomic_stats;
};

typedef struct retryix_svm_context retryix_svm_context_t;


