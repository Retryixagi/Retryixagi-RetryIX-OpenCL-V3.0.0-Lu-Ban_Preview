#pragma once
// RetryIX OpenCL 兼容層 - 純模擬實現,不依賴 OpenCL SDK
// 定義 OpenCL 類型和常量用於模擬

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

// OpenCL 基礎類型定義
#ifndef RETRYIX_OPENCL_TYPES_DEFINED
#define RETRYIX_OPENCL_TYPES_DEFINED
typedef int32_t cl_int;
typedef uint32_t cl_uint;
typedef uint64_t cl_ulong;
typedef uint64_t cl_bitfield;
typedef uint32_t cl_bool;
typedef uint64_t cl_device_type;
typedef cl_bitfield cl_device_svm_capabilities;
typedef cl_bitfield cl_mem_flags;
typedef cl_bitfield cl_map_flags;

// OpenCL 指標類型(不透明)
typedef struct _cl_mem* cl_mem;
typedef struct _cl_kernel* cl_kernel;
typedef struct _cl_context* cl_context;
typedef struct _cl_command_queue* cl_command_queue;
typedef struct _cl_program* cl_program;
typedef struct _cl_platform* cl_platform_id;
typedef struct _cl_device* cl_device_id;
#endif

// OpenCL 錯誤碼
#define CL_SUCCESS 0
#define CL_DEVICE_NOT_FOUND -1
#define CL_INVALID_VALUE -30
#define CL_OUT_OF_RESOURCES -5
#define CL_OUT_OF_HOST_MEMORY -6

// OpenCL 布爾值
#define CL_TRUE 1
#define CL_FALSE 0

// OpenCL 設備類型
#define CL_DEVICE_TYPE_DEFAULT      (1 << 0)
#define CL_DEVICE_TYPE_CPU          (1 << 1)
#define CL_DEVICE_TYPE_GPU          (1 << 2)
#define CL_DEVICE_TYPE_ACCELERATOR  (1 << 3)
#define CL_DEVICE_TYPE_CUSTOM       (1 << 4)
#define CL_DEVICE_TYPE_ALL          0xFFFFFFFF

// OpenCL SVM 能力標誌
#define CL_DEVICE_SVM_COARSE_GRAIN_BUFFER (1 << 0)
#define CL_DEVICE_SVM_FINE_GRAIN_BUFFER   (1 << 1)
#define CL_DEVICE_SVM_FINE_GRAIN_SYSTEM   (1 << 2)
#define CL_DEVICE_SVM_ATOMICS             (1 << 3)

// OpenCL 平台查詢參數
#define CL_PLATFORM_NAME                  0x0902
#define CL_PLATFORM_VENDOR                0x0903
#define CL_PLATFORM_VERSION               0x0901
#define CL_PLATFORM_PROFILE               0x0900
#define CL_PLATFORM_EXTENSIONS            0x0904

// OpenCL 記憶體標誌
#define CL_MEM_READ_WRITE                 (1 << 0)
#define CL_MEM_WRITE_ONLY                 (1 << 1)
#define CL_MEM_READ_ONLY                  (1 << 2)
#define CL_MEM_USE_HOST_PTR               (1 << 3)
#define CL_MEM_ALLOC_HOST_PTR             (1 << 4)
#define CL_MEM_COPY_HOST_PTR              (1 << 5)

// OpenCL 映射標誌
#define CL_MAP_READ                       (1 << 0)
#define CL_MAP_WRITE                      (1 << 1)
#define CL_MAP_WRITE_INVALIDATE_REGION    (1 << 2)

// OpenCL 設備查詢參數
#define CL_DEVICE_NAME 0x102B
#define CL_DEVICE_VENDOR 0x102C
#define CL_DEVICE_VERSION 0x102F
#define CL_DRIVER_VERSION 0x102D
#define CL_DEVICE_EXTENSIONS 0x1030
#define CL_DEVICE_TYPE 0x1000
#define CL_DEVICE_MAX_COMPUTE_UNITS 0x1002
#define CL_DEVICE_MAX_CLOCK_FREQUENCY 0x100C
#define CL_DEVICE_GLOBAL_MEM_SIZE 0x101F
#define CL_DEVICE_LOCAL_MEM_SIZE 0x1022
#define CL_DEVICE_MAX_WORK_GROUP_SIZE 0x1004
#define CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS 0x1003
#define CL_DEVICE_MEM_BASE_ADDR_ALIGN 0x1019
#define CL_DEVICE_MAX_WORK_ITEM_SIZES 0x1005
#define CL_DEVICE_AVAILABLE 0x1027
#define CL_DEVICE_SVM_CAPABILITIES 0x1053

