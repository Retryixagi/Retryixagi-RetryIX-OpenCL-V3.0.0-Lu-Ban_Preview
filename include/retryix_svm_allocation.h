#pragma once
#include <stddef.h>
#include <stdint.h>
#include "retryix_svm_types.h"

// SVM 分配資訊結構
struct retryix_svm_allocation {
    void* ptr;
    size_t size;
    size_t actual_size;
    retryix_svm_flags_t flags;
    retryix_svm_level_t level;
    bool is_mapped;
    uint64_t allocation_id;
};

typedef struct retryix_svm_allocation retryix_svm_allocation_t;
