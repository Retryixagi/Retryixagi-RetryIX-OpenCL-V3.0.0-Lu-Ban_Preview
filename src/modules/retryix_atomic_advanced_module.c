// RetryIX 3.0.0 高級原子操作模組 (128/256-bit)
// 簡化版本 - 無 SVM context 依賴
// Module: high-level atomic operations wrapper (128/256-bit)
#define RETRYIX_BUILD_DLL

#include "../../include/retryix_export.h"
#include "../../include/retryix_svm.h"
#include "../../include/retryix_atomic_advanced.h"
#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#pragma intrinsic(_InterlockedExchange8)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// This module provides convenience wrappers named retryix_atomic_* that map to
// the canonical SVM-aware implementations (retryix_svm_atomic_*). Keeping the
// wrapper names ensures backward compatibility with existing tests and
// consumers while centralizing 128/256 logic in src/svm/retryix_svm_atomic.c

// Spinlock
#define SPINLOCK_TABLE_SIZE 256
static volatile uint8_t g_spinlocks[SPINLOCK_TABLE_SIZE] = {0};

static inline size_t get_lock_index(const void* p) {
    return ((uintptr_t)p >> 6) % SPINLOCK_TABLE_SIZE;
}

static inline void lock_addr(const void* p) {
    size_t idx = get_lock_index(p);
#ifdef _WIN32
    while (InterlockedExchange8((volatile char*)&g_spinlocks[idx], 1) != 0) {
        _mm_pause();
    }
#endif
}

static inline void unlock_addr(const void* p) {
    size_t idx = get_lock_index(p);
#ifdef _WIN32
    InterlockedExchange8((volatile char*)&g_spinlocks[idx], 0);
#endif
}

// 能力查詢
RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_get_128bit_capabilities(void) {
#if defined(_MSC_VER) && defined(_M_X64)
    return RETRYIX_ATOMIC_CAP_128_NATIVE | RETRYIX_ATOMIC_CAP_256_PAIR;
#else
    return 0;
#endif
}

