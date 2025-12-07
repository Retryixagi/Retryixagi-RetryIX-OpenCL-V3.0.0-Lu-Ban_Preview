#ifndef RETRYIX_BUILD_DLL
#define RETRYIX_BUILD_DLL_TEMP
#define RETRYIX_BUILD_DLL
#endif
#include "../../include/retryix_export.h"
#ifdef RETRYIX_BUILD_DLL_TEMP
#undef RETRYIX_BUILD_DLL
#undef RETRYIX_BUILD_DLL_TEMP
#endif
#include "../../include/retryix_svm.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ============================================================================
// RetryIX v3.0.0 - 128/256-bit 原子操作實作
// ============================================================================

// 動態偵測 SVM atomic 能力，支援則執行 atomic，否則回傳 NOT_SUPPORTED

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#pragma intrinsic(_InterlockedCompareExchange128)
#define HOST_ATOMIC_ADD(ptr, val) InterlockedAdd((volatile LONG*)(ptr), (LONG)(val))
#else
#include <pthread.h>
#define HOST_ATOMIC_ADD(ptr, val) __sync_fetch_and_add((volatile int*)(ptr), (int)(val))
#endif

// ============================================================================
// 128-bit 原子操作 - Spinlock Fallback (Slow Path)
// ============================================================================

#define SPINLOCK_TABLE_SIZE 256
static volatile uint8_t g_spinlocks[SPINLOCK_TABLE_SIZE] = {0};

static inline size_t get_lock_index(const void* p) {
    return ((uintptr_t)p >> 6) % SPINLOCK_TABLE_SIZE;
}

static inline void lock_addr(const void* p) {
    size_t idx = get_lock_index(p);
#ifdef _WIN32
    while (InterlockedExchange8((volatile char*)&g_spinlocks[idx], 1) != 0) {
        // Spin
        _mm_pause();
    }
#else
    while (__sync_lock_test_and_set(&g_spinlocks[idx], 1)) {
        // Spin
        __builtin_ia32_pause();
    }
#endif
}

static inline void unlock_addr(const void* p) {
    size_t idx = get_lock_index(p);
#ifdef _WIN32
    InterlockedExchange8((volatile char*)&g_spinlocks[idx], 0);
#else
    __sync_lock_release(&g_spinlocks[idx]);
#endif
}

static int supports_atomic(const retryix_svm_context_t* ctx) {
	return ctx && ctx->is_initialized && ctx->supports_atomic_svm;
}

// 指標合法性檢查（舊版安全邏輯）：只要 context/init/ptr 非 NULL 即允許
static int is_valid_svm_ptr(retryix_svm_context_t* ctx, void* ptr) {
	return ctx && ctx->is_initialized && ptr != NULL;
}

// ============================================================================
// v3.0.0: 原子能力查詢 API
// ============================================================================

RETRYIX_API uint32_t RETRYIX_CALL retryix_svm_atomic_capabilities(const retryix_svm_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return RETRYIX_ATOMIC_CAP_NONE;
    }
    
    uint32_t caps = RETRYIX_ATOMIC_CAP_NONE;
    
    // 32-bit 原子操作 (基本支援)
    if (ctx->supports_atomic_svm) {
        caps |= RETRYIX_ATOMIC_CAP_32BIT;
        caps |= RETRYIX_ATOMIC_CAP_64BIT;
    }
    
    // 128-bit 能力偵測
#if RETRYIX_HAS_INT128_TYPE
    #if RETRYIX_HAS_INT128_NATIVE
        // GCC/Clang: 原生 __int128 + __atomic built-ins
        caps |= RETRYIX_ATOMIC_CAP_128_NATIVE;
    #elif defined(_MSC_VER) && defined(_M_X64)
        // MSVC: _InterlockedCompareExchange128 可用
        caps |= RETRYIX_ATOMIC_CAP_128_NATIVE;  // MSVC intrinsic 視為 native
    #else
        // Fallback 到軟體模擬 (spinlock)
        caps |= RETRYIX_ATOMIC_CAP_128_EMULATED;
    #endif
    
    // 256-bit pair CAS (需要 128-bit 型別支援)
    caps |= RETRYIX_ATOMIC_CAP_256_PAIR;
#endif
    
    return caps;
}