// clGetDeviceInfo - RetryIX OpenCL 全新實作
static inline cl_int clGetDeviceInfo(
    cl_device_id device,
    cl_uint param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    (void)device;
    (void)param_value_size_ret;

    switch (param_name) {
        case CL_DEVICE_SVM_CAPABILITIES:
            if (param_value && param_value_size >= sizeof(cl_device_svm_capabilities)) {
                // 模擬:返回完整 SVM 支持
                *(cl_device_svm_capabilities*)param_value = 
                    CL_DEVICE_SVM_COARSE_GRAIN_BUFFER |
                    CL_DEVICE_SVM_FINE_GRAIN_BUFFER |
                    CL_DEVICE_SVM_FINE_GRAIN_SYSTEM |
                    CL_DEVICE_SVM_ATOMICS;
            }
            return CL_SUCCESS;

        case CL_DEVICE_NAME:
            if (param_value && param_value_size > 0) {
                const char* name = "RetryIX Virtual GPU (Lu Ban)";
                size_t len = strlen(name) + 1;
                if (len > param_value_size) len = param_value_size;
                memcpy(param_value, name, len);
                ((char*)param_value)[param_value_size - 1] = '\0';
            }
            return CL_SUCCESS;

        case CL_DEVICE_VERSION:
            if (param_value && param_value_size > 0) {
                const char* version = "OpenCL 2.0 RetryIX v3.0";
                size_t len = strlen(version) + 1;
                if (len > param_value_size) len = param_value_size;
                memcpy(param_value, version, len);
                ((char*)param_value)[param_value_size - 1] = '\0';
            }
            return CL_SUCCESS;

        default:
            return CL_INVALID_VALUE;
    }
}

// OpenCL Platform/Device Query Stubs
static inline cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms) {
    (void)num_entries; (void)platforms;
    if (num_platforms) *num_platforms = 1;
    return CL_SUCCESS;
}

static inline cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type, 
    cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices) {
    (void)platform; (void)device_type; (void)num_entries; (void)devices;
    if (num_devices) *num_devices = 1;
    return CL_SUCCESS;
}

static inline cl_int clGetPlatformInfo(cl_platform_id platform, cl_uint param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret) {
    (void)platform; (void)param_value_size; (void)param_value_size_ret;
    if (param_value && param_name == CL_PLATFORM_NAME) {
        strncpy((char*)param_value, "RetryIX Platform", param_value_size);
    }
    return CL_SUCCESS;
}

// OpenCL Memory Stubs
static inline cl_mem clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size,
    void* host_ptr, cl_int* errcode_ret) {
    (void)context; (void)flags; (void)host_ptr;
    if (errcode_ret) *errcode_ret = CL_SUCCESS;
    return (cl_mem)malloc(size);
}

static inline cl_int clReleaseMemObject(cl_mem memobj) {
    if (memobj) free(memobj);
    return CL_SUCCESS;
}

static inline void* clEnqueueMapBuffer(cl_command_queue queue, cl_mem buffer, cl_bool blocking,
    cl_map_flags map_flags, size_t offset, size_t size, cl_uint num_events, const void* event_wait_list,
    void* event, cl_int* errcode_ret) {
    (void)queue; (void)blocking; (void)map_flags; (void)offset; (void)num_events; (void)event_wait_list; (void)event;
    if (errcode_ret) *errcode_ret = CL_SUCCESS;
    return (void*)((char*)buffer + offset);
}

static inline cl_int clEnqueueUnmapMemObject(cl_command_queue queue, cl_mem memobj, void* mapped_ptr,
    cl_uint num_events, const void* event_wait_list, void* event) {
    (void)queue; (void)memobj; (void)mapped_ptr; (void)num_events; (void)event_wait_list; (void)event;
    return CL_SUCCESS;
}

static inline cl_int clEnqueueWriteBuffer(cl_command_queue queue, cl_mem buffer, cl_bool blocking,
    size_t offset, size_t size, const void* ptr, cl_uint num_events, const void* event_wait_list, void* event) {
    (void)queue; (void)blocking; (void)num_events; (void)event_wait_list; (void)event;
    if (buffer && ptr) {
        memcpy((char*)buffer + offset, ptr, size);
    }
    return CL_SUCCESS;
}

static inline cl_int clEnqueueReadBuffer(cl_command_queue queue, cl_mem buffer, cl_bool blocking,
    size_t offset, size_t size, void* ptr, cl_uint num_events, const void* event_wait_list, void* event) {
    (void)queue; (void)blocking; (void)num_events; (void)event_wait_list; (void)event;
    if (buffer && ptr) {
        memcpy(ptr, (char*)buffer + offset, size);
    }
    return CL_SUCCESS;
}
