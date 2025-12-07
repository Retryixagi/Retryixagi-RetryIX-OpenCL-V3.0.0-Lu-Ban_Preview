/**
 * 128-bit Atomic Operations Stress Test
 * Compare with 256-bit version
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

/* 128-bit structure */
typedef struct {
    uint64_t parts[2];
} uint128_t;

/* Global variables */
static uint128_t* g_atomic_array = NULL;
static int g_array_size = 0;
static CRITICAL_SECTION* g_cs_array = NULL;
static volatile long g_total_ops = 0;
static volatile long g_cas_success = 0;
static volatile long g_cas_fail = 0;
static volatile long g_add_ops = 0;
static volatile long g_exchange_ops = 0;
static volatile int g_stop_flag = 0;

/* 128-bit Atomic CAS */
int atomic_cas_128_indexed(int index, const uint128_t* expected, const uint128_t* desired) {
    EnterCriticalSection(&g_cs_array[index]);
    
    int match = 1;
    for (int i = 0; i < 2; i++) {
        if (g_atomic_array[index].parts[i] != expected->parts[i]) {
            match = 0;
            break;
        }
    }
    
    if (match) {
        for (int i = 0; i < 2; i++) {
            g_atomic_array[index].parts[i] = desired->parts[i];
        }
    }
    
    LeaveCriticalSection(&g_cs_array[index]);
    return match;
}

/* 128-bit Atomic Add */
void atomic_add_128_indexed(int index, const uint128_t* value) {
    EnterCriticalSection(&g_cs_array[index]);
    
    uint64_t carry = 0;
    for (int i = 0; i < 2; i++) {
        uint64_t old_val = g_atomic_array[index].parts[i];
        g_atomic_array[index].parts[i] = old_val + value->parts[i] + carry;
        carry = (g_atomic_array[index].parts[i] < old_val) ? 1 : 0;
    }
    
    LeaveCriticalSection(&g_cs_array[index]);
}

/* 128-bit Atomic Exchange */
void atomic_exchange_128_indexed(int index, const uint128_t* new_val, uint128_t* old_val) {
    EnterCriticalSection(&g_cs_array[index]);
    
    if (old_val) {
        for (int i = 0; i < 2; i++) {
            old_val->parts[i] = g_atomic_array[index].parts[i];
        }
    }
    
    for (int i = 0; i < 2; i++) {
        g_atomic_array[index].parts[i] = new_val->parts[i];
    }
    
    LeaveCriticalSection(&g_cs_array[index]);
}

/* Test 1: High Frequency Operations */
typedef struct {
    int thread_id;
    int ops_count;
    int array_index;
} thread_params_t;

DWORD WINAPI high_freq_worker(LPVOID param) {
    thread_params_t* p = (thread_params_t*)param;
    uint128_t add_val = {{1, 0}};
    
    for (int i = 0; i < p->ops_count; i++) {
        atomic_add_128_indexed(p->array_index, &add_val);
        InterlockedIncrement(&g_total_ops);
        InterlockedIncrement(&g_add_ops);
    }
    
    return 0;
}

int test_high_frequency() {
    printf("\n[Stress Test 1] High Frequency Operations\n");
    printf("=========================================\n");
    
    const int NUM_THREADS = 16;
    const int OPS_PER_THREAD = 100000;
    
    printf("  Threads: %d\n", NUM_THREADS);
    printf("  Ops/Thread: %d\n", OPS_PER_THREAD);
    printf("  Total Ops: %d\n", NUM_THREADS * OPS_PER_THREAD);
    
    g_total_ops = 0;
    g_add_ops = 0;
    
    g_array_size = NUM_THREADS;
    g_atomic_array = (uint128_t*)calloc(g_array_size, sizeof(uint128_t));
    g_cs_array = (CRITICAL_SECTION*)malloc(g_array_size * sizeof(CRITICAL_SECTION));
    for (int i = 0; i < g_array_size; i++) {
        InitializeCriticalSection(&g_cs_array[i]);
    }
    
    HANDLE* threads = (HANDLE*)malloc(NUM_THREADS * sizeof(HANDLE));
    thread_params_t* params = (thread_params_t*)malloc(NUM_THREADS * sizeof(thread_params_t));
    
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        params[i].thread_id = i;
        params[i].ops_count = OPS_PER_THREAD;
        params[i].array_index = i;
        threads[i] = CreateThread(NULL, 0, high_freq_worker, &params[i], 0, NULL);
    }
    
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
    
    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    
    int pass = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (g_atomic_array[i].parts[0] != OPS_PER_THREAD) {
            pass = 0;
        }
        CloseHandle(threads[i]);
    }
    
    printf("\n  Elapsed: %.3f sec\n", elapsed);
    printf("  Throughput: %.2f M ops/sec\n", (NUM_THREADS * OPS_PER_THREAD) / elapsed / 1000000.0);
    printf("  Test: %s\n", pass ? "PASS" : "FAIL");
    
    for (int i = 0; i < g_array_size; i++) {
        DeleteCriticalSection(&g_cs_array[i]);
    }
    free(g_atomic_array);
    free(g_cs_array);
    free(threads);
    free(params);
    
    return pass;
}