// 舊版相容 API (deprecated)
RETRYIX_API int RETRYIX_CALL retryix_svm_has_atomic_support(const retryix_svm_context_t* ctx) {
    return supports_atomic(ctx);
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_sub_int32(retryix_svm_context_t* ctx, volatile int32_t* ptr, int32_t value, int32_t* old_value) {
	if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
	if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	*old_value = HOST_ATOMIC_ADD(ptr, -value);
	return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_and_int32(retryix_svm_context_t* ctx, volatile int32_t* ptr, int32_t value, int32_t* old_value) {
	if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
	if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	
#ifdef _WIN32
	*old_value = InterlockedAnd((volatile LONG*)ptr, (LONG)value);
#else
	*old_value = __sync_fetch_and_and((volatile int*)ptr, value);
#endif
	return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_or_int32(retryix_svm_context_t* ctx, volatile int32_t* ptr, int32_t value, int32_t* old_value) {
	if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
	if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	
#ifdef _WIN32
	*old_value = InterlockedOr((volatile LONG*)ptr, (LONG)value);
#else
	*old_value = __sync_fetch_and_or((volatile int*)ptr, value);
#endif
	return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_xor_int32(retryix_svm_context_t* ctx, volatile int32_t* ptr, int32_t value, int32_t* old_value) {
	if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
	if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	
#ifdef _WIN32
	*old_value = InterlockedXor((volatile LONG*)ptr, (LONG)value);
#else
	*old_value = __sync_fetch_and_xor((volatile int*)ptr, value);
#endif
	return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_exchange_int32(retryix_svm_context_t* ctx, volatile int32_t* ptr, int32_t value, int32_t* old_value) {
	if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
	if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	
#ifdef _WIN32
	*old_value = InterlockedExchange((volatile LONG*)ptr, (LONG)value);
#else
	*old_value = __sync_lock_test_and_set((volatile int*)ptr, value);
#endif
	return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_min_int32(retryix_svm_context_t* ctx, volatile int32_t* ptr, int32_t value, int32_t* old_value) {
	if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
	if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
	
	// 使用 CAS 迴圈實現原子 min
#ifdef _WIN32
	LONG expected, desired;
	do {
		expected = *ptr;
		desired = (value < expected) ? value : expected;
	} while (InterlockedCompareExchange((volatile LONG*)ptr, desired, expected) != expected);
	*old_value = expected;
#else
	int expected, desired;
	do {
		expected = *ptr;
		desired = (value < expected) ? value : expected;
	} while (!__sync_bool_compare_and_swap((volatile int*)ptr, expected, desired));
	*old_value = expected;
#endif
	return RETRYIX_SVM_SUCCESS;
}

// ============================================================================
// v3.0.0: 128-bit 原子操作實作
// ============================================================================

#if RETRYIX_HAS_INT128

// 檢查是否支援 128-bit 原子操作
static inline bool supports_atomic_128(const retryix_svm_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) return false;
    
#if defined(__GNUC__) || defined(__clang__)
    // GCC/Clang 在 x86_64 支援 __int128 原子操作
    return true;
#elif defined(_MSC_VER) && defined(_M_X64)
    // MSVC 支援 _InterlockedCompareExchange128
    return true;
#else
    return false;
#endif
}

// 128-bit Fetch-Add 實作
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_fetch_add_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t value,
    u128_t* old_value)
{
    if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
    if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    
    // 檢查 16-byte 對齊
    if (((uintptr_t)ptr & 0xF) != 0) {
        return RETRYIX_SVM_ERROR_ALIGNMENT;
    }
    
#if defined(__GNUC__) || defined(__clang__)
    // GCC/Clang 快速路徑：使用 __atomic built-ins
    if (supports_atomic_128(ctx)) {
        u128_t expected = __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
        while (true) {
            u128_t desired = expected + value;
            if (__atomic_compare_exchange_n(ptr, &expected, desired, false, 
                                            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                *old_value = expected;
                
                // 統計：快速路徑
                if (ctx->atomic_stats) {
                    HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_128bit_ops, 1);
                    HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_fast_path, 1);
                }
                
                return RETRYIX_SVM_SUCCESS;
            }
            // expected 已被 compare_exchange 更新，繼續迴圈
        }
    }
#elif defined(_MSC_VER) && defined(_M_X64)
    // MSVC 快速路徑：使用 _InterlockedCompareExchange128
    if (supports_atomic_128(ctx)) {
        __int64* p64 = (__int64*)ptr;
        __int64 expected_lo, expected_hi;
        __int64 desired_lo, desired_hi;
        
        do {
            expected_lo = p64[0];
            expected_hi = p64[1];
            
            // 128-bit 加法
            u128_t expected_val = {expected_lo, expected_hi};
            u128_t desired_val;
            desired_val.lo = expected_val.lo + ((uint64_t*)&value)[0];
            desired_val.hi = expected_val.hi + ((uint64_t*)&value)[1];
            if (desired_val.lo < expected_val.lo) desired_val.hi++; // Carry
            
            desired_lo = desired_val.lo;
            desired_hi = desired_val.hi;
            
        } while (!_InterlockedCompareExchange128(p64, desired_hi, desired_lo, &expected_lo));
        
        // 返回舊值
        ((uint64_t*)old_value)[0] = expected_lo;
        ((uint64_t*)old_value)[1] = expected_hi;
        
        // 統計：快速路徑
        if (ctx->atomic_stats) {
            HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_128bit_ops, 1);
            HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_fast_path, 1);
        }
        
        return RETRYIX_SVM_SUCCESS;
    }
#endif
    
    // Fallback 慢速路徑：使用 spinlock
    lock_addr((void*)ptr);
    u128_t prev = *ptr;
    *ptr = prev + value;
    *old_value = prev;
    unlock_addr((void*)ptr);
    
    // 統計：慢速路徑
    if (ctx->atomic_stats) {
        HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_128bit_ops, 1);
        HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_slow_path, 1);
    }
    
    return RETRYIX_SVM_SUCCESS;
}

