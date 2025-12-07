/*
 * retryix_zerocopy.h
 * Zerocopy Network API - RDMA, RoCE, iWARP, Infiniband Support
 * High-performance network data transfer without CPU memcpy
 */

#ifndef RETRYIX_ZEROCOPY_H
#define RETRYIX_ZEROCOPY_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "retryix_export.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================== 網路協議類型定義 =====================

/**
 * 網路協議類型枚舉
 */
typedef enum {
    RETRYIX_NET_PROTO_RDMA = 0,          ///< RDMA (RoCEv1/v2)
    RETRYIX_NET_PROTO_IWARP,              ///< iWARP
    RETRYIX_NET_PROTO_INFINIBAND,         ///< Infiniband
    RETRYIX_NET_PROTO_ROCE_V1,            ///< RoCE v1.0
    RETRYIX_NET_PROTO_ROCE_V2,            ///< RoCE v2.0
    RETRYIX_NET_PROTO_TCP_OFFLOAD,        ///< TCP Offload Engine
    RETRYIX_NET_PROTO_UDP_OFFLOAD,        ///< UDP Offload Engine
    RETRYIX_NET_PROTO_UNKNOWN
} retryix_net_protocol_t;

/**
 * 網路設備類型
 */
typedef enum {
    RETRYIX_NET_DEV_ETHERNET = 0,         ///< 以太網路
    RETRYIX_NET_DEV_INFINIBAND,           ///< Infiniband HCA
    RETRYIX_NET_DEV_OMNIPATH,             ///< Intel Omnipath
    RETRYIX_NET_DEV_ROCE,                 ///< RoCE RNIC
    RETRYIX_NET_DEV_IWARP,                ///< iWARP RNIC
    RETRYIX_NET_DEV_UNKNOWN
} retryix_net_device_type_t;

/**
 * Zerocopy操作結果
 */
typedef enum {
    RETRYIX_ZC_SUCCESS = 0,
    RETRYIX_ZC_ERROR_INVALID_PARAM,
    RETRYIX_ZC_ERROR_NO_DEVICE,
    RETRYIX_ZC_ERROR_CONNECTION_FAILED,
    RETRYIX_ZC_ERROR_TIMEOUT,
    RETRYIX_ZC_ERROR_OUT_OF_MEMORY,
    RETRYIX_ZC_ERROR_PROTOCOL_NOT_SUPPORTED,
    RETRYIX_ZC_ERROR_BUFFER_TOO_SMALL,
    RETRYIX_ZC_ERROR_TRANSFER_FAILED,
    RETRYIX_ZC_ERROR_NOT_INITIALIZED,
    RETRYIX_ZC_ERROR_DEVICE_BUSY,
    RETRYIX_ZC_ERROR_NETWORK_DOWN,
    RETRYIX_ZC_ERROR_NOT_IMPLEMENTED
} retryix_zerocopy_result_t;

/**
 * 網路緩衝區描述符
 */
typedef struct {
    void* buffer;                         ///< 緩衝區指針
    size_t size;                          ///< 緩衝區大小
    uint64_t remote_key;                  ///< 遠程記憶體鍵 (RDMA)
    uint32_t lkey;                        ///< 本地記憶體鍵
    uint32_t rkey;                        ///< 遠程記憶體鍵
    bool is_registered;                   ///< 是否已註冊
    bool owns_buffer;                     ///< 是否擁有緩衝區所有權
    void* handle;                         ///< 軟體緩衝區句柄
} retryix_net_buffer_t;

/**
 * 網路連接信息
 */
typedef struct {
    char remote_ip[64];                   ///< 遠程IP地址
    uint16_t remote_port;                 ///< 遠程端口
    char local_ip[64];                    ///< 本地IP地址
    uint16_t local_port;                  ///< 本地端口
    retryix_net_protocol_t protocol;      ///< 協議類型
    uint32_t qp_num;                      ///< Queue Pair號碼 (IB/RDMA)
    uint32_t psn;                         ///< Packet Sequence Number
} retryix_net_connection_t;

