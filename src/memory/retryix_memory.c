#define CL_TARGET_OPENCL_VERSION 200

#include "retryix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
    #include <malloc.h>
    #define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
    #define aligned_free(ptr) _aligned_free(ptr)
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define aligned_free(ptr) free(ptr)
    #define DLL_EXPORT __attribute__((visibility("default")))
#endif

// Forward declaration for unmap function
int retryix_memory_unmap(void* ptr, cl_command_queue queue);

// 記憶體類型定義
typedef enum {
    RETRYIX_MEM_READ_ONLY = 0x1,
    RETRYIX_MEM_WRITE_ONLY = 0x2,
    RETRYIX_MEM_READ_WRITE = 0x4,
    RETRYIX_MEM_HOST_PTR = 0x8,
    RETRYIX_MEM_ALLOC_HOST_PTR = 0x10,
    RETRYIX_MEM_COPY_HOST_PTR = 0x20,
    RETRYIX_MEM_PERSISTENT = 0x40,
    RETRYIX_MEM_ZERO_COPY = 0x80
} retryix_memory_flags_t;

// 記憶體描述符
typedef struct {
    void* host_ptr;                 // 主機端指針
    cl_mem device_mem;              // 設備端記憶體對象
    size_t size;                    // 記憶體大小
    retryix_memory_flags_t flags;   // 記憶體標誌
    cl_context context;             // 關聯上下文
    cl_device_id device;            // 關聯設備
    bool is_mapped;                 // 是否已映射
    void* mapped_ptr;               // 映射指針
    uint32_t ref_count;             // 引用計數
    char debug_name[64];            // 調試名稱
} retryix_memory_descriptor_t;

// 記憶體管理上下文
typedef struct {
    cl_context context;
    cl_device_id device;
    
    // 記憶體池
    retryix_memory_descriptor_t* descriptors;
    size_t descriptor_count;
    size_t descriptor_capacity;
    
    // 對齊要求
    size_t base_alignment;
    size_t preferred_alignment;
    
    // 統計信息
    size_t total_allocated;
    size_t peak_allocated;
    size_t host_allocated;
    size_t device_allocated;
    uint64_t alloc_count;
    uint64_t free_count;
    uint64_t transfer_count;
    size_t total_transferred;
    
    // 性能統計
    double total_transfer_time;
    double peak_bandwidth;
} retryix_memory_context_t;

// 全局記憶體管理器
static retryix_memory_context_t* g_memory_context = NULL;

// === 內部函數 ===

// 初始化記憶體管理器
retryix_memory_context_t* retryix_memory_init(cl_context context, cl_device_id device) {
    if (g_memory_context) {
        return g_memory_context; // 已初始化
    }
    
    retryix_memory_context_t* ctx = (retryix_memory_context_t*)calloc(1, sizeof(retryix_memory_context_t));
    if (!ctx) return NULL;
    
    ctx->context = context;
    ctx->device = device;
    
    // 查詢設備記憶體對齊要求
    size_t base_align = 0;
    clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(base_align), &base_align, NULL);
    ctx->base_alignment = (base_align > 0) ? (base_align / 8) : 64;
    ctx->preferred_alignment = 256; // 256位元組對齊通常性能最佳
    
    // 初始化描述符池
    ctx->descriptor_capacity = 128;
    ctx->descriptors = (retryix_memory_descriptor_t*)calloc(ctx->descriptor_capacity, 
                                                           sizeof(retryix_memory_descriptor_t));
    if (!ctx->descriptors) {
        free(ctx);
        return NULL;
    }
    
    g_memory_context = ctx;
    
    printf("RetryIX Memory Manager Initialized\n");
    printf("  Base Alignment: %zu bytes\n", ctx->base_alignment);
    printf("  Preferred Alignment: %zu bytes\n", ctx->preferred_alignment);
    
    return ctx;
}

// 查找記憶體描述符
static retryix_memory_descriptor_t* find_memory_descriptor(void* ptr) {
    if (!g_memory_context || !ptr) return NULL;
    
    for (size_t i = 0; i < g_memory_context->descriptor_count; i++) {
        if (g_memory_context->descriptors[i].host_ptr == ptr) {
            return &g_memory_context->descriptors[i];
        }
    }
    return NULL;
}

// 添加記憶體描述符
static int add_memory_descriptor(retryix_memory_descriptor_t* desc) {
    if (!g_memory_context) return -1;
    
    // 擴展容量
    if (g_memory_context->descriptor_count >= g_memory_context->descriptor_capacity) {
        g_memory_context->descriptor_capacity *= 2;
        g_memory_context->descriptors = (retryix_memory_descriptor_t*)realloc(
            g_memory_context->descriptors,
            g_memory_context->descriptor_capacity * sizeof(retryix_memory_descriptor_t));
        if (!g_memory_context->descriptors) return -1;
    }
    
    g_memory_context->descriptors[g_memory_context->descriptor_count++] = *desc;
    return 0;
}

