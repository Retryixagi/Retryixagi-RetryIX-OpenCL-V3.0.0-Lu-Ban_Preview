// Production: legacy 128/256 host-side atomic code removed to avoid duplicate
// symbol exports. Use src/svm/retryix_svm_atomic.c for canonical SVM-aware
// implementations of 128/256-bit atomic APIs.

#include "../../include/retryix_export.h"

/* No exported symbols in this file (production stub) */

// ============================================================================
// 128-bit 原子操作 - Compare Exchange
// ============================================================================
RETRYIX_API int RETRYIX_CALL retryix_svm_atomic_compare_exchange_i128(
    void* ptr,
    int64_t* expected_low,
    int64_t* expected_high,
    int64_t desired_low,
    int64_t desired_high)
{
    if (!ptr || !expected_low || !expected_high) {
        return -1;
    }

#ifdef _WIN32
    volatile __int64* target = (__int64*)ptr;
    __int64 comparand[2] = { *expected_low, *expected_high };
    
    unsigned char success = _InterlockedCompareExchange128(
        target, 
        desired_high, 
        desired_low, 
        comparand
    );
    
    if (!success) {
        // 交換失敗,更新期望值為當前值
        *expected_low = comparand[0];
        *expected_high = comparand[1];
    }
    
    return success ? 0 : -33;
    
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__aarch64__))
    typedef __int128 int128_t;
    _Atomic(int128_t)* target = (_Atomic(int128_t)*)ptr;
    
    int128_t expected = ((int128_t)(*expected_high) << 64) | (uint64_t)(*expected_low);
    int128_t desired = ((int128_t)desired_high << 64) | (uint64_t)desired_low;
    
    bool success = __atomic_compare_exchange_n(
        target, &expected, desired, 
        false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    );
    
    if (!success) {
        *expected_low = (int64_t)expected;
        *expected_high = (int64_t)(expected >> 64);
    }
    
    return success ? 0 : -33;
    
#else
/* stub */

// ============================================================================
// 128-bit 原子操作 - Exchange
// ============================================================================
RETRYIX_API int RETRYIX_CALL retryix_svm_atomic_exchange_i128(
    void* ptr,
    int64_t new_low,
    int64_t new_high,
    int64_t* old_low,
    int64_t* old_high)
{
    if (!ptr || !old_low || !old_high) {
        return -1;
    }

#ifdef _WIN32
    volatile __int64* target = (__int64*)ptr;
    __int64 comparand[2];
    
    do {
        comparand[0] = target[0];
        comparand[1] = target[1];
    } while (!_InterlockedCompareExchange128(target, new_high, new_low, comparand));
    
    *old_low = comparand[0];
    *old_high = comparand[1];
    return 0;
    
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__aarch64__))
    typedef __int128 int128_t;
    _Atomic(int128_t)* target = (_Atomic(int128_t)*)ptr;
    
    int128_t new_value = ((int128_t)new_high << 64) | (uint64_t)new_low;
    int128_t old_value = __atomic_exchange_n(target, new_value, __ATOMIC_SEQ_CST);
    
    *old_low = (int64_t)old_value;
    *old_high = (int64_t)(old_value >> 64);
    return 0;
    
#else
    return -33;
#endif
}

// ============================================================================
// 128-bit 原子操作 - Load
// ============================================================================
RETRYIX_API int RETRYIX_CALL retryix_svm_atomic_load_i128(
    const void* ptr,
    int64_t* low,
    int64_t* high)
{
    if (!ptr || !low || !high) {
        return -1;
    }

#ifdef _WIN32
    volatile __int64* target = (__int64*)ptr;
    __int64 zero[2] = {0, 0};
    __int64 result[2] = {0, 0};
    
    // 使用 CAS 實現原子讀取: 與0比較但總是失敗,返回當前值
    _InterlockedCompareExchange128(target, 0, 0, result);
    
    *low = result[0];
    *high = result[1];
    return 0;
    
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__aarch64__))
    typedef __int128 int128_t;
    _Atomic(int128_t)* target = (_Atomic(int128_t)*)ptr;
    
    int128_t value = __atomic_load_n(target, __ATOMIC_SEQ_CST);
    
    *low = (int64_t)value;
    *high = (int64_t)(value >> 64);
    return 0;
    
#else
    return -33;
#endif
}

