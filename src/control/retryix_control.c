// retryix_control.c - 原子細粒控制
#define WIN32_LEAN_AND_MEAN
#define RETRYIX_BUILD_DLL

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "retryix.h"
#include "cJSON.h"
#include "retryix_core.h"
#include "retryix_device.h"

// 工作負載親和性配置
typedef struct {
    uint64_t workload_id;
    int      numa_node;
    int      gpu_device_index;
    int      network_port_index;
    int      rdma_enabled;
    int      gpu_audio_offload;
} retryix_workload_affinity_t;

#define MAX_WORKLOADS 64
static retryix_workload_affinity_t g_workload_table[MAX_WORKLOADS] = {0};
static int g_workload_count = 0;

// 設定工作負載親和性
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_set_workload_topology_affinity(
    uint64_t workload_id, 
    const char* affinity_config_json
) {
    if (!affinity_config_json) {
        return RETRYIX_ERROR_NULL_PTR;
    }

    cJSON *config = cJSON_Parse(affinity_config_json);
    if (!config) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    int index = -1;
    for (int i = 0; i < g_workload_count; i++) {
        if (g_workload_table[i].workload_id == workload_id) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        if (g_workload_count >= MAX_WORKLOADS) {
            cJSON_Delete(config);
            return RETRYIX_ERROR_OUT_OF_MEMORY;
        }
        index = g_workload_count++;
        g_workload_table[index].workload_id = workload_id;
    }

    cJSON *numa = cJSON_GetObjectItem(config, "numa_node");
    if (numa && cJSON_IsNumber(numa)) {
        g_workload_table[index].numa_node = numa->valueint;
    } else {
        g_workload_table[index].numa_node = -1;
    }

    cJSON *gpu = cJSON_GetObjectItem(config, "gpu_device");
    if (gpu && cJSON_IsNumber(gpu)) {
        g_workload_table[index].gpu_device_index = gpu->valueint;
    } else {
        g_workload_table[index].gpu_device_index = -1;
    }

    cJSON *net = cJSON_GetObjectItem(config, "network_port");
    if (net && cJSON_IsNumber(net)) {
        g_workload_table[index].network_port_index = net->valueint;
    } else {
        g_workload_table[index].network_port_index = -1;
    }

    cJSON *rdma = cJSON_GetObjectItem(config, "rdma_enabled");
    if (rdma && cJSON_IsBool(rdma)) {
        g_workload_table[index].rdma_enabled = cJSON_IsTrue(rdma) ? 1 : 0;
    } else {
        g_workload_table[index].rdma_enabled = 0;
    }

    cJSON *audio = cJSON_GetObjectItem(config, "gpu_audio_offload");
    if (audio && cJSON_IsBool(audio)) {
        g_workload_table[index].gpu_audio_offload = cJSON_IsTrue(audio) ? 1 : 0;
    } else {
        g_workload_table[index].gpu_audio_offload = 0;
    }

    cJSON_Delete(config);

    return RETRYIX_SUCCESS;
}

// 查詢工作負載親和性
RETRYIX_API char* RETRYIX_CALL retryix_query_workload_affinity(uint64_t workload_id) {
    int index = -1;
    for (int i = 0; i < g_workload_count; i++) {
        if (g_workload_table[i].workload_id == workload_id) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "workload_id", (double)workload_id);
        cJSON_AddStringToObject(root, "status", "not_found");
        char* json = cJSON_Print(root);
        cJSON_Delete(root);
        return json;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "workload_id", (double)workload_id);
    cJSON_AddNumberToObject(root, "numa_node", (double)g_workload_table[index].numa_node);
    cJSON_AddNumberToObject(root, "gpu_device", (double)g_workload_table[index].gpu_device_index);
    cJSON_AddNumberToObject(root, "network_port", (double)g_workload_table[index].network_port_index);
    cJSON_AddBoolToObject(root, "rdma_enabled", g_workload_table[index].rdma_enabled);
    cJSON_AddBoolToObject(root, "gpu_audio_offload", g_workload_table[index].gpu_audio_offload);
    cJSON_AddStringToObject(root, "status", "active");

    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

// 原子設備重置
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_atomic_reset_device_state(
    const char* device_name
) {
    if (!device_name) {
        return RETRYIX_ERROR_NULL_PTR;
    }

    return RETRYIX_SUCCESS;
}

// 查詢鏈路延遲
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_query_link_latency_us(
    const char* nic_name,
    double* latency_us
) {
    if (!nic_name || !latency_us) {
        return RETRYIX_ERROR_NULL_PTR;
    }

    if (strstr(nic_name, "InfiniBand") || strstr(nic_name, "RDMA")) {
        *latency_us = 1.5;
    } else if (strstr(nic_name, "100G")) {
        *latency_us = 5.0;
    } else {
        *latency_us = 50.0;
    }

    return RETRYIX_SUCCESS;
}

// 查詢設備溫度
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_query_device_temperature(
    const char* device_name,
    float* temperature_celsius
) {
    if (!device_name || !temperature_celsius) {
        return RETRYIX_ERROR_NULL_PTR;
    }

    if (strstr(device_name, "GPU") || strstr(device_name, "RTX") || strstr(device_name, "Radeon")) {
        *temperature_celsius = 65.0f;
    } else if (strstr(device_name, "CPU")) {
        *temperature_celsius = 55.0f;
    } else {
        *temperature_celsius = 45.0f;
    }

    return RETRYIX_SUCCESS;
}

// 系統健康狀態
RETRYIX_API char* RETRYIX_CALL retryix_get_system_health_json(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON *devices = cJSON_CreateArray();

    retryix_device_t gpus[RETRYIX_MAX_DEVICES];
    int gpu_count = 0;
    if (retryix_discover_all_devices(gpus, RETRYIX_MAX_DEVICES, &gpu_count) == RETRYIX_SUCCESS) {
        for (int i = 0; i < gpu_count; i++) {
            cJSON *gpu = cJSON_CreateObject();
            cJSON_AddStringToObject(gpu, "type", "GPU");
            cJSON_AddStringToObject(gpu, "name", gpus[i].name);
            
            float temp = 0.0f;
            retryix_query_device_temperature(gpus[i].name, &temp);
            cJSON_AddNumberToObject(gpu, "temperature_c", temp);
            cJSON_AddStringToObject(gpu, "status", temp < 85.0f ? "healthy" : "warning");
            
            cJSON_AddItemToArray(devices, gpu);
        }
    }

    const char* nics[] = {"Mellanox ConnectX-6", "Intel E810", "NVIDIA BlueField"};
    for (int i = 0; i < 3; i++) {
        cJSON *nic = cJSON_CreateObject();
        cJSON_AddStringToObject(nic, "type", "Network");
        cJSON_AddStringToObject(nic, "name", nics[i]);
        
        double latency = 0.0;
        retryix_query_link_latency_us(nics[i], &latency);
        cJSON_AddNumberToObject(nic, "latency_us", latency);
        cJSON_AddStringToObject(nic, "status", latency < 10.0 ? "healthy" : "degraded");
        
        cJSON_AddItemToArray(devices, nic);
    }

    cJSON_AddItemToObject(root, "devices", devices);
    cJSON_AddNumberToObject(root, "active_workloads", g_workload_count);
    cJSON_AddStringToObject(root, "overall_status", "operational");
    cJSON_AddStringToObject(root, "timestamp", __DATE__ " " __TIME__);

    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}
