
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "retryix.h"

// 若 retryix.h 未定義 RetryIXDevice，則補充定義
#ifndef RETRYIXDEVICE_DEFINED
#define RETRYIXDEVICE_DEFINED
typedef struct {
    char name[256];
    cl_device_type type;
    cl_ulong global_mem_size;
    char opencl_version[128];
    cl_device_id id;
} RetryIXDevice;
#endif

// 輸出所有平台與裝置資訊（含 extension/SVM）成 JSON 檔案
 #ifdef _WIN32
     #define DLL_EXPORT __declspec(dllexport)
 #else
     #define DLL_EXPORT
 #endif

DLL_EXPORT retryix_result_t retryix_export_all_devices_json(const char* out_path) {
    cl_platform_id platforms[8];
    cl_uint num_platforms = 0;
    cl_int status = clGetPlatformIDs(8, platforms, &num_platforms);
    if (status != CL_SUCCESS || num_platforms == 0) return -1;

    FILE* fp = fopen(out_path, "w");
    if (!fp) return -2;
    fprintf(fp, "{\n  \"platforms\": [\n");
    for (cl_uint p = 0; p < num_platforms; ++p) {
        char plat_name[256] = {0};
        clGetPlatformInfo(platforms[p], CL_PLATFORM_NAME, sizeof(plat_name), plat_name, NULL);
        fprintf(fp, "    {\n      \"name\": \"%s\",\n      \"devices\": [\n", plat_name);

        cl_device_id devices[32];
        cl_uint num_devices = 0;
        status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 32, devices, &num_devices);
        for (cl_uint d = 0; d < num_devices; ++d) {
            char dev_name[256] = {0};
            cl_device_type dev_type = 0;
            cl_ulong mem = 0;
            char version[128] = {0};
            char extensions[2048] = {0};
            cl_bitfield svm_caps = 0;
            clGetDeviceInfo(devices[d], CL_DEVICE_NAME, sizeof(dev_name), dev_name, NULL);
            clGetDeviceInfo(devices[d], CL_DEVICE_TYPE, sizeof(dev_type), &dev_type, NULL);
            clGetDeviceInfo(devices[d], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem), &mem, NULL);
            clGetDeviceInfo(devices[d], CL_DEVICE_VERSION, sizeof(version), version, NULL);
            clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, sizeof(extensions), extensions, NULL);
            clGetDeviceInfo(devices[d], CL_DEVICE_SVM_CAPABILITIES, sizeof(svm_caps), &svm_caps, NULL);

            fprintf(fp, "        {\n          \"name\": \"%s\",\n          \"type\": %lu,\n          \"global_mem_size\": %llu,\n          \"opencl_version\": \"%s\",\n          \"extensions\": \"%s\",\n          \"svm_capabilities\": %llu\n        }%s\n",
                dev_name, (unsigned long)dev_type, (unsigned long long)mem, version, extensions, (unsigned long long)svm_caps,
                (d == num_devices-1) ? "" : ",");
        }
        fprintf(fp, "      ]\n    }%s\n", (p == num_platforms-1) ? "" : ",");
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp);
    return 0;
}

// 裝置資訊結構
typedef struct {
    char name[256];
    cl_device_type type;
    cl_ulong global_mem_size;
    char opencl_version[128];
} RetryIXDeviceInfo;

DLL_EXPORT int retryix_get_device_info(int device_index, RetryIXDeviceInfo* info) {
    cl_platform_id platform;
    cl_uint num_platforms = 0;
    cl_int status = clGetPlatformIDs(1, &platform, &num_platforms);
    if (status != CL_SUCCESS || num_platforms == 0) return -1;

    cl_device_id devices[32];
    cl_uint num_devices = 0;
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 32, devices, &num_devices);
    if (status != CL_SUCCESS || num_devices == 0) return -2;

    if (device_index < 0 || device_index >= (int)num_devices) return -3;

    memset(info, 0, sizeof(RetryIXDeviceInfo));
    clGetDeviceInfo(devices[device_index], CL_DEVICE_NAME, sizeof(info->name), info->name, NULL);
    clGetDeviceInfo(devices[device_index], CL_DEVICE_TYPE, sizeof(info->type), &info->type, NULL);
    clGetDeviceInfo(devices[device_index], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(info->global_mem_size), &info->global_mem_size, NULL);
    clGetDeviceInfo(devices[device_index], CL_DEVICE_VERSION, sizeof(info->opencl_version), info->opencl_version, NULL);

    return 0;
}

DLL_EXPORT int retryix_export_device_info_json(int device_index, char* json_buffer, int buffer_size) {
    RetryIXDeviceInfo info;
    if (retryix_get_device_info(device_index, &info) != 0) return -1;

    snprintf(json_buffer, buffer_size,
        "{ \"name\": \"%s\", \"type\": %lu, \"global_mem_size\": %llu, \"opencl_version\": \"%s\" }",
        info.name,
        (unsigned long)info.type,
        (unsigned long long)info.global_mem_size,
        info.opencl_version
    );
    return 0;
}

DLL_EXPORT int retryix_enumerate_platforms(char* buffer, int buffer_size) {
    cl_platform_id platforms[8];
    cl_uint num_platforms = 0;
    cl_int status = clGetPlatformIDs(8, platforms, &num_platforms);
    if (status != CL_SUCCESS || num_platforms == 0) return -1;

    int offset = 0;
    for (cl_uint i = 0; i < num_platforms; ++i) {
        char name[256] = {0};
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(name), name, NULL);
        int written = snprintf(buffer + offset, buffer_size - offset, "[%u] %s\n", i, name);
        if (written < 0 || written >= buffer_size - offset) break;
        offset += written;
    }
    return num_platforms;
}

#define MAX_DEVICES 32

int retryixGetDeviceList(RetryIXDevice* devices, int max_devices, int* out_count) {
    cl_platform_id platform;
    cl_uint num_platforms = 0;
    cl_int status = clGetPlatformIDs(1, &platform, &num_platforms);
    if (status != CL_SUCCESS || num_platforms == 0) return -1;

    cl_device_id cl_devices[32];
    cl_uint num_devices = 0;
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 32, cl_devices, &num_devices);
    if (status != CL_SUCCESS || num_devices == 0) return -2;

    *out_count = (num_devices < (cl_uint)max_devices) ? (int)num_devices : max_devices;

    for (int i = 0; i < *out_count; ++i) {
        RetryIXDevice* dev = &devices[i];
        dev->id = cl_devices[i];

        clGetDeviceInfo(cl_devices[i], CL_DEVICE_NAME, sizeof(dev->name), dev->name, NULL);
        clGetDeviceInfo(cl_devices[i], CL_DEVICE_TYPE, sizeof(dev->type), &dev->type, NULL);
        clGetDeviceInfo(cl_devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(dev->global_mem_size), &dev->global_mem_size, NULL);
        clGetDeviceInfo(cl_devices[i], CL_DEVICE_VERSION, sizeof(dev->opencl_version), dev->opencl_version, NULL);
    }

    return 0;
}

DLL_EXPORT int retryix_enumerate_devices() {
    RetryIXDevice devices[32];
    int device_count = 0;
    if (retryixGetDeviceList(devices, 32, &device_count) == 0) {
        return device_count;
    }
    return -1;
}