// === 公開 API ===

// 通用記憶體分配
DLL_EXPORT void* retryix_memory_alloc(size_t size, retryix_memory_flags_t flags, const char* debug_name) {
    if (!g_memory_context || size == 0) return NULL;
    
    // 對齊大小
    size_t alignment = (flags & RETRYIX_MEM_ZERO_COPY) ? g_memory_context->preferred_alignment : g_memory_context->base_alignment;
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    void* host_ptr = NULL;
    cl_mem device_mem = NULL;
    cl_int err = CL_SUCCESS;
    
    // 根據標誌選擇分配策略
    if (flags & RETRYIX_MEM_HOST_PTR) {
        // 使用用戶提供的主機記憶體
        printf("ERROR: RETRYIX_MEM_HOST_PTR requires external pointer\n");
        return NULL;
    } else if (flags & RETRYIX_MEM_ZERO_COPY) {
        // 零拷貝記憶體（優先使用 SVM 或 pinned memory）
        host_ptr = aligned_alloc(alignment, aligned_size);
        if (host_ptr) {
            // 嘗試創建零拷貝 OpenCL 緩衝區
            cl_mem_flags cl_flags = CL_MEM_USE_HOST_PTR;
            if (flags & RETRYIX_MEM_READ_ONLY) cl_flags |= CL_MEM_READ_ONLY;
            else if (flags & RETRYIX_MEM_WRITE_ONLY) cl_flags |= CL_MEM_WRITE_ONLY;
            else cl_flags |= CL_MEM_READ_WRITE;
            
            device_mem = clCreateBuffer(g_memory_context->context, cl_flags, aligned_size, host_ptr, &err);
            if (err != CL_SUCCESS) {
                aligned_free(host_ptr);
                return NULL;
            }
            printf("Zero-copy allocation: %p (%zu bytes)\n", host_ptr, aligned_size);
        }
    } else {
        // 標準記憶體分配
        host_ptr = aligned_alloc(alignment, aligned_size);
        if (host_ptr) {
            cl_mem_flags cl_flags = 0;
            if (flags & RETRYIX_MEM_READ_ONLY) cl_flags = CL_MEM_READ_ONLY;
            else if (flags & RETRYIX_MEM_WRITE_ONLY) cl_flags = CL_MEM_WRITE_ONLY;
            else cl_flags = CL_MEM_READ_WRITE;
            
            if (flags & RETRYIX_MEM_COPY_HOST_PTR) {
                cl_flags |= CL_MEM_COPY_HOST_PTR;
                device_mem = clCreateBuffer(g_memory_context->context, cl_flags, aligned_size, host_ptr, &err);
            } else {
                device_mem = clCreateBuffer(g_memory_context->context, cl_flags, aligned_size, NULL, &err);
            }
            
            if (err != CL_SUCCESS) {
                aligned_free(host_ptr);
                return NULL;
            }
            printf("Standard allocation: %p (%zu bytes)\n", host_ptr, aligned_size);
        }
    }
    
    if (host_ptr && device_mem) {
        // 創建記憶體描述符
        retryix_memory_descriptor_t desc = {0};
        desc.host_ptr = host_ptr;
        desc.device_mem = device_mem;
        desc.size = aligned_size;
        desc.flags = flags;
        desc.context = g_memory_context->context;
        desc.device = g_memory_context->device;
        desc.is_mapped = false;
        desc.mapped_ptr = NULL;
        desc.ref_count = 1;
        
        if (debug_name) {
            strncpy(desc.debug_name, debug_name, sizeof(desc.debug_name) - 1);
        } else {
            snprintf(desc.debug_name, sizeof(desc.debug_name), "mem_%p", host_ptr);
        }
        
        if (add_memory_descriptor(&desc) == 0) {
            // 更新統計
            g_memory_context->total_allocated += aligned_size;
            g_memory_context->host_allocated += aligned_size;
            g_memory_context->alloc_count++;
            
            if (g_memory_context->total_allocated > g_memory_context->peak_allocated) {
                g_memory_context->peak_allocated = g_memory_context->total_allocated;
            }
            
            return host_ptr;
        } else {
            // 清理失敗的分配
            clReleaseMemObject(device_mem);
            aligned_free(host_ptr);
        }
    }
    
    return NULL;
}