// ============================================================================
// 128-bit 原子操作 - Store
// ============================================================================
RETRYIX_API int RETRYIX_CALL retryix_svm_atomic_store_i128(
    void* ptr,
    int64_t low,
    int64_t high)
{
    if (!ptr) {
        return -1;
    }

#ifdef _WIN32
    volatile __int64* target = (__int64*)ptr;
    __int64 comparand[2];
    
    // 使用 CAS 循環實現原子寫入
    do {
        comparand[0] = target[0];
        comparand[1] = target[1];
    } while (!_InterlockedCompareExchange128(target, high, low, comparand));
    
    return 0;
    
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__aarch64__))
    typedef __int128 int128_t;
    _Atomic(int128_t)* target = (_Atomic(int128_t)*)ptr;
    
    int128_t value = ((int128_t)high << 64) | (uint64_t)low;
    __atomic_store_n(target, value, __ATOMIC_SEQ_CST);
    return 0;
    
#else
    return -33;
#endif
}

// ============================================================================
// 256-bit 原子操作 - Compare Exchange (兩個 128-bit 對)
// ============================================================================
RETRYIX_API int RETRYIX_CALL retryix_svm_atomic_compare_exchange_pair_256(
    void* ptr1,
    void* ptr2,
    int64_t* expected1_low, int64_t* expected1_high,
    int64_t* expected2_low, int64_t* expected2_high,
    int64_t desired1_low, int64_t desired1_high,
    int64_t desired2_low, int64_t desired2_high)
{
    if (!ptr1 || !ptr2 || 
        !expected1_low || !expected1_high || 
        !expected2_low || !expected2_high) {
        return -1;
    }

    // 注意: 真正的 256-bit 原子操作需要特殊硬件支持
    // 這裡使用兩次 128-bit CAS,不保證嚴格原子性
    // 僅用於模擬,實際應用需要考慮一致性
    
#ifdef _WIN32
    volatile __int64* target1 = (__int64*)ptr1;
    volatile __int64* target2 = (__int64*)ptr2;
    __int64 comp1[2] = { *expected1_low, *expected1_high };
    __int64 comp2[2] = { *expected2_low, *expected2_high };
    
    // 先嘗試第一個 128-bit
    unsigned char success1 = _InterlockedCompareExchange128(
        target1, desired1_high, desired1_low, comp1
    );
    
    if (!success1) {
        *expected1_low = comp1[0];
        *expected1_high = comp1[1];
        return -33;
    }
    
    // 再嘗試第二個 128-bit
    unsigned char success2 = _InterlockedCompareExchange128(
        target2, desired2_high, desired2_low, comp2
    );
    
    if (!success2) {
        // 第二個失敗,回滾第一個 (非完美原子)
        __int64 rollback[2] = { desired1_low, desired1_high };
        _InterlockedCompareExchange128(target1, comp1[1], comp1[0], rollback);
        
        *expected2_low = comp2[0];
        *expected2_high = comp2[1];
        return -33;
    }
    
    return 0;
    
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__aarch64__))
    // GCC 實現類似
    typedef __int128 int128_t;
    _Atomic(int128_t)* target1 = (_Atomic(int128_t)*)ptr1;
    _Atomic(int128_t)* target2 = (_Atomic(int128_t)*)ptr2;
    
    int128_t exp1 = ((int128_t)(*expected1_high) << 64) | (uint64_t)(*expected1_low);
    int128_t des1 = ((int128_t)desired1_high << 64) | (uint64_t)desired1_low;
    
    bool success1 = __atomic_compare_exchange_n(
        target1, &exp1, des1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    );
    
    if (!success1) {
        *expected1_low = (int64_t)exp1;
        *expected1_high = (int64_t)(exp1 >> 64);
        return -33;
    }
    
    int128_t exp2 = ((int128_t)(*expected2_high) << 64) | (uint64_t)(*expected2_low);
    int128_t des2 = ((int128_t)desired2_high << 64) | (uint64_t)desired2_low;
    
    bool success2 = __atomic_compare_exchange_n(
        target2, &exp2, des2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    );
    
    if (!success2) {
        // 回滾第一個
        int128_t rollback = des1;
        __atomic_compare_exchange_n(target1, &rollback, exp1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        
        *expected2_low = (int64_t)exp2;
        *expected2_high = (int64_t)(exp2 >> 64);
        return -33;
    }
    
    return 0;
    
#else
    return -33;
#endif
}
