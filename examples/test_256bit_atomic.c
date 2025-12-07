/**
 * Test 256-bit Atomic Operations Fine-Grained Control
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

/* 256-bit structure */
typedef struct {
    uint64_t parts[4];
} uint256_t;

/* Global test variables */
static uint256_t g_atomic_value;
static CRITICAL_SECTION g_cs;

/* 256-bit Atomic CAS */
int atomic_cas_256(uint256_t* addr, const uint256_t* expected, const uint256_t* desired) {
    EnterCriticalSection(&g_cs);
    
    int match = 1;
    for (int i = 0; i < 4; i++) {
        if (addr->parts[i] != expected->parts[i]) {
            match = 0;
            break;
        }
    }
    
    if (match) {
        for (int i = 0; i < 4; i++) {
            addr->parts[i] = desired->parts[i];
        }
    }
    
    LeaveCriticalSection(&g_cs);
    return match;
}

/* 256-bit Atomic Exchange */
void atomic_exchange_256(uint256_t* addr, const uint256_t* new_val, uint256_t* old_val) {
    EnterCriticalSection(&g_cs);
    
    if (old_val) {
        for (int i = 0; i < 4; i++) {
            old_val->parts[i] = addr->parts[i];
        }
    }
    
    for (int i = 0; i < 4; i++) {
        addr->parts[i] = new_val->parts[i];
    }
    
    LeaveCriticalSection(&g_cs);
}

/* 256-bit Atomic Add */
void atomic_add_256(uint256_t* addr, const uint256_t* value) {
    EnterCriticalSection(&g_cs);
    
    uint64_t carry = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t old_val = addr->parts[i];
        addr->parts[i] = old_val + value->parts[i] + carry;
        carry = (addr->parts[i] < old_val) ? 1 : 0;
    }
    
    LeaveCriticalSection(&g_cs);
}

/* 256-bit Atomic Load */
void atomic_load_256(const uint256_t* addr, uint256_t* result) {
    EnterCriticalSection(&g_cs);
    for (int i = 0; i < 4; i++) {
        result->parts[i] = addr->parts[i];
    }
    LeaveCriticalSection(&g_cs);
}

/* 256-bit Atomic Store */
void atomic_store_256(uint256_t* addr, const uint256_t* value) {
    EnterCriticalSection(&g_cs);
    for (int i = 0; i < 4; i++) {
        addr->parts[i] = value->parts[i];
    }
    LeaveCriticalSection(&g_cs);
}

/* Helper functions */
void print_uint256(const char* label, const uint256_t* val) {
    printf("%s: 0x%016llX_%016llX_%016llX_%016llX\n", 
           label, val->parts[3], val->parts[2], val->parts[1], val->parts[0]);
}

int compare_uint256(const uint256_t* a, const uint256_t* b) {
    for (int i = 0; i < 4; i++) {
        if (a->parts[i] != b->parts[i]) return 0;
    }
    return 1;
}

/* Test 1: Basic CAS */
int test_cas_basic() {
    printf("\n[Test 1] Basic 256-bit CAS\n");
    printf("=====================================\n");
    
    uint256_t initial = {{0x1111111111111111ULL, 0x2222222222222222ULL, 
                          0x3333333333333333ULL, 0x4444444444444444ULL}};
    uint256_t expected = {{0x1111111111111111ULL, 0x2222222222222222ULL, 
                           0x3333333333333333ULL, 0x4444444444444444ULL}};
    uint256_t desired = {{0xAAAAAAAAAAAAAAAAULL, 0xBBBBBBBBBBBBBBBBULL, 
                          0xCCCCCCCCCCCCCCCCULL, 0xDDDDDDDDDDDDDDDDULL}};
    
    atomic_store_256(&g_atomic_value, &initial);
    print_uint256("  Initial", &initial);
    print_uint256("  Expected", &expected);
    print_uint256("  Desired", &desired);
    
    int result = atomic_cas_256(&g_atomic_value, &expected, &desired);
    printf("  CAS Result: %s\n", result ? "SUCCESS" : "FAILED");
    
    uint256_t current;
    atomic_load_256(&g_atomic_value, &current);
    print_uint256("  Current", &current);
    
    int pass = (result == 1) && compare_uint256(&current, &desired);
    printf("  Test: %s\n", pass ? "PASS" : "FAIL");
    
    return pass;
}