// 128-bit Compare-Exchange 實作
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_compare_exchange_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t* expected,
    u128_t desired)
{
    if (!ctx || !ptr || !expected) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
    if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    
    // 檢查 16-byte 對齊
    if (((uintptr_t)ptr & 0xF) != 0) {
        return RETRYIX_SVM_ERROR_ALIGNMENT;
    }
    
#if defined(__GNUC__) || defined(__clang__)
    if (supports_atomic_128(ctx)) {
        bool success = __atomic_compare_exchange_n(ptr, expected, desired, false,
                                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        
        if (ctx->atomic_stats) {
            HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_128bit_ops, 1);
            HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_fast_path, 1);
        }
        
        return success ? RETRYIX_SVM_SUCCESS : RETRYIX_SVM_ERROR_INTERNAL;
    }
#elif defined(_MSC_VER) && defined(_M_X64)
    if (supports_atomic_128(ctx)) {
        __int64* p64 = (__int64*)ptr;
        __int64 exp_lo = ((uint64_t*)expected)[0];
        __int64 exp_hi = ((uint64_t*)expected)[1];
        __int64 des_lo = ((uint64_t*)&desired)[0];
        __int64 des_hi = ((uint64_t*)&desired)[1];
        
        unsigned char success = _InterlockedCompareExchange128(p64, des_hi, des_lo, &exp_lo);
        
        if (!success) {
            // 更新 expected 為當前值
            ((uint64_t*)expected)[0] = exp_lo;
            ((uint64_t*)expected)[1] = (__int64)p64[1];
        }
        
        if (ctx->atomic_stats) {
            HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_128bit_ops, 1);
            HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_fast_path, 1);
        }
        
        return success ? RETRYIX_SVM_SUCCESS : RETRYIX_SVM_ERROR_INTERNAL;
    }
#endif
    
    // Fallback
    lock_addr((void*)ptr);
    bool success = false;
    if (memcmp((void*)ptr, expected, sizeof(u128_t)) == 0) {
        *ptr = desired;
        success = true;
    } else {
        *expected = *ptr;
    }
    unlock_addr((void*)ptr);
    
    if (ctx->atomic_stats) {
        HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_128bit_ops, 1);
        HOST_ATOMIC_ADD(&ctx->atomic_stats->atomic_slow_path, 1);
    }
    
    return success ? RETRYIX_SVM_SUCCESS : RETRYIX_SVM_ERROR_INTERNAL;
}