// 128-bit Fetch-Add
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_atomic_fetch_add_u128(
    volatile u128_t* ptr, u128_t value, u128_t* old_value) {
    
    if (!ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (((uintptr_t)ptr & 0xF) != 0) return RETRYIX_SVM_ERROR_ALIGNMENT;
    
#if defined(_MSC_VER) && defined(_M_X64)
    __int64* p64 = (__int64*)ptr;
    __int64 exp_lo, exp_hi, des_lo, des_hi;
    
    do {
        exp_lo = p64[0];
        exp_hi = p64[1];
        
        uint64_t sum_lo = value.lo + (uint64_t)exp_lo;
        uint64_t sum_hi = value.hi + (uint64_t)exp_hi;
        if (sum_lo < value.lo) sum_hi++;
        
        des_lo = sum_lo;
        des_hi = sum_hi;
        
    } while (!_InterlockedCompareExchange128(p64, des_hi, des_lo, &exp_lo));
    
    old_value->lo = exp_lo;
    old_value->hi = p64[1];
    
    return RETRYIX_SVM_SUCCESS;
#else
    lock_addr((void*)ptr);
    *old_value = *ptr;
    ptr->lo += value.lo;
    if (ptr->lo < old_value->lo) ptr->hi++;
    ptr->hi += value.hi;
    unlock_addr((void*)ptr);
    return RETRYIX_SVM_SUCCESS;
#endif
}

// 128-bit CAS
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_atomic_compare_exchange_u128(
    volatile u128_t* ptr, u128_t* expected, u128_t desired) {
    
    if (!ptr || !expected) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (((uintptr_t)ptr & 0xF) != 0) return RETRYIX_SVM_ERROR_ALIGNMENT;
    
#if defined(_MSC_VER) && defined(_M_X64)
    __int64* p64 = (__int64*)ptr;
    __int64 cmp[2];
    cmp[0] = expected->lo;
    cmp[1] = expected->hi;
    __int64 des_lo = desired.lo;
    __int64 des_hi = desired.hi;
    
    // _InterlockedCompareExchange128 更新 cmp 參數當 CAS 失敗
    unsigned char success = _InterlockedCompareExchange128(p64, des_hi, des_lo, cmp);
    
    if (!success) {
        // CAS 失敗，更新 expected 為當前值
        expected->lo = cmp[0];
        expected->hi = cmp[1];
    }
    
    return RETRYIX_SVM_SUCCESS;  // 總是返回成功，expected 已更新
#else
    lock_addr((void*)ptr);
    int match = (ptr->lo == expected->lo && ptr->hi == expected->hi);
    if (match) {
        *ptr = desired;
    } else {
        *expected = *ptr;
    }
    unlock_addr((void*)ptr);
    return RETRYIX_SVM_SUCCESS;  // 總是返回成功，expected 已更新
#endif
}

// 128-bit Exchange
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_atomic_exchange_u128(
    volatile u128_t* ptr, u128_t value, u128_t* old_value) {
    
    if (!ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (((uintptr_t)ptr & 0xF) != 0) return RETRYIX_SVM_ERROR_ALIGNMENT;
    
#if defined(_MSC_VER) && defined(_M_X64)
    __int64* p64 = (__int64*)ptr;
    __int64 exp_lo, exp_hi;
    __int64 val_lo = value.lo;
    __int64 val_hi = value.hi;
    
    do {
        exp_lo = p64[0];
        exp_hi = p64[1];
    } while (!_InterlockedCompareExchange128(p64, val_hi, val_lo, &exp_lo));
    
    old_value->lo = exp_lo;
    old_value->hi = exp_hi;
    
    return RETRYIX_SVM_SUCCESS;
#else
    lock_addr((void*)ptr);
    *old_value = *ptr;
    *ptr = value;
    unlock_addr((void*)ptr);
    return RETRYIX_SVM_SUCCESS;
#endif
}

// 128-bit Load
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_atomic_load_u128(
    volatile u128_t* ptr, u128_t* out_value) {
    
    if (!ptr || !out_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (((uintptr_t)ptr & 0xF) != 0) return RETRYIX_SVM_ERROR_ALIGNMENT;
    
#if defined(_MSC_VER) && defined(_M_X64)
    __int64* p64 = (__int64*)ptr;
    __int64 lo = p64[0];
    __int64 hi = p64[1];
    
    _InterlockedCompareExchange128(p64, hi, lo, &lo);
    
    out_value->lo = lo;
    out_value->hi = hi;
    
    return RETRYIX_SVM_SUCCESS;
#else
    lock_addr((void*)ptr);
    *out_value = *ptr;
    unlock_addr((void*)ptr);
    return RETRYIX_SVM_SUCCESS;
#endif
}

// 128-bit Store
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_atomic_store_u128(
    volatile u128_t* ptr, u128_t value) {
    
    if (!ptr) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (((uintptr_t)ptr & 0xF) != 0) return RETRYIX_SVM_ERROR_ALIGNMENT;
    
#if defined(_MSC_VER) && defined(_M_X64)
    __int64* p64 = (__int64*)ptr;
    __int64 val_lo = value.lo;
    __int64 val_hi = value.hi;
    __int64 exp_lo, exp_hi;
    
    do {
        exp_lo = p64[0];
        exp_hi = p64[1];
    } while (!_InterlockedCompareExchange128(p64, val_hi, val_lo, &exp_lo));
    
    return RETRYIX_SVM_SUCCESS;
#else
    lock_addr((void*)ptr);
    *ptr = value;
    unlock_addr((void*)ptr);
    return RETRYIX_SUCCESS;
#endif
}

// 256-bit Pair CAS
// Python 調用: (buf_ptr, expected_lo[2], expected_hi[2], desired_lo[2], desired_hi[2])
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_atomic_compare_exchange_pair_256(
    volatile void* ptr,
    uint64_t* expected_lo,
    uint64_t* expected_hi,
    uint64_t* desired_lo,
    uint64_t* desired_hi) {
    
    if (!ptr || !expected_lo || !expected_hi || !desired_lo || !desired_hi) {
        return RETRYIX_SVM_ERROR_INVALID_PARAM;
    }
    if (((uintptr_t)ptr & 0x1F) != 0) return RETRYIX_SVM_ERROR_ALIGNMENT;  // 32-byte aligned
    
    // 將 ptr 視為 256-bit (4x uint64_t)
    volatile uint64_t* p64 = (volatile uint64_t*)ptr;
    
    lock_addr((void*)ptr);
    
    // 比較當前值與 expected
    int match = (p64[0] == expected_lo[0] && p64[1] == expected_lo[1] &&
                 p64[2] == expected_hi[0] && p64[3] == expected_hi[1]);
    
    if (match) {
        // 匹配：寫入 desired 值
        p64[0] = desired_lo[0];
        p64[1] = desired_lo[1];
        p64[2] = desired_hi[0];
        p64[3] = desired_hi[1];
    } else {
        // 不匹配：更新 expected 為當前值
        expected_lo[0] = p64[0];
        expected_lo[1] = p64[1];
        expected_hi[0] = p64[2];
        expected_hi[1] = p64[3];
    }
    
    unlock_addr((void*)ptr);
    
    return RETRYIX_SVM_SUCCESS;  // 總是返回成功，expected 已更新
}