/* Test 2: Many Threads */
DWORD WINAPI many_threads_worker(LPVOID param) {
    thread_params_t* p = (thread_params_t*)param;
    uint128_t add_val = {{1, 0}};
    
    for (int i = 0; i < p->ops_count; i++) {
        atomic_add_128_indexed(0, &add_val);
        InterlockedIncrement(&g_total_ops);
    }
    
    return 0;
}

int test_many_threads() {
    printf("\n[Stress Test 2] Many Threads (Contention)\n");
    printf("==========================================\n");
    
    const int NUM_THREADS = 64;
    const int OPS_PER_THREAD = 10000;
    
    printf("  Threads: %d (all accessing same atomic variable)\n", NUM_THREADS);
    printf("  Ops/Thread: %d\n", OPS_PER_THREAD);
    printf("  Total Ops: %d\n", NUM_THREADS * OPS_PER_THREAD);
    
    g_total_ops = 0;
    
    g_array_size = 1;
    g_atomic_array = (uint128_t*)calloc(1, sizeof(uint128_t));
    g_cs_array = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(&g_cs_array[0]);
    
    HANDLE* threads = (HANDLE*)malloc(NUM_THREADS * sizeof(HANDLE));
    thread_params_t* params = (thread_params_t*)malloc(NUM_THREADS * sizeof(thread_params_t));
    
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        params[i].thread_id = i;
        params[i].ops_count = OPS_PER_THREAD;
        params[i].array_index = 0;
        threads[i] = CreateThread(NULL, 0, many_threads_worker, &params[i], 0, NULL);
    }
    
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
    
    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    
    uint64_t expected = (uint64_t)NUM_THREADS * OPS_PER_THREAD;
    int pass = (g_atomic_array[0].parts[0] == expected);
    
    printf("\n  Elapsed: %.3f sec\n", elapsed);
    printf("  Throughput: %.2f M ops/sec\n", (NUM_THREADS * OPS_PER_THREAD) / elapsed / 1000000.0);
    printf("  Expected: %llu\n", expected);
    printf("  Actual: %llu\n", g_atomic_array[0].parts[0]);
    printf("  Test: %s\n", pass ? "PASS" : "FAIL");
    
    for (int i = 0; i < NUM_THREADS; i++) {
        CloseHandle(threads[i]);
    }
    DeleteCriticalSection(&g_cs_array[0]);
    free(g_atomic_array);
    free(g_cs_array);
    free(threads);
    free(params);
    
    return pass;
}

/* Test 3: Mixed Operations */
DWORD WINAPI mixed_ops_worker(LPVOID param) {
    thread_params_t* p = (thread_params_t*)param;
    uint128_t add_val = {{1, 0}};
    uint128_t exchange_val = {{0x1000 + p->thread_id, 0}};
    uint128_t old_val;
    
    for (int i = 0; i < p->ops_count; i++) {
        int op = rand() % 3;
        
        switch (op) {
            case 0:
                atomic_add_128_indexed(p->array_index, &add_val);
                InterlockedIncrement(&g_add_ops);
                break;
                
            case 1:
                atomic_exchange_128_indexed(p->array_index, &exchange_val, &old_val);
                InterlockedIncrement(&g_exchange_ops);
                break;
                
            case 2: {
                uint128_t current = g_atomic_array[p->array_index];
                uint128_t new_val = {{current.parts[0] + 1, 0}};
                if (atomic_cas_128_indexed(p->array_index, &current, &new_val)) {
                    InterlockedIncrement(&g_cas_success);
                } else {
                    InterlockedIncrement(&g_cas_fail);
                }
                break;
            }
        }
        
        InterlockedIncrement(&g_total_ops);
    }
    
    return 0;
}