// 128-bit Exchange 實作
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_exchange_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t value,
    u128_t* old_value)
{
    if (!ctx || !ptr || !old_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
    if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    
    // 檢查 16-byte 對齊
    if (((uintptr_t)ptr & 0xF) != 0) {
        return RETRYIX_SVM_ERROR_ALIGNMENT;
    }
    
#if defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: 使用 CAS 迴圈實作 exchange
    u128_t expected = __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
    while (!__atomic_compare_exchange_n(ptr, &expected, value, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        // expected 已更新
    }
    *old_value = expected;
#elif defined(_MSC_VER) && defined(_M_X64)
    // MSVC: 使用 _InterlockedCompareExchange128 迴圈
    __int64* p64 = (__int64*)ptr;
    __int64 exp_lo, exp_hi;
    __int64 val_lo = ((uint64_t*)&value)[0];
    __int64 val_hi = ((uint64_t*)&value)[1];
    
    do {
        exp_lo = p64[0];
        exp_hi = p64[1];
    } while (!_InterlockedCompareExchange128(p64, val_hi, val_lo, &exp_lo));
    
    ((uint64_t*)old_value)[0] = exp_lo;
    ((uint64_t*)old_value)[1] = exp_hi;
#else
    // Fallback: spinlock
    lock_addr((void*)ptr);
    *old_value = *ptr;
    *ptr = value;
    unlock_addr((void*)ptr);
#endif
    
    return RETRYIX_SVM_SUCCESS;
}

// 128-bit Load/Store 實作
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_load_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t* out_value)
{
    if (!ctx || !ptr || !out_value) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
    if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    
    // 檢查 16-byte 對齊
    if (((uintptr_t)ptr & 0xF) != 0) {
        return RETRYIX_SVM_ERROR_ALIGNMENT;
    }
    
#if defined(__GNUC__) || defined(__clang__)
    *out_value = __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
#elif defined(_MSC_VER) && defined(_M_X64)
    // MSVC: 使用 CAS with same value 實現原子 load
    __int64* p64 = (__int64*)ptr;
    __int64 lo = p64[0];
    __int64 hi = p64[1];
    
    // 用 CAS 確保原子性讀取
    _InterlockedCompareExchange128(p64, hi, lo, &lo);
    
    ((uint64_t*)out_value)[0] = lo;
    ((uint64_t*)out_value)[1] = hi;
#else
    lock_addr((void*)ptr);
    *out_value = *ptr;
    unlock_addr((void*)ptr);
#endif
    
    return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_store_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t value)
{
    if (!ctx || !ptr) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (!supports_atomic(ctx)) return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
    if (!is_valid_svm_ptr(ctx, (void*)ptr)) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    
    // 檢查 16-byte 對齊
    if (((uintptr_t)ptr & 0xF) != 0) {
        return RETRYIX_SVM_ERROR_ALIGNMENT;
    }
    
#if defined(__GNUC__) || defined(__clang__)
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
#elif defined(_MSC_VER) && defined(_M_X64)
    // MSVC: 使用 exchange 實現原子 store
    __int64* p64 = (__int64*)ptr;
    __int64 val_lo = ((uint64_t*)&value)[0];
    __int64 val_hi = ((uint64_t*)&value)[1];
    __int64 exp_lo, exp_hi;
    
    do {
        exp_lo = p64[0];
        exp_hi = p64[1];
    } while (!_InterlockedCompareExchange128(p64, val_hi, val_lo, &exp_lo));
#else
    lock_addr((void*)ptr);
    *ptr = value;
    unlock_addr((void*)ptr);
#endif
    
    return RETRYIX_SVM_SUCCESS;
}

#endif // RETRYIX_HAS_INT128

// ============================================================================
// v3.0.0: 256-bit Pair CAS 實作
// ============================================================================

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_compare_exchange_pair_256(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr_lo,
    volatile u128_t* ptr_hi,
    u256_pair_t* expected,
    u256_pair_t desired)
{
    if (!ctx || !ptr_lo || !ptr_hi || !expected) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    
    // 使用雙重 CAS（注意 ABA 問題）
    lock_addr((void*)ptr_lo);
    lock_addr((void*)ptr_hi);
    
    bool success = false;
    if (memcmp((void*)ptr_lo, &expected->lo, sizeof(u128_t)) == 0 &&
        memcmp((void*)ptr_hi, &expected->hi, sizeof(u128_t)) == 0) {
        *ptr_lo = desired.lo;
        *ptr_hi = desired.hi;
        success = true;
    } else {
        expected->lo = *ptr_lo;
        expected->hi = *ptr_hi;
    }
    
    unlock_addr((void*)ptr_hi);
    unlock_addr((void*)ptr_lo);
    
    // 統計更新(後續實作)
    (void)ctx->atomic_stats;
    
    return success ? RETRYIX_SVM_SUCCESS : RETRYIX_SVM_ERROR_INTERNAL;
}
