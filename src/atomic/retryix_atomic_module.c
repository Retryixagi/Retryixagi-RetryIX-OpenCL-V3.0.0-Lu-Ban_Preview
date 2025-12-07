// RetryIX 3.0.0 "魯班" 原子操作模塊 - 分身術第五分身
// 基於魯班智慧：精密機關術（無鎖並行）
// Version: 3.0.0 Codename: 魯班 (Lu Ban)
#define RETRYIX_BUILD_DLL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#define RETRYIX_API __declspec(dllexport)
#else
#define RETRYIX_API __attribute__((visibility("default")))
#endif

#define RETRYIX_CALL __cdecl

typedef enum {
    RETRYIX_SUCCESS = 0,
    RETRYIX_ERROR_NOT_INITIALIZED = -6,
    RETRYIX_ERROR_INVALID_PARAMETER = -1,
    RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED = -33
} retryix_result_t;

// === 原子操作狀態（下卷智慧：機關狀態）===
static bool g_atomic_initialized = false;

// === i32原子比較交換（上卷技術：精密機關術）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_compare_exchange_i32(
    volatile int32_t* target, int32_t* expected, int32_t desired) {

    if (!target || !expected) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] i32 compare-exchange operation\n");

#ifdef _WIN32
    int32_t original = InterlockedCompareExchange((LONG*)target, desired, *expected);
    if (original == *expected) {
        printf("[Atomic Lu Ban] i32 exchange successful: %d -> %d\n", *expected, desired);
        return RETRYIX_SUCCESS;
    } else {
        *expected = original;
        printf("[Atomic Lu Ban] i32 exchange failed, current value: %d\n", original);
        return RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED;
    }
#else
    // GCC builtin atomic operations
    if (__atomic_compare_exchange_n(target, expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        return RETRYIX_SUCCESS;
    }
    return RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED;
#endif
}

// === i64原子比較交換===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_compare_exchange_i64(
    volatile int64_t* target, int64_t* expected, int64_t desired) {

    if (!target || !expected) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] i64 compare-exchange operation\n");

#ifdef _WIN32
    int64_t original = InterlockedCompareExchange64((LONGLONG*)target, desired, *expected);
    if (original == *expected) {
        return RETRYIX_SUCCESS;
    } else {
        *expected = original;
        return RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED;
    }
#else
    if (__atomic_compare_exchange_n(target, expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        return RETRYIX_SUCCESS;
    }
    return RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED;
#endif
}

// === i32原子交換===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_exchange_i32(
    volatile int32_t* target, int32_t desired, int32_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] i32 exchange operation\n");

#ifdef _WIN32
    *previous = InterlockedExchange((LONG*)target, desired);
    printf("[Atomic Lu Ban] i32 exchanged: %d -> %d\n", *previous, desired);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_exchange_n(target, desired, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

// === i64原子交換===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_exchange_i64(
    volatile int64_t* target, int64_t desired, int64_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] i64 exchange operation\n");

#ifdef _WIN32
    *previous = InterlockedExchange64((LONGLONG*)target, desired);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_exchange_n(target, desired, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

// === 原子加法操作（各種數據類型）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_add_i32(
    volatile int32_t* target, int32_t value, int32_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] i32 fetch-add: %d\n", value);

#ifdef _WIN32
    *previous = InterlockedExchangeAdd((LONG*)target, value);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_add_i64(
    volatile int64_t* target, int64_t value, int64_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] i64 fetch-add: %lld\n", (long long)value);

#ifdef _WIN32
    *previous = InterlockedExchangeAdd64((LONGLONG*)target, value);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_add_u32(
    volatile uint32_t* target, uint32_t value, uint32_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] u32 fetch-add: %u\n", value);

#ifdef _WIN32
    *previous = InterlockedExchangeAdd((LONG*)target, value);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_add_u64(
    volatile uint64_t* target, uint64_t value, uint64_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] u64 fetch-add: %llu\n", (unsigned long long)value);

#ifdef _WIN32
    *previous = InterlockedExchangeAdd64((LONGLONG*)target, value);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

// === 浮點數原子加法（特殊實現）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_add_f32(
    volatile float* target, float value, float* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] f32 fetch-add: %f\n", value);

    // 浮點數原子操作需要特殊處理
    volatile uint32_t* int_target = (volatile uint32_t*)target;
    uint32_t old_bits, new_bits;
    float old_value, new_value;

    do {
        old_bits = *int_target;
        old_value = *(float*)&old_bits;
        new_value = old_value + value;
        new_bits = *(uint32_t*)&new_value;
    } while (retryix_atomic_compare_exchange_i32((volatile int32_t*)int_target,
                                                (int32_t*)&old_bits, new_bits) != RETRYIX_SUCCESS);

    *previous = old_value;
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_add_f64(
    volatile double* target, double value, double* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] f64 fetch-add: %f\n", value);

    // 浮點數原子操作需要特殊處理
    volatile uint64_t* int_target = (volatile uint64_t*)target;
    uint64_t old_bits, new_bits;
    double old_value, new_value;

    do {
        old_bits = *int_target;
        old_value = *(double*)&old_bits;
        new_value = old_value + value;
        new_bits = *(uint64_t*)&new_value;
    } while (retryix_atomic_compare_exchange_i64((volatile int64_t*)int_target,
                                                (int64_t*)&old_bits, new_bits) != RETRYIX_SUCCESS);

    *previous = old_value;
    return RETRYIX_SUCCESS;
}

// === 位操作原子函數===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_and_u32(
    volatile uint32_t* target, uint32_t value, uint32_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] u32 fetch-and: 0x%08X\n", value);

#ifdef _WIN32
    *previous = InterlockedAnd((LONG*)target, value);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_or_u32(
    volatile uint32_t* target, uint32_t value, uint32_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] u32 fetch-or: 0x%08X\n", value);

#ifdef _WIN32
    *previous = InterlockedOr((LONG*)target, value);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_fetch_xor_u32(
    volatile uint32_t* target, uint32_t value, uint32_t* previous) {

    if (!target || !previous) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    printf("[Atomic Lu Ban] u32 fetch-xor: 0x%08X\n", value);

#ifdef _WIN32
    *previous = InterlockedXor((LONG*)target, value);
    return RETRYIX_SUCCESS;
#else
    *previous = __atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST);
    return RETRYIX_SUCCESS;
#endif
}