// 記憶體釋放
DLL_EXPORT int retryix_memory_free(void* ptr) {
    if (!g_memory_context || !ptr) return -1;
    
    retryix_memory_descriptor_t* desc = find_memory_descriptor(ptr);
    if (!desc) return -1;
    
    // 減少引用計數
    if (--desc->ref_count > 0) {
        return 0; // 仍有其他引用
    }
    
    // 如果記憶體已映射，先解映射
    if (desc->is_mapped && desc->mapped_ptr) {
        retryix_memory_unmap(ptr, NULL);
    }
    
    // 釋放 OpenCL 記憶體對象
    if (desc->device_mem) {
        clReleaseMemObject(desc->device_mem);
    }
    
    // 釋放主機記憶體
    if (desc->host_ptr) {
        aligned_free(desc->host_ptr);
        printf("Memory freed: %s (%zu bytes)\n", desc->debug_name, desc->size);
    }
    
    // 更新統計
    g_memory_context->total_allocated -= desc->size;
    g_memory_context->host_allocated -= desc->size;
    g_memory_context->free_count++;
    
    // 從描述符陣列中移除
    for (size_t i = 0; i < g_memory_context->descriptor_count; i++) {
        if (&g_memory_context->descriptors[i] == desc) {
            g_memory_context->descriptors[i] = g_memory_context->descriptors[--g_memory_context->descriptor_count];
            break;
        }
    }
    
    return 0;
}

// 記憶體映射
void* retryix_memory_map(void* ptr, cl_command_queue queue, retryix_memory_flags_t map_flags) {
    if (!g_memory_context || !ptr || !queue) return NULL;
    
    retryix_memory_descriptor_t* desc = find_memory_descriptor(ptr);
    if (!desc || !desc->device_mem) return NULL;
    
    if (desc->is_mapped) {
        return desc->mapped_ptr; // 已映射
    }
    
    // 轉換映射標誌
    cl_map_flags cl_flags = 0;
    if (map_flags & RETRYIX_MEM_READ_ONLY) cl_flags = CL_MAP_READ;
    else if (map_flags & RETRYIX_MEM_WRITE_ONLY) cl_flags = CL_MAP_WRITE;
    else cl_flags = CL_MAP_READ | CL_MAP_WRITE;
    
    cl_int err;
    void* mapped = clEnqueueMapBuffer(queue, desc->device_mem, CL_TRUE, cl_flags, 0, desc->size, 0, NULL, NULL, &err);
    
    if (err == CL_SUCCESS && mapped) {
        desc->is_mapped = true;
        desc->mapped_ptr = mapped;
        printf("Memory mapped: %s -> %p\n", desc->debug_name, mapped);
        return mapped;
    }
    
    return NULL;
}

// 記憶體解映射
int retryix_memory_unmap(void* ptr, cl_command_queue queue) {
    if (!g_memory_context || !ptr) return -1;
    
    retryix_memory_descriptor_t* desc = find_memory_descriptor(ptr);
    if (!desc || !desc->is_mapped) return -1;
    
    if (queue && desc->device_mem && desc->mapped_ptr) {
        cl_int err = clEnqueueUnmapMemObject(queue, desc->device_mem, desc->mapped_ptr, 0, NULL, NULL);
        if (err == CL_SUCCESS) {
            desc->is_mapped = false;
            desc->mapped_ptr = NULL;
            printf("Memory unmapped: %s\n", desc->debug_name);
            return 0;
        }
    }
    
    return -1;
}

// 記憶體拷貝（主機到設備）
int retryix_memory_copy_to_device(void* host_ptr, cl_command_queue queue, bool blocking) {
    if (!g_memory_context || !host_ptr || !queue) return -1;
    
    retryix_memory_descriptor_t* desc = find_memory_descriptor(host_ptr);
    if (!desc || !desc->device_mem) return -1;
    
    // 如果是零拷貝記憶體，不需要拷貝
    if (desc->flags & RETRYIX_MEM_ZERO_COPY) {
        return 0; // 成功但無操作
    }
    
    cl_int err = clEnqueueWriteBuffer(queue, desc->device_mem, blocking ? CL_TRUE : CL_FALSE,
                                     0, desc->size, desc->host_ptr, 0, NULL, NULL);
    
    if (err == CL_SUCCESS) {
        g_memory_context->transfer_count++;
        g_memory_context->total_transferred += desc->size;
        printf("Host->Device: %s (%zu bytes)\n", desc->debug_name, desc->size);
        return 0;
    }
    
    return -1;
}