/* Test 2: CAS Fail */
int test_cas_fail() {
    printf("\n[Test 2] CAS Fail (Expected Mismatch)\n");
    printf("=====================================\n");
    
    uint256_t initial = {{0x1111111111111111ULL, 0x2222222222222222ULL, 
                          0x3333333333333333ULL, 0x4444444444444444ULL}};
    uint256_t wrong_expected = {{0x9999999999999999ULL, 0x8888888888888888ULL, 
                                  0x7777777777777777ULL, 0x6666666666666666ULL}};
    uint256_t desired = {{0xAAAAAAAAAAAAAAAAULL, 0xBBBBBBBBBBBBBBBBULL, 
                          0xCCCCCCCCCCCCCCCCULL, 0xDDDDDDDDDDDDDDDDULL}};
    
    atomic_store_256(&g_atomic_value, &initial);
    print_uint256("  Initial", &initial);
    print_uint256("  Wrong Expected", &wrong_expected);
    
    int result = atomic_cas_256(&g_atomic_value, &wrong_expected, &desired);
    printf("  CAS Result: %s\n", result ? "SUCCESS" : "FAILED");
    
    uint256_t current;
    atomic_load_256(&g_atomic_value, &current);
    print_uint256("  Current", &current);
    
    int pass = (result == 0) && compare_uint256(&current, &initial);
    printf("  Test: %s (value should remain unchanged)\n", pass ? "PASS" : "FAIL");
    
    return pass;
}

/* Test 3: Exchange */
int test_exchange() {
    printf("\n[Test 3] 256-bit Exchange\n");
    printf("=====================================\n");
    
    uint256_t initial = {{0x1111111111111111ULL, 0x2222222222222222ULL, 
                          0x3333333333333333ULL, 0x4444444444444444ULL}};
    uint256_t new_value = {{0x5555555555555555ULL, 0x6666666666666666ULL, 
                            0x7777777777777777ULL, 0x8888888888888888ULL}};
    uint256_t old_value;
    
    atomic_store_256(&g_atomic_value, &initial);
    print_uint256("  Initial", &initial);
    print_uint256("  New Value", &new_value);
    
    atomic_exchange_256(&g_atomic_value, &new_value, &old_value);
    print_uint256("  Returned Old", &old_value);
    
    uint256_t current;
    atomic_load_256(&g_atomic_value, &current);
    print_uint256("  Current", &current);
    
    int pass = compare_uint256(&old_value, &initial) && 
               compare_uint256(&current, &new_value);
    printf("  Test: %s\n", pass ? "PASS" : "FAIL");
    
    return pass;
}

/* Test 4: Add (no overflow) */
int test_add_no_overflow() {
    printf("\n[Test 4] 256-bit Add (No Overflow)\n");
    printf("=====================================\n");
    
    uint256_t initial = {{0x1000000000000000ULL, 0x2000000000000000ULL, 
                          0x3000000000000000ULL, 0x4000000000000000ULL}};
    uint256_t add_value = {{0x0000000000000001ULL, 0x0000000000000002ULL, 
                            0x0000000000000003ULL, 0x0000000000000004ULL}};
    uint256_t expected = {{0x1000000000000001ULL, 0x2000000000000002ULL, 
                           0x3000000000000003ULL, 0x4000000000000004ULL}};
    
    atomic_store_256(&g_atomic_value, &initial);
    print_uint256("  Initial", &initial);
    print_uint256("  Add Value", &add_value);
    
    atomic_add_256(&g_atomic_value, &add_value);
    
    uint256_t current;
    atomic_load_256(&g_atomic_value, &current);
    print_uint256("  Result", &current);
    print_uint256("  Expected", &expected);
    
    int pass = compare_uint256(&current, &expected);
    printf("  Test: %s\n", pass ? "PASS" : "FAIL");
    
    return pass;
}

