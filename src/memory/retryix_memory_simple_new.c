// retryix_memory_simple.c
// RetryIX v3.0.0 - Simplified memory management utilities
// Additional memory utilities (non-conflicting with retryix_api.c)

#include "retryix.h"
#include "retryix_svm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===== Memory Statistics =====
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t alloc_count;
    size_t free_count;
} memory_stats_t;

static memory_stats_t g_mem_stats = {0};

// ===== Memory Allocation (Additional utilities) =====
RETRYIX_API void* RETRYIX_CALL retryix_mem_alloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        g_mem_stats.total_allocated += size;
        g_mem_stats.current_usage += size;
        g_mem_stats.alloc_count++;
        
        if (g_mem_stats.current_usage > g_mem_stats.peak_usage) {
            g_mem_stats.peak_usage = g_mem_stats.current_usage;
        }
        
        printf("[RetryIX Memory] Allocated %zu bytes -> %p\n", size, ptr);
    } else {
        printf("[RetryIX Memory] Allocation failed! Requested %zu bytes\n", size);
    }
    
    return ptr;
}

RETRYIX_API void RETRYIX_CALL retryix_mem_free(void* ptr) {
    if (!ptr) return;
    
    g_mem_stats.free_count++;
    printf("[RetryIX Memory] Freed %p\n", ptr);
    free(ptr);
}

// ===== Memory Allocation (Extended API) =====
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_memory_alloc(
    void** ptr_out,
    size_t size,
    retryix_svm_level_t svm_level) {
    
    if (!ptr_out) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }
    
    (void)svm_level;  // Ignore SVM level in simulation mode
    
    *ptr_out = retryix_mem_alloc(size);
    return (*ptr_out) ? RETRYIX_SUCCESS : RETRYIX_ERROR_OUT_OF_MEMORY;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_memory_free(void* ptr) {
    retryix_mem_free(ptr);
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_memory_cleanup(void) {
    printf("[RetryIX Memory] Memory system cleanup\n");
    printf("  Total allocated: %zu bytes (%zu times)\n", g_mem_stats.total_allocated, g_mem_stats.alloc_count);
    printf("  Total freed: %zu times\n", g_mem_stats.free_count);
    printf("  Peak usage: %zu bytes\n", g_mem_stats.peak_usage);
    
    if (g_mem_stats.alloc_count != g_mem_stats.free_count) {
        printf("  WARNING: Possible memory leak (%zu allocs, %zu frees)\n",
               g_mem_stats.alloc_count, g_mem_stats.free_count);
    }
    
    return RETRYIX_SUCCESS;
}

// ===== NUMA Related (Simulated) =====
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_mem_bind_numa(void* ptr, int node_id) {
    if (!ptr) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }
    
    printf("[RetryIX Memory] NUMA bind %p -> node %d (simulated)\n", ptr, node_id);
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_mem_prefetch(void* ptr, size_t size, int device_id) {
    if (!ptr) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }
    
    printf("[RetryIX Memory] Prefetch %zu bytes (%p) to device %d (simulated)\n", size, ptr, device_id);
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_mem_advise(void* ptr, size_t size, int advice) {
    if (!ptr) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }
    
    const char* advice_str = "UNKNOWN";
    switch (advice) {
        case 0: advice_str = "READ_MOSTLY"; break;
        case 1: advice_str = "PREFERRED_LOCATION"; break;
        case 2: advice_str = "ACCESSED_BY"; break;
    }
    
    printf("[RetryIX Memory] Advise %s for %zu bytes (%p) (simulated)\n", advice_str, size, ptr);
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_mem_query_location(void* ptr, int* location_out) {
    if (!ptr || !location_out) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }
    
    *location_out = 0;  // Simulated: always on host
    printf("[RetryIX Memory] Query location %p -> host (simulated)\n", ptr);
    return RETRYIX_SUCCESS;
}

// ===== Statistics & Validation =====
RETRYIX_API void RETRYIX_CALL retryix_memory_print_stats(void) {
    printf("\n[RetryIX Memory] Memory Statistics\n");
    printf("==========================================\n");
    printf("Total allocated: %10zu bytes (%zu times)\n", g_mem_stats.total_allocated, g_mem_stats.alloc_count);
    printf("Total freed:     %10zu times\n", g_mem_stats.free_count);
    printf("Current usage:   %10zu bytes\n", g_mem_stats.current_usage);
    printf("Peak usage:      %10zu bytes\n", g_mem_stats.peak_usage);
    printf("==========================================\n");
    
    if (g_mem_stats.alloc_count != g_mem_stats.free_count) {
        printf("WARNING: Potential leak: %zu unfreed allocations\n", 
               g_mem_stats.alloc_count - g_mem_stats.free_count);
    } else {
        printf("OK: No memory leaks\n");
    }
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_memory_validate(void) {
    printf("[RetryIX Memory] Validating memory state...\n");
    
    if (g_mem_stats.alloc_count != g_mem_stats.free_count) {
        printf("  FAILED: Memory leak detected: %zu unfreed allocations\n",
               g_mem_stats.alloc_count - g_mem_stats.free_count);
        return RETRYIX_ERROR_UNKNOWN;
    }
    
    printf("  PASS: Memory validation successful\n");
    return RETRYIX_SUCCESS;
}