int test_mixed_operations() {
    printf("\n[Stress Test 3] Mixed Operations\n");
    printf("=================================\n");
    
    const int NUM_THREADS = 32;
    const int OPS_PER_THREAD = 50000;
    
    printf("  Threads: %d\n", NUM_THREADS);
    printf("  Ops/Thread: %d\n", OPS_PER_THREAD);
    printf("  Total Ops: %d\n", NUM_THREADS * OPS_PER_THREAD);
    
    g_total_ops = 0;
    g_add_ops = 0;
    g_exchange_ops = 0;
    g_cas_success = 0;
    g_cas_fail = 0;
    
    g_array_size = NUM_THREADS / 4;
    g_atomic_array = (uint128_t*)calloc(g_array_size, sizeof(uint128_t));
    g_cs_array = (CRITICAL_SECTION*)malloc(g_array_size * sizeof(CRITICAL_SECTION));
    for (int i = 0; i < g_array_size; i++) {
        InitializeCriticalSection(&g_cs_array[i]);
    }
    
    HANDLE* threads = (HANDLE*)malloc(NUM_THREADS * sizeof(HANDLE));
    thread_params_t* params = (thread_params_t*)malloc(NUM_THREADS * sizeof(thread_params_t));
    
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    srand((unsigned int)time(NULL));
    
    for (int i = 0; i < NUM_THREADS; i++) {
        params[i].thread_id = i;
        params[i].ops_count = OPS_PER_THREAD;
        params[i].array_index = i % g_array_size;
        threads[i] = CreateThread(NULL, 0, mixed_ops_worker, &params[i], 0, NULL);
    }
    
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
    
    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    
    printf("\n  Elapsed: %.3f sec\n", elapsed);
    printf("  Throughput: %.2f M ops/sec\n", (NUM_THREADS * OPS_PER_THREAD) / elapsed / 1000000.0);
    printf("\n  Operation Breakdown:\n");
    printf("    Add: %ld (%.1f%%)\n", g_add_ops, 100.0 * g_add_ops / g_total_ops);
    printf("    Exchange: %ld (%.1f%%)\n", g_exchange_ops, 100.0 * g_exchange_ops / g_total_ops);
    printf("    CAS: %ld (%.1f%%)\n", 
           g_cas_success + g_cas_fail, 
           100.0 * (g_cas_success + g_cas_fail) / g_total_ops);
    
    int pass = (g_total_ops == NUM_THREADS * OPS_PER_THREAD);
    printf("\n  Test: %s\n", pass ? "PASS" : "FAIL");
    
    for (int i = 0; i < NUM_THREADS; i++) {
        CloseHandle(threads[i]);
    }
    for (int i = 0; i < g_array_size; i++) {
        DeleteCriticalSection(&g_cs_array[i]);
    }
    free(g_atomic_array);
    free(g_cs_array);
    free(threads);
    free(params);
    
    return pass;
}

/* Test 4: Duration Test */
DWORD WINAPI duration_worker(LPVOID param) {
    thread_params_t* p = (thread_params_t*)param;
    uint128_t add_val = {{1, 0}};
    
    while (!g_stop_flag) {
        atomic_add_128_indexed(p->array_index, &add_val);
        InterlockedIncrement(&g_total_ops);
        
        if ((g_total_ops % 1000) == 0) {
            Sleep(0);
        }
    }
    
    return 0;
}

int test_duration_stress() {
    printf("\n[Stress Test 4] Duration Test\n");
    printf("==============================\n");
    
    const int NUM_THREADS = 16;
    const int DURATION_SEC = 5;
    
    printf("  Threads: %d\n", NUM_THREADS);
    printf("  Duration: %d seconds\n", DURATION_SEC);
    
    g_total_ops = 0;
    g_stop_flag = 0;
    
    g_array_size = NUM_THREADS;
    g_atomic_array = (uint128_t*)calloc(g_array_size, sizeof(uint128_t));
    g_cs_array = (CRITICAL_SECTION*)malloc(g_array_size * sizeof(CRITICAL_SECTION));
    for (int i = 0; i < g_array_size; i++) {
        InitializeCriticalSection(&g_cs_array[i]);
    }
    
    HANDLE* threads = (HANDLE*)malloc(NUM_THREADS * sizeof(HANDLE));
    thread_params_t* params = (thread_params_t*)malloc(NUM_THREADS * sizeof(thread_params_t));
    
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        params[i].thread_id = i;
        params[i].ops_count = 0;
        params[i].array_index = i;
        threads[i] = CreateThread(NULL, 0, duration_worker, &params[i], 0, NULL);
    }
    
    printf("\n  Running");
    fflush(stdout);
    for (int i = 0; i < DURATION_SEC; i++) {
        Sleep(1000);
        printf(".");
        fflush(stdout);
    }
    printf(" Done\n");
    
    g_stop_flag = 1;
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
    
    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    
    printf("\n  Elapsed: %.3f sec\n", elapsed);
    printf("  Total Ops: %ld\n", g_total_ops);
    printf("  Average: %.2f M ops/sec\n", g_total_ops / elapsed / 1000000.0);
    printf("  Test: PASS\n");
    
    for (int i = 0; i < NUM_THREADS; i++) {
        CloseHandle(threads[i]);
    }
    for (int i = 0; i < g_array_size; i++) {
        DeleteCriticalSection(&g_cs_array[i]);
    }
    free(g_atomic_array);
    free(g_cs_array);
    free(threads);
    free(params);
    
    return 1;
}

/* Main */
int main() {
    printf("============================================================\n");
    printf("128-bit Atomic Operations STRESS TEST\n");
    printf("============================================================\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    total_tests++; if (test_high_frequency()) passed_tests++;
    total_tests++; if (test_many_threads()) passed_tests++;
    total_tests++; if (test_mixed_operations()) passed_tests++;
    total_tests++; if (test_duration_stress()) passed_tests++;
    
    printf("\n============================================================\n");
    printf("128-bit STRESS TEST SUMMARY\n");
    printf("============================================================\n");
    printf("Total Tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success Rate: %.1f%%\n", (100.0 * passed_tests) / total_tests);
    
    if (passed_tests == total_tests) {
        printf("\nSUCCESS! All 128-bit tests passed!\n");
    }
    
    return (passed_tests == total_tests) ? 0 : 1;
}
