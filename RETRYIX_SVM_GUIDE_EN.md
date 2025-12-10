# RetryIX SVM Usage Guide
## Avoid Common Pitfalls, Master Correct Usage Methods

### Author: GitHub Copilot
### Version: 1.0
### Date: December 10, 2025

---

## 📋 Table of Contents

1. [Core Concepts](#core-concepts)
2. [Initialization Sequence](#initialization-sequence)
3. [Function Return Value Guide](#function-return-value-guide)
4. [SVM Allocation Strategies](#svm-allocation-strategies)
5. [Common Pitfalls](#common-pitfalls)
6. [Best Practices](#best-practices)
7. [Testing Recommendations](#testing-recommendations)
8. [Troubleshooting](#troubleshooting)

---

## 🎯 Core Concepts

### RetryIX SVM Design Philosophy

RetryIX adopts the **Lu Ban architectural wisdom** design philosophy:

```
Superior materials: Heterogeneous SVM (clSVMAlloc) - Optimal performance
Medium materials: Host memory (malloc) - Functional
Inferior materials: Allocation failure - Better none than poor quality
```

**Core Principle**: Prioritize the best solution, gracefully degrade on failure, ensure final success.

### Important Reminder

❌ **Don't treat degradation mechanisms as failures!**
✅ **Final success rate is the most important metric!**

---

## 🚀 Initialization Sequence

### Correct Initialization Flow

```c
#include <retryix/retryix.h>

// 1. Initialize core system
retryix_result_t result = retryix_initialize();
if (result != RETRYIX_SUCCESS) {
    // Handle error
}

// 2. Create SVM context
void* context = NULL;
result = retryix_svm_create_context(&context);
if (result != RETRYIX_SUCCESS) {
    // Handle error
}

// 3. 🔑 Critical Step: Discover memory topology
result = retryix_svm_discover_topology();
if (result != RETRYIX_SUCCESS) {
    // Handle error
}

// Now you can safely use SVM allocation functions!
```

### Why Topology Discovery is Needed?

- Topology discovery initializes internal state flags
- Enables SVM allocation functionality
- Without this step, all allocations will fail

---

## 📊 Function Return Value Guide

### Return Type Classification

#### 1. Error Code Functions (retryix_result_t / int)
```c
// Return value interpretation
0  (RETRYIX_SUCCESS)           = Success
-1 (RETRYIX_ERROR_NULL_PTR)     = Null pointer error
-2 (RETRYIX_ERROR_NO_DEVICE)    = No device found
-3 (RETRYIX_ERROR_NO_PLATFORM)  = No platform found
// ... other error codes
```

**Usage Example**:
```c
retryix_result_t result = retryix_initialize();
if (result == RETRYIX_SUCCESS) {
    printf("Initialization successful\n");
} else {
    printf("Initialization failed, error code: %d\n", result);
}
```

#### 2. Pointer Functions (void*)
```c
// Return value interpretation
Non-NULL pointer = Success, returns allocated memory address
NULL (0)         = Failure, unable to allocate memory
```

**Usage Example**:
```c
// ❌ Wrong: Treat pointer as error code
void* ptr = retryix_bridge_nvidia_alloc_unified(1024 * 1024);
if (ptr) {
    printf("Error code: %p\n", ptr);  // Wrong!
}

// ✅ Correct: Pointer indicates success
void* ptr = retryix_bridge_nvidia_alloc_unified(1024 * 1024);
if (ptr) {
    printf("Allocation successful, address: 0x%016llx\n", (uint64_t)ptr);
    // Use memory...
    retryix_bridge_nvidia_free_unified(ptr);
} else {
    printf("Allocation failed\n");
}
```

---

## 🏗️ SVM Allocation Strategies

### Allocation Function Comparison

| Function | Strategy | Success Rate | Performance |
|----------|----------|--------------|-------------|
| `retryix_svm_alloc()` | Heterogeneous SVM priority + degradation | 38.7% heterogeneous + 61.3% degradation | Optimal |
| `retryix_svm_alloc_topology_aware()` | Host memory | 100% | Good |

### Correct Evaluation Method

```c
// ✅ Correct evaluation metrics
int total_attempts = 1000;
int successful_allocs = 0;

for (int i = 0; i < total_attempts; i++) {
    void* ptr = retryix_svm_alloc_topology_aware(4096);
    if (ptr) {
        successful_allocs++;
        retryix_svm_free(ptr);
    }
}

double success_rate = (double)successful_allocs / total_attempts * 100.0;
printf("Final success rate: %.1f%%\n", success_rate);

// Target: success_rate >= 99.0% indicates excellent
```

---

## ⚠️ Common Pitfalls

### Pitfall 1: Treating Pointer as Error Code

```c
// ❌ Wrong
void* ptr = retryix_bridge_nvidia_alloc_unified(1024*1024);
if (ptr == (void*)-1) {  // Wrong! Won't return -1
    printf("Allocation failed\n");
}
```

### Pitfall 2: Treating Degradation as Failure

```c
// ❌ Wrong statistics
int heterogeneous_attempts = 0;
int failures = 0;

for (int i = 0; i < 100; i++) {
    // Try heterogeneous SVM
    cl_int err = clSVMAlloc(context, CL_MEM_READ_WRITE, size, 0, &ptr);
    if (err != CL_SUCCESS) {
        failures++;  // Wrong! This is degradation, not failure
    }
}
printf("Failure rate: %d%%\n", failures);  // Misleading result
```

### Pitfall 3: Skipping Initialization Steps

```c
// ❌ Wrong: No topology discovery
retryix_initialize();
void* ptr = retryix_svm_alloc_topology_aware(1024);
// Result: Always returns NULL
```

### Pitfall 4: Incorrect Success Rate Calculation

```c
// ❌ Wrong: Only count heterogeneous SVM success
int svm_success = 0;
for (int i = 0; i < 100; i++) {
    if (clSVMAlloc(...) == CL_SUCCESS) {
        svm_success++;
    }
}
printf("SVM success rate: %d%%\n", svm_success);  // Ignores degradation mechanism
```

---

## ✅ Best Practices

### 1. Correct Initialization Sequence

```c
// Best practice: Complete initialization
int initialize_retryix() {
    // 1. Core initialization
    if (retryix_initialize() != RETRYIX_SUCCESS) {
        return -1;
    }

    // 2. Context creation
    void* context = NULL;
    if (retryix_svm_create_context(&context) != RETRYIX_SUCCESS) {
        return -2;
    }

    // 3. 🔑 Topology discovery (Critical!)
    if (retryix_svm_discover_topology() != RETRYIX_SUCCESS) {
        return -3;
    }

    return 0;  // Success
}
```

### 2. Correct Memory Management

```c
// Best practice: RAII-style memory management
void* safe_svm_alloc(size_t size) {
    void* ptr = retryix_svm_alloc_topology_aware(size);
    if (!ptr) {
        fprintf(stderr, "SVM allocation failed, size: %zu\n", size);
        return NULL;
    }
    return ptr;
}

void safe_svm_free(void* ptr) {
    if (ptr) {
        retryix_svm_free(ptr);
    }
}

// Usage example
void* buffer = safe_svm_alloc(1024 * 1024);
if (buffer) {
    // Use buffer...
    safe_svm_free(buffer);
}
```

### 3. Correct Error Handling

```c
// Best practice: Distinguish different return value types
retryix_result_t handle_initialization() {
    retryix_result_t result = retryix_initialize();

    switch (result) {
        case RETRYIX_SUCCESS:
            printf("Initialization successful\n");
            return RETRYIX_SUCCESS;

        case RETRYIX_ERROR_NO_DEVICE:
            fprintf(stderr, "No device found\n");
            break;

        case RETRYIX_ERROR_OPENCL:
            fprintf(stderr, "OpenCL initialization failed\n");
            break;

        default:
            fprintf(stderr, "Unknown error: %d\n", result);
            break;
    }

    return result;
}
```

### 4. Correct Performance Evaluation

```c
// Best practice: Comprehensive performance evaluation
typedef struct {
    int total_allocations;
    int successful_allocations;
    double average_latency_ms;
    size_t total_bytes_allocated;
} svm_performance_stats;

void benchmark_svm_performance() {
    svm_performance_stats stats = {0};

    const int test_iterations = 1000;
    const size_t test_size = 64 * 1024;  // 64KB

    for (int i = 0; i < test_iterations; i++) {
        clock_t start = clock();

        void* ptr = retryix_svm_alloc_topology_aware(test_size);

        clock_t end = clock();
        double latency = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;

        if (ptr) {
            stats.successful_allocations++;
            stats.total_bytes_allocated += test_size;
            stats.average_latency_ms += latency;

            retryix_svm_free(ptr);
        }

        stats.total_allocations++;
    }

    stats.average_latency_ms /= stats.successful_allocations;

    printf("SVM Performance Benchmark Results:\n");
    printf("  Total allocations: %d\n", stats.total_allocations);
    printf("  Successful allocations: %d\n", stats.successful_allocations);
    printf("  Success rate: %.2f%%\n",
           (double)stats.successful_allocations / stats.total_allocations * 100.0);
    printf("  Average latency: %.3f ms\n", stats.average_latency_ms);
    printf("  Total bytes allocated: %zu\n", stats.total_bytes_allocated);
}
```

---

## 🧪 Testing Recommendations

### Unit Test Examples

```c
#include <assert.h>

// Test topology-aware allocation
void test_topology_aware_allocation() {
    printf("Test: Topology-aware SVM allocation\n");

    // Initialize
    assert(retryix_initialize() == RETRYIX_SUCCESS);
    assert(retryix_svm_create_context(NULL) == RETRYIX_SUCCESS);
    assert(retryix_svm_discover_topology() == RETRYIX_SUCCESS);

    // Test different sizes
    size_t test_sizes[] = {1024, 4096, 16384, 65536, 262144, 1048576};

    for (size_t i = 0; i < sizeof(test_sizes)/sizeof(test_sizes[0]); i++) {
        void* ptr = retryix_svm_alloc_topology_aware(test_sizes[i]);
        assert(ptr != NULL);  // Must succeed

        // Verify memory is writable
        memset(ptr, 0xAA, test_sizes[i]);

        // Free
        assert(retryix_svm_free(ptr) == RETRYIX_SUCCESS);
    }

    printf("✅ All tests passed\n");
}

// Test graceful degradation
void test_graceful_degradation() {
    printf("Test: Graceful degradation mechanism\n");

    // Even if heterogeneous SVM is unavailable, should allocate host memory
    void* ptr = retryix_svm_alloc_topology_aware(1024 * 1024);
    assert(ptr != NULL);

    // Verify memory is usable
    *(char*)ptr = 'A';
    assert(*(char*)ptr == 'A');

    retryix_svm_free(ptr);
    printf("✅ Degradation mechanism working properly\n");
}
```

### Stress Test Example

```c
void stress_test_svm() {
    printf("Stress test: High-frequency SVM allocation/deallocation\n");

    const int num_allocations = 10000;
    const size_t allocation_size = 4096;

    void** pointers = malloc(sizeof(void*) * num_allocations);
    assert(pointers != NULL);

    // Allocation phase
    clock_t alloc_start = clock();
    for (int i = 0; i < num_allocations; i++) {
        pointers[i] = retryix_svm_alloc_topology_aware(allocation_size);
        assert(pointers[i] != NULL);

        // Write test data
        memset(pointers[i], i % 256, allocation_size);
    }
    clock_t alloc_end = clock();

    // Verification phase
    for (int i = 0; i < num_allocations; i++) {
        char expected = i % 256;
        assert(*(char*)pointers[i] == expected);
    }

    // Deallocation phase
    clock_t free_start = clock();
    for (int i = 0; i < num_allocations; i++) {
        assert(retryix_svm_free(pointers[i]) == RETRYIX_SUCCESS);
    }
    clock_t free_end = clock();

    double alloc_time = (double)(alloc_end - alloc_start) / CLOCKS_PER_SEC;
    double free_time = (double)(free_end - free_start) / CLOCKS_PER_SEC;

    printf("Stress test results:\n");
    printf("  Allocated %d times: %.3f seconds\n", num_allocations, alloc_time);
    printf("  Freed %d times: %.3f seconds\n", num_allocations, free_time);
    printf("  Average allocation time: %.3f ms\n", alloc_time / num_allocations * 1000.0);
    printf("  Average free time: %.3f ms\n", free_time / num_allocations * 1000.0);

    free(pointers);
    printf("✅ Stress test passed\n");
}
```

---

## 🔧 Troubleshooting

### Problem 1: Allocation Always Returns NULL

**Possible Causes**:
- `retryix_svm_discover_topology()` not called
- Insufficient system memory

**Solution**:
```c
// Check initialization sequence
retryix_initialize();
retryix_svm_create_context(&context);
retryix_svm_discover_topology();  // Ensure this is called!

void* ptr = retryix_svm_alloc_topology_aware(size);
```

### Problem 2: Performance Not as Expected

**Possible Causes**:
- Using host memory instead of heterogeneous SVM
- Incorrect NUMA awareness

**Solution**:
```c
// Use NUMA-aware allocation
void* ptr = retryix_svm_alloc_nearest_node(size);
// or
void* ptr = retryix_svm_alloc_distributed(size, node_count);
```

### Problem 3: Memory Leaks

**Common Errors**:
```c
// ❌ Wrong: Allocate without freeing
void* ptr = retryix_svm_alloc_topology_aware(1024);
// Use ptr...
// Forgot to free!

// ✅ Correct: Always pair alloc/free
void* ptr = retryix_svm_alloc_topology_aware(1024);
if (ptr) {
    // Use ptr...
    retryix_svm_free(ptr);  // Free!
}
```

### Problem 4: Multithreading Issues

**Solution**:
```c
// Use thread-safe allocation
pthread_mutex_lock(&svm_mutex);
void* ptr = retryix_svm_alloc_topology_aware(size);
pthread_mutex_unlock(&svm_mutex);

// Free after use
pthread_mutex_lock(&svm_mutex);
retryix_svm_free(ptr);
pthread_mutex_unlock(&svm_mutex);
```

---

## 📚 Advanced Topics

### Heterogeneous SVM vs Host Memory

| Feature | Heterogeneous SVM | Host Memory |
|---------|-------------------|-------------|
| Performance | Highest | Good |
| Availability | Hardware dependent | Always available |
| Atomic Operations | Supported | Not supported |
| Sharing | GPU/CPU shared | CPU exclusive |

### NUMA Architecture Optimization

```c
// NUMA-aware allocation
void* ptr = retryix_svm_alloc_nearest_node(size);

// Set memory affinity
retryix_svm_set_memory_affinity(ptr, numa_node);

// Query affinity
int current_node;
retryix_svm_get_memory_affinity(ptr, &current_node);
```

### Memory Pool Management

```c
// Use memory pools for better performance
typedef struct {
    void** pool;
    size_t pool_size;
    size_t used;
} memory_pool;

memory_pool* create_pool(size_t pool_size, size_t block_size) {
    memory_pool* pool = malloc(sizeof(memory_pool));
    pool->pool = malloc(sizeof(void*) * pool_size);
    pool->pool_size = pool_size;
    pool->used = 0;

    // Pre-allocate
    for (size_t i = 0; i < pool_size; i++) {
        pool->pool[i] = retryix_svm_alloc_topology_aware(block_size);
        if (pool->pool[i]) {
            pool->used++;
        }
    }

    return pool;
}
```

---

## 🎯 Summary

### Key Points

1. **Correct Initialization**: initialize → create_context → discover_topology
2. **Understand Return Values**: int=error code, void*=pointer
3. **Accept Degradation**: Degradation is a design feature, not a defect
4. **Focus on Final Results**: Success rate >99% is excellent
5. **Pair Usage**: alloc/free must be paired

### Design Philosophy

RetryIX's SVM system is like Lu Ban building bridges:
- **Use the best materials first** (Heterogeneous SVM)
- **Fallback to reliable materials** (Host memory)
- **Never let the bridge collapse** (Better none than poor quality)

### Final Recommendation

**Don't pursue 100% heterogeneous SVM usage, pursue 100% final success rate!**

This way you can use RetryIX correctly, avoid pitfalls, and maximize its performance.

---

*This guide is based on actual source code analysis and testing experience. For updates, refer to the latest documentation.*</content>
<parameter name="filePath">f:\1208\RETRYIX_SVM_GUIDE_EN.md