// 記憶體拷貝（設備到主機）
int retryix_memory_copy_from_device(void* host_ptr, cl_command_queue queue, bool blocking) {
    if (!g_memory_context || !host_ptr || !queue) return -1;
    
    retryix_memory_descriptor_t* desc = find_memory_descriptor(host_ptr);
    if (!desc || !desc->device_mem) return -1;
    
    // 如果是零拷貝記憶體，不需要拷貝
    if (desc->flags & RETRYIX_MEM_ZERO_COPY) {
        return 0; // 成功但無操作
    }
    
    cl_int err = clEnqueueReadBuffer(queue, desc->device_mem, blocking ? CL_TRUE : CL_FALSE,
                                    0, desc->size, desc->host_ptr, 0, NULL, NULL);
    
    if (err == CL_SUCCESS) {
        g_memory_context->transfer_count++;
        g_memory_context->total_transferred += desc->size;
        printf("Device->Host: %s (%zu bytes)\n", desc->debug_name, desc->size);
        return 0;
    }
    
    return -1;
}

// 取得設備記憶體對象
cl_mem retryix_memory_get_device_mem(void* host_ptr) {
    if (!g_memory_context || !host_ptr) return NULL;
    
    retryix_memory_descriptor_t* desc = find_memory_descriptor(host_ptr);
    return desc ? desc->device_mem : NULL;
}

// 記憶體統計報告
DLL_EXPORT void retryix_memory_print_stats(void) {
    if (!g_memory_context) {
        printf("Memory manager not initialized\n");
        return;
    }
    
    printf("\n=== RetryIX Memory Statistics ===\n");
    printf("Total Allocations: %llu\n", (unsigned long long)g_memory_context->alloc_count);
    printf("Total Frees: %llu\n", (unsigned long long)g_memory_context->free_count);
    printf("Active Allocations: %zu\n", g_memory_context->descriptor_count);
    printf("Current Allocated: %.2f MB\n", (double)g_memory_context->total_allocated / (1024*1024));
    printf("Peak Allocated: %.2f MB\n", (double)g_memory_context->peak_allocated / (1024*1024));
    printf("Total Transfers: %llu\n", (unsigned long long)g_memory_context->transfer_count);
    printf("Total Transferred: %.2f MB\n", (double)g_memory_context->total_transferred / (1024*1024));
    
    if (g_memory_context->total_transfer_time > 0) {
        double bandwidth = (g_memory_context->total_transferred / (1024*1024)) / g_memory_context->total_transfer_time;
        printf("Average Bandwidth: %.2f MB/s\n", bandwidth);
        printf("Peak Bandwidth: %.2f MB/s\n", g_memory_context->peak_bandwidth);
    }
    
    printf("\nActive Memory Blocks:\n");
    for (size_t i = 0; i < g_memory_context->descriptor_count; i++) {
        retryix_memory_descriptor_t* desc = &g_memory_context->descriptors[i];
        printf("  %s: %p (%zu bytes, refs=%u, mapped=%s)\n",
               desc->debug_name, desc->host_ptr, desc->size, desc->ref_count,
               desc->is_mapped ? "YES" : "NO");
    }
    printf("===================================\n\n");
}

// 清理記憶體管理器
DLL_EXPORT void retryix_memory_cleanup(void) {
    if (!g_memory_context) return;
    
    printf("RetryIX Memory Manager Cleanup\n");
    
    // 釋放所有未釋放的記憶體
    while (g_memory_context->descriptor_count > 0) {
        retryix_memory_free(g_memory_context->descriptors[0].host_ptr);
    }
    
    // 打印最終統計
    retryix_memory_print_stats();
    
    free(g_memory_context->descriptors);
    free(g_memory_context);
    g_memory_context = NULL;
    
    printf("Memory manager cleanup complete\n");
}

// 記憶體完整性檢查
DLL_EXPORT int retryix_memory_validate(void) {
    if (!g_memory_context) return -1;
    
    int errors = 0;
    
    for (size_t i = 0; i < g_memory_context->descriptor_count; i++) {
        retryix_memory_descriptor_t* desc = &g_memory_context->descriptors[i];
        
        // 檢查基本指針
        if (!desc->host_ptr) {
            printf("ERROR: Null host pointer in descriptor %zu\n", i);
            errors++;
        }
        
        if (!desc->device_mem) {
            printf("ERROR: Null device memory in descriptor %zu\n", i);
            errors++;
        }
        
        // 檢查引用計數
        if (desc->ref_count == 0) {
            printf("ERROR: Zero reference count for %s\n", desc->debug_name);
            errors++;
        }
        
        // 檢查大小合理性
        if (desc->size == 0) {
            printf("ERROR: Zero size for %s\n", desc->debug_name);
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("Memory validation passed (%zu blocks checked)\n", g_memory_context->descriptor_count);
    } else {
        printf("Memory validation failed with %d errors\n", errors);
    }
    
    return errors;
}