/* Test 5: Add with carry */
int test_add_with_carry() {
    printf("\n[Test 5] 256-bit Add (With Carry)\n");
    printf("=====================================\n");
    
    uint256_t initial = {{0xFFFFFFFFFFFFFFFFULL, 0x0000000000000001ULL, 
                          0x0000000000000000ULL, 0x0000000000000000ULL}};
    uint256_t add_value = {{0x0000000000000001ULL, 0x0000000000000000ULL, 
                            0x0000000000000000ULL, 0x0000000000000000ULL}};
    uint256_t expected = {{0x0000000000000000ULL, 0x0000000000000002ULL, 
                           0x0000000000000000ULL, 0x0000000000000000ULL}};
    
    atomic_store_256(&g_atomic_value, &initial);
    print_uint256("  Initial", &initial);
    print_uint256("  Add Value", &add_value);
    
    atomic_add_256(&g_atomic_value, &add_value);
    
    uint256_t current;
    atomic_load_256(&g_atomic_value, &current);
    print_uint256("  Result", &current);
    print_uint256("  Expected", &expected);
    
    int pass = compare_uint256(&current, &expected);
    printf("  Test: %s (check carry)\n", pass ? "PASS" : "FAIL");
    
    return pass;
}

/* Test 6: Multi-threaded */
#define NUM_THREADS 8
#define OPS_PER_THREAD 1000

static volatile long g_completed_threads = 0;

DWORD WINAPI thread_worker(LPVOID param) {
    int thread_id = (int)(intptr_t)param;
    uint256_t add_val = {{1, 0, 0, 0}};
    
    for (int i = 0; i < OPS_PER_THREAD; i++) {
        atomic_add_256(&g_atomic_value, &add_val);
    }
    
    InterlockedIncrement(&g_completed_threads);
    return 0;
}

int test_multithreaded() {
    printf("\n[Test 6] Multi-threaded Concurrent Test\n");
    printf("=====================================\n");
    printf("  Threads: %d\n", NUM_THREADS);
    printf("  Ops/Thread: %d\n", OPS_PER_THREAD);
    printf("  Total Ops: %d\n", NUM_THREADS * OPS_PER_THREAD);
    
    uint256_t zero = {{0, 0, 0, 0}};
    atomic_store_256(&g_atomic_value, &zero);
    
    HANDLE threads[NUM_THREADS];
    g_completed_threads = 0;
    
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i] = CreateThread(NULL, 0, thread_worker, (LPVOID)(intptr_t)i, 0, NULL);
    }
    
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
    
    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        CloseHandle(threads[i]);
    }
    
    uint256_t current;
    atomic_load_256(&g_atomic_value, &current);
    
    uint64_t expected_count = (uint64_t)NUM_THREADS * OPS_PER_THREAD;
    
    printf("\n  Elapsed: %.3f sec\n", elapsed);
    printf("  Rate: %.0f ops/sec\n", (NUM_THREADS * OPS_PER_THREAD) / elapsed);
    printf("  Expected: %llu\n", expected_count);
    printf("  Actual: %llu\n", current.parts[0]);
    
    int pass = (current.parts[0] == expected_count);
    printf("  Test: %s\n", pass ? "PASS (no data race)" : "FAIL (data race detected)");
    
    return pass;
}

/* Main */
int main() {
    printf("============================================================\n");
    printf("256-bit Atomic Operations Fine-Grained Control Test\n");
    printf("============================================================\n");
    
    InitializeCriticalSection(&g_cs);
    
    int total_tests = 0;
    int passed_tests = 0;
    
    total_tests++; if (test_cas_basic()) passed_tests++;
    total_tests++; if (test_cas_fail()) passed_tests++;
    total_tests++; if (test_exchange()) passed_tests++;
    total_tests++; if (test_add_no_overflow()) passed_tests++;
    total_tests++; if (test_add_with_carry()) passed_tests++;
    total_tests++; if (test_multithreaded()) passed_tests++;
    
    printf("\n============================================================\n");
    printf("Test Summary\n");
    printf("============================================================\n");
    printf("Total: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success Rate: %.1f%%\n", (100.0 * passed_tests) / total_tests);
    printf("\n");
    
    if (passed_tests == total_tests) {
        printf("All tests passed! 256-bit atomic operations working!\n");
    } else {
        printf("Some tests failed, need further investigation\n");
    }
    
    DeleteCriticalSection(&g_cs);
    
    return (passed_tests == total_tests) ? 0 : 1;
}