/**
 * DMA傳輸狀態
 */
typedef enum {
    RETRYIX_DMA_IDLE = 0,
    RETRYIX_DMA_IN_PROGRESS,
    RETRYIX_DMA_COMPLETED,
    RETRYIX_DMA_ERROR,
    RETRYIX_DMA_TIMEOUT
} retryix_dma_status_t;

/**
 * DMA傳輸描述符
 */
typedef struct {
    void* local_buffer;                   ///< 本地緩衝區
    uint64_t remote_addr;                 ///< 遠程地址
    size_t size;                          ///< 傳輸大小
    uint32_t rkey;                        ///< 遠程鍵
    retryix_dma_status_t status;          ///< 傳輸狀態
    uint64_t transfer_id;                 ///< 傳輸ID
} retryix_dma_transfer_t;

// ===================== API函數聲明 =====================

// 網路初始化和清理
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_net_init(void);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_net_cleanup(void);

// 協議配置
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_configure_rdma(const char* device_name);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_configure_dpdk(uint16_t port_id);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_infiniband_init(const char* device_name);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_iwarp_init(const char* device_name);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_roce_init(const char* device_name);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_omnipath_init(const char* device_name);

// 網路緩衝區管理
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_create_net_buffer(size_t size, retryix_net_buffer_t** buffer);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_destroy_net_buffer(retryix_net_buffer_t* buffer);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_register_buffer(void* buffer, size_t size, retryix_net_buffer_t** net_buffer);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_unregister_buffer(retryix_net_buffer_t* buffer);

// 網路連接管理
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_balance_net_load(uint32_t* load_distribution, int num_devices);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_coherence_protocol(retryix_net_connection_t* connection);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_create_distributed_svm(void* svm_ptr, size_t size, retryix_net_buffer_t** distributed_buffer);

// 網路拓撲發現
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_discover_net_topology(char* topology_info, size_t buffer_size);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_monitor_net_perf(float* bandwidth_mbps, float* latency_us, int* packet_loss);

// 網路健康監控
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_get_system_health(char* health_report, size_t buffer_size);

// DMA傳輸操作
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_dma_transfer(const retryix_net_connection_t* connection, const void* local_buffer, uint64_t remote_addr, size_t size, uint32_t rkey);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_dma_transfer_async(const retryix_net_connection_t* connection, const void* local_buffer, uint64_t remote_addr, size_t size, uint32_t rkey, uint64_t* transfer_id);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_dma_wait(uint64_t transfer_id, uint32_t timeout_ms);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_dma_status(uint64_t transfer_id, retryix_dma_status_t* status);

// GPU網路互操作
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_gpu_rdma_read(const retryix_net_connection_t* connection, void* gpu_buffer, uint64_t remote_addr, size_t size, uint32_t rkey);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_gpu_rdma_write(const retryix_net_connection_t* connection, const void* gpu_buffer, uint64_t remote_addr, size_t size, uint32_t rkey);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_gpu_to_net(const void* gpu_buffer, retryix_net_buffer_t* net_buffer, size_t size);
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_net_to_gpu(const retryix_net_buffer_t* net_buffer, void* gpu_buffer, size_t size);

// 網路路徑優化
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_optimize_net_path(const char* source_ip, const char* dest_ip, char* optimized_path, size_t buffer_size);

// 遠程記憶體管理
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_migrate_remote_memory(uint64_t source_addr, uint64_t dest_addr, size_t size, const retryix_net_connection_t* connection);

// 分佈式記憶體同步
RETRYIX_API retryix_zerocopy_result_t RETRYIX_CALL retryix_zerocopy_sync_distributed_memory(void* local_buffer, size_t size, const retryix_net_connection_t* connections, int num_connections);

#ifdef __cplusplus
}
#endif

#endif /* RETRYIX_ZEROCOPY_H */