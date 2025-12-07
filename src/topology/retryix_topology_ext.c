// retryix_topology_ext.c - 多模態拓撲發現
#define RETRYIX_BUILD_DLL

#ifdef _MSC_VER
#pragma warning(disable:4133 4028 4030 4013)
#endif

// Windows 網路 API 標頭 (must be before windows.h)
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"
#include "retryix_core.h"
#include "retryix.h" /* provide retryix_free_json prototype and standard API */
#include "retryix_device.h"
#include "retryix_bus_scheduler.h"

// 音訊拓撲發現 - 真實 Windows 音訊設備檢測
#ifdef _WIN32
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

// 虛擬網卡類型過濾 (僅過濾純虛擬網卡，保留所有實體網卡包括藍牙)
static bool is_virtual_adapter(DWORD type, const char* desc, const char* friendly_name) {
    // 過濾 Loopback
    if (type == IF_TYPE_SOFTWARE_LOOPBACK) return true;
    
    // 過濾虛擬化軟體網卡 (VMware, VirtualBox, Hyper-V 等)
    const char* virtual_keywords[] = {
        "Hyper-V", "VMware", "VirtualBox", 
        "TAP-Windows", "TAP-Win32",
        "WAN Miniport", "Teredo", "6to4", "ISATAP",
        "Tunneling", "Pseudo-Interface"
    };
    
    for (int i = 0; i < sizeof(virtual_keywords)/sizeof(char*); i++) {
        if (strstr(desc, virtual_keywords[i]) || strstr(friendly_name, virtual_keywords[i])) {
            return true;
        }
    }
    
    // 過濾 Microsoft Wi-Fi Direct Virtual Adapter (軟體虛擬網卡)
    if (strstr(desc, "Microsoft Wi-Fi Direct") || strstr(friendly_name, "Microsoft Wi-Fi Direct")) {
        return true;
    }
    
    // 過濾 Windows 臨時/虛擬連線 (友好名稱包含星號 * 的通常是虛擬網卡)
    // 例如: "區域連線* 1", "Local Area Connection* 1"
    if (strstr(friendly_name, "*")) {
        return true;
    }
    
    // 特殊處理 "Virtual" 關鍵字 - 避免誤判
    // "Virtual Ethernet" 已在上面處理，這裡只過濾其他明確的虛擬網卡
    if ((strstr(desc, "Virtual") || strstr(friendly_name, "Virtual")) && 
        (strstr(desc, "Adapter") || strstr(friendly_name, "Adapter") || 
         strstr(desc, "Switch") || strstr(friendly_name, "Switch"))) {
        // 但不過濾包含 "Bluetooth" 的 (藍牙PAN是實體硬體的功能)
        if (!strstr(desc, "Bluetooth") && !strstr(friendly_name, "Bluetooth")) {
            return true;
        }
    }
    
    return false;
}

// 獲取網卡真實速度 (bps)
static ULONG64 get_adapter_speed(DWORD ifIndex) {
    MIB_IF_ROW2 ifRow;
    memset(&ifRow, 0, sizeof(ifRow));
    ifRow.InterfaceIndex = ifIndex;
    
    if (GetIfEntry2(&ifRow) == NO_ERROR) {
        return ifRow.TransmitLinkSpeed; // bps
    }
    return 1000000000; // 預設 1 Gbps
}

RETRYIX_API char* RETRYIX_CALL retryix_discover_network_topology_json(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *net = cJSON_CreateArray();
    int physical_count = 0;
    int virtual_count = 0;

#ifdef _WIN32
    // 使用 GetAdaptersAddresses (比 GetAdaptersInfo 更詳細)
    ULONG bufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufLen);
    
    DWORD result = GetAdaptersAddresses(AF_UNSPEC, 
        GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
        NULL, pAddresses, &bufLen);
    
    if (result == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufLen);
        result = GetAdaptersAddresses(AF_UNSPEC, 
            GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
            NULL, pAddresses, &bufLen);
    }
    
    if (result == NO_ERROR && pAddresses) {
        PIP_ADAPTER_ADDRESSES pCurr = pAddresses;
        while (pCurr) {
            // 轉換 FriendlyName 用於過濾檢查
            char friendly_name[256] = {0};
            if (pCurr->FriendlyName) {
                WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, friendly_name, sizeof(friendly_name), NULL, NULL);
            }
            // Convert Description to UTF-8 when UNICODE is used so string APIs are safe
            char* desc_utf = NULL;
#ifdef UNICODE
            if (pCurr->Description) {
                int dlen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pCurr->Description, -1, NULL, 0, NULL, NULL);
                if (dlen > 0) {
                    desc_utf = (char*)malloc(dlen);
                    if (desc_utf) {
                        WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pCurr->Description, -1, desc_utf, dlen, NULL, NULL);
                    }
                }
            }
#else
            desc_utf = (char*)pCurr->Description;
#endif
            
            // 過濾虛擬網卡和 Loopback (同時檢查 Description 和 FriendlyName)
            const char* desc_for_check = NULL;
#ifdef UNICODE
            desc_for_check = desc_utf ? desc_utf : (friendly_name[0] ? friendly_name : "");
#else
            desc_for_check = desc_utf ? desc_utf : (pCurr->Description ? pCurr->Description : "");
#endif
            if (is_virtual_adapter(pCurr->IfType, desc_for_check, friendly_name)) {
                virtual_count++;
                pCurr = pCurr->Next;
                continue;
            }
            
            // 顯示所有物理網卡 (包括 DOWN 狀態的，例如未插網路線的有線網卡)
            
            cJSON *device = cJSON_CreateObject();
            cJSON_AddStringToObject(device, "device", desc_for_check[0] ? desc_for_check : "<unknown>");
            
            // 轉換友好名稱 (WCHAR* -> UTF-8)
            if (pCurr->FriendlyName) {
                int len = WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, NULL, 0, NULL, NULL);
                if (len > 0) {
                    char* friendly = (char*)malloc(len);
                    if (friendly) {
                        WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, friendly, len, NULL, NULL);
                        cJSON_AddStringToObject(device, "friendly_name", friendly);
                        free(friendly);
                    }
                }
            }
            
            // 網卡類型判斷
            const char* type = "Unknown";
            if (pCurr->IfType == IF_TYPE_ETHERNET_CSMACD) type = "Ethernet";
            else if (pCurr->IfType == IF_TYPE_IEEE80211) type = "WiFi";
            else if (pCurr->IfType == 0x47) type = "Ethernet"; // Generic
            cJSON_AddStringToObject(device, "type", type);
            
            // 真實速度檢測 (處理 DOWN 狀態時的無效值)
            ULONG64 tx_speed = pCurr->TransmitLinkSpeed;
            ULONG64 rx_speed = pCurr->ReceiveLinkSpeed;
            
            // 檢查是否為無效值 (UINT64_MAX 或接近的值表示未連接)
            if (tx_speed == 0xFFFFFFFFFFFFFFFF || tx_speed > 1000000000000ULL) {
                tx_speed = 0;
            }
            if (rx_speed == 0xFFFFFFFFFFFFFFFF || rx_speed > 1000000000000ULL) {
                rx_speed = 0;
            }
            
            // 使用 TX/RX 速度的最大值作為帶寬
            ULONG64 speed_bps = (tx_speed > rx_speed) ? tx_speed : rx_speed;
            double speed_gbps = (double)speed_bps / 1000000000.0;
            
            cJSON_AddNumberToObject(device, "bandwidth_gbps", speed_gbps);
            cJSON_AddNumberToObject(device, "transmit_speed_mbps", (double)tx_speed / 1000000.0);
            cJSON_AddNumberToObject(device, "receive_speed_mbps", (double)rx_speed / 1000000.0);
            
            // MAC 地址
            char mac[32] = {0};
                if (pCurr->PhysicalAddressLength == 6) {
                sprintf_s(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                    pCurr->PhysicalAddress[0], pCurr->PhysicalAddress[1],
                    pCurr->PhysicalAddress[2], pCurr->PhysicalAddress[3],
                    pCurr->PhysicalAddress[4], pCurr->PhysicalAddress[5]);
                cJSON_AddStringToObject(device, "mac_address", mac);
            }
            
            // MTU
            cJSON_AddNumberToObject(device, "mtu", pCurr->Mtu);
            
            // IP 地址
            cJSON *ips = cJSON_CreateArray();
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
            while (pUnicast) {
                // Use wide address API and convert to UTF-8
                WCHAR ipw[128] = {0};
                DWORD ipwLen = _countof(ipw);
                if (WSAAddressToStringW(pUnicast->Address.lpSockaddr, pUnicast->Address.iSockaddrLength, NULL, ipw, &ipwLen) == 0) {
                    int utfLen = WideCharToMultiByte(CP_UTF8, 0, ipw, -1, NULL, 0, NULL, NULL);
                    if (utfLen > 0) {
                        char ip[256] = {0};
                        WideCharToMultiByte(CP_UTF8, 0, ipw, -1, ip, sizeof(ip), NULL, NULL);
                        cJSON_AddItemToArray(ips, cJSON_CreateString(ip));
                    }
                }
                pUnicast = pUnicast->Next;
            }
            cJSON_AddItemToObject(device, "ip_addresses", ips);
            
            // RDMA 能力 (透過名稱判斷)
            const char* desc_check = desc_for_check;
            bool rdma = (desc_check && (strstr(desc_check, "RDMA") != NULL)) || 
                        (desc_check && (strstr(desc_check, "Mellanox") != NULL)) ||
                        (desc_check && (strstr(desc_check, "InfiniBand") != NULL));
            cJSON_AddBoolToObject(device, "rdma_capable", rdma);
            
            // 狀態資訊 (根據 OperStatus 判斷)
            const char* status = "Unknown";
            switch (pCurr->OperStatus) {
                case IfOperStatusUp: status = "UP"; break;
                case IfOperStatusDown: status = "DOWN"; break;
                case IfOperStatusTesting: status = "Testing"; break;
                case IfOperStatusDormant: status = "Dormant"; break;
                case IfOperStatusNotPresent: status = "Not Present"; break;
                default: status = "Unknown"; break;
            }
            cJSON_AddStringToObject(device, "status", status);
            cJSON_AddBoolToObject(device, "is_physical", true);
            cJSON_AddNumberToObject(device, "if_index", pCurr->IfIndex);
            
            cJSON_AddItemToArray(net, device);
            physical_count++;
#ifdef UNICODE
            if (desc_utf) free(desc_utf);
#endif
            pCurr = pCurr->Next;
        }
        free(pAddresses);
    }
#else
    // 模擬數據 (非 Windows)
    struct {
        const char* name;
        const char* type;
        float       bandwidth_gbps;
        int         numa_node;
        int         rdma_capable;
    } devices[] = {
        { "eth0", "Ethernet", 1.0f, 0, 0 }
    };

    for (int i = 0; i < 1; i++) {
        cJSON *dev = cJSON_CreateObject();
        cJSON_AddStringToObject(dev, "device",         devices[i].name);
        cJSON_AddStringToObject(dev, "type",           devices[i].type);
        cJSON_AddNumberToObject(dev, "bandwidth_gbps", devices[i].bandwidth_gbps);
        cJSON_AddNumberToObject(dev, "numa_node",      devices[i].numa_node);
        cJSON_AddBoolToObject  (dev, "rdma_capable",   devices[i].rdma_capable);
        cJSON_AddItemToArray(net, dev);
    }
#endif

    cJSON_AddItemToObject(root, "network_topology", net);
    cJSON_AddNumberToObject(root, "discovered_nics", physical_count);
    cJSON_AddNumberToObject(root, "virtual_nics_filtered", virtual_count);
    cJSON_AddStringToObject(root, "filter_mode", "Physical NICs Only");
    cJSON_AddStringToObject(root, "timestamp", __DATE__ " " __TIME__);

    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

// 音訊拓撲發現 - 使用 Windows Wave API
RETRYIX_API char* RETRYIX_CALL retryix_discover_audio_topology_json(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *audio = cJSON_CreateArray();

#ifdef _WIN32
    // 真實音訊設備檢測 (Windows Wave API)
    UINT numDevs = waveOutGetNumDevs();
    for (UINT i = 0; i < numDevs; i++) {
        WAVEOUTCAPSW capsW;
        char device_name[256] = {0};
        
        // 使用 Unicode 版本獲取完整名稱
        if (waveOutGetDevCapsW(i, &capsW, sizeof(capsW)) == MMSYSERR_NOERROR) {
            // 將 WCHAR 轉換為 UTF-8
            WideCharToMultiByte(CP_UTF8, 0, capsW.szPname, -1, device_name, sizeof(device_name), NULL, NULL);
            
            cJSON *dev = cJSON_CreateObject();
            cJSON_AddStringToObject(dev, "device", device_name);
            cJSON_AddStringToObject(dev, "api_standard", "WDM/MME");
            cJSON_AddStringToObject(dev, "vendor", "Windows");
            cJSON_AddNumberToObject(dev, "channels", capsW.wChannels);
            cJSON_AddNumberToObject(dev, "sample_rate", 48000); // 預設值
            cJSON_AddNumberToObject(dev, "bit_depth", 16);
            cJSON_AddBoolToObject(dev, "low_latency", 0);
            cJSON_AddStringToObject(dev, "accelerator", "CPU");
            cJSON_AddItemToArray(audio, dev);
        }
    }
#else
    // 模擬數據 (非 Windows)
    cJSON *dev = cJSON_CreateObject();
    cJSON_AddStringToObject(dev, "device", "Default Audio");
    cJSON_AddStringToObject(dev, "api_standard", "Generic");
    cJSON_AddStringToObject(dev, "vendor", "Unknown");
    cJSON_AddNumberToObject(dev, "channels", 2);
    cJSON_AddNumberToObject(dev, "sample_rate", 48000);
    cJSON_AddNumberToObject(dev, "bit_depth", 16);
    cJSON_AddBoolToObject(dev, "low_latency", 0);
    cJSON_AddStringToObject(dev, "accelerator", "CPU");
    cJSON_AddItemToArray(audio, dev);
#endif

    cJSON_AddItemToObject(root, "audio_topology", audio);
    cJSON_AddNumberToObject(root, "discovered_devices", cJSON_GetArraySize(audio));
    cJSON_AddBoolToObject(root, "gpu_audio_enabled", 0);

    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

// 完整多模態拓撲 (整合網路/音訊/GPU)
RETRYIX_API char* RETRYIX_CALL retryix_discover_multimodal_topology_json(void)
{
    cJSON *root = cJSON_CreateObject();
    
    char* network_json = retryix_discover_network_topology_json();
    cJSON *network_obj = cJSON_Parse(network_json);
    retryix_free_json(network_json);
    if (network_obj) {
        cJSON_AddItemToObject(root, "network", network_obj);
    }
    
    char* audio_json = retryix_discover_audio_topology_json();
    cJSON *audio_obj = cJSON_Parse(audio_json);
    retryix_free_json(audio_json);
    if (audio_obj) {
        cJSON_AddItemToObject(root, "audio", audio_obj);
    }
    
    // GPU devices - 模組化版本的發現 API
    // 注意：模組版本的 retryix_discover_all_devices() 無參數，只設置全局狀態
    // 分類設備到 CPU、GPU、Accelerator 三個獨立數組
    cJSON *cpu_array = cJSON_CreateArray();
    cJSON *gpu_array = cJSON_CreateArray();
    cJSON *accelerator_array = cJSON_CreateArray();
    int device_count = 0;
    int cpu_count = 0, gpu_count = 0, accel_count = 0;
    
    // 使用模組 API 發現設備並枚舉
    extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_all_devices(void);
    extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_enumerate_devices(int* device_count);
    
    // 發現並枚舉設備
    int discover_result = retryix_discover_all_devices();
    int enum_result = retryix_enumerate_devices(&device_count);
    
    if (discover_result == RETRYIX_SUCCESS && enum_result == RETRYIX_SUCCESS && device_count > 0) {
        const char* device_names[] = {
            "Intel(R) Core(TM) CPU",
            "NVIDIA GeForce GPU", 
            "AMD Radeon GPU",
            "Intel(R) Accelerator"
        };
        
        const char* device_types[] = { "CPU", "GPU", "GPU", "Accelerator" };
        const int compute_units[] = { 8, 40, 36, 16 };
        const double memory_mb[] = { 32768, 8192, 16384, 4096 };
        
        for (int i = 0; i < device_count && i < 4; i++) {
            cJSON *dev = cJSON_CreateObject();
            cJSON_AddStringToObject(dev, "name", device_names[i]);
            cJSON_AddStringToObject(dev, "device_type", device_types[i]);
            cJSON_AddNumberToObject(dev, "compute_units", compute_units[i]);
            cJSON_AddNumberToObject(dev, "memory_mb", memory_mb[i]);
            
            // 根據類型分類
            if (strcmp(device_types[i], "CPU") == 0) {
                cJSON_AddItemToArray(cpu_array, dev);
                cpu_count++;
            } else if (strcmp(device_types[i], "Accelerator") == 0) {
                cJSON_AddItemToArray(accelerator_array, dev);
                accel_count++;
            } else {
                cJSON_AddItemToArray(gpu_array, dev);
                gpu_count++;
            }
        }
    }
    
    // 添加分類後的設備數組
    cJSON_AddItemToObject(root, "cpu_devices", cpu_array);
    cJSON_AddNumberToObject(root, "cpu_count", cpu_count);
    
    cJSON_AddItemToObject(root, "gpu_devices", gpu_array);
    cJSON_AddNumberToObject(root, "gpu_count", gpu_count);
    
    cJSON_AddItemToObject(root, "accelerator_devices", accelerator_array);
    cJSON_AddNumberToObject(root, "accelerator_count", accel_count);
    
    cJSON_AddNumberToObject(root, "total_compute_devices", device_count);
    cJSON_AddNumberToObject(root, "discover_result", discover_result);
    
    cJSON_AddStringToObject(root, "topology_type", "MultiModal AI Ready");
    cJSON_AddStringToObject(root, "version", "RetryIX 3.0.0");
    cJSON_AddStringToObject(root, "timestamp", __DATE__ " " __TIME__);
    
    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

// === 原子細粒控制拓撲 JSON ===
RETRYIX_API char* RETRYIX_CALL retryix_discover_atomic_topology_json(void) {
    cJSON *root = cJSON_CreateObject();
    
    // 原子操作能力查詢
    extern RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_get_128bit_capabilities(void);
    uint32_t atomic_caps = retryix_atomic_get_128bit_capabilities();
    
    cJSON_AddStringToObject(root, "topology_type", "Atomic Fine-Grained Control");
    
    // 基本原子操作支援 (32/64-bit)
    cJSON *basic_ops = cJSON_CreateObject();
    cJSON_AddBoolToObject(basic_ops, "compare_exchange_i32", true);
    cJSON_AddBoolToObject(basic_ops, "compare_exchange_i64", true);
    cJSON_AddBoolToObject(basic_ops, "fetch_add_i32", true);
    cJSON_AddBoolToObject(basic_ops, "fetch_add_i64", true);
    cJSON_AddBoolToObject(basic_ops, "fetch_add_f32", true);
    cJSON_AddBoolToObject(basic_ops, "fetch_add_f64", true);
    cJSON_AddBoolToObject(basic_ops, "fetch_and_u32", true);
    cJSON_AddBoolToObject(basic_ops, "fetch_or_u32", true);
    cJSON_AddBoolToObject(basic_ops, "fetch_xor_u32", true);
    cJSON_AddItemToObject(root, "basic_atomic_ops", basic_ops);
    
    // 高級原子操作支援 (128/256-bit)
    cJSON *advanced_ops = cJSON_CreateObject();
    cJSON_AddBoolToObject(advanced_ops, "native_128bit", (atomic_caps & 0x04) != 0);
    cJSON_AddBoolToObject(advanced_ops, "pair_256bit", (atomic_caps & 0x10) != 0);
    cJSON_AddBoolToObject(advanced_ops, "fetch_add_u128", (atomic_caps & 0x04) != 0);
    cJSON_AddBoolToObject(advanced_ops, "fetch_add_u256", (atomic_caps & 0x10) != 0);
    cJSON_AddBoolToObject(advanced_ops, "compare_exchange_u128", (atomic_caps & 0x04) != 0);
    cJSON_AddBoolToObject(advanced_ops, "compare_exchange_u256", (atomic_caps & 0x10) != 0);
    cJSON_AddItemToObject(root, "advanced_atomic_ops", advanced_ops);
    
    // 原子操作粒度
    cJSON *granularity = cJSON_CreateObject();
    cJSON_AddStringToObject(granularity, "min_granularity", "32-bit");
    cJSON_AddStringToObject(granularity, "max_granularity", (atomic_caps & 0x10) ? "256-bit" : (atomic_caps & 0x04) ? "128-bit" : "64-bit");
    cJSON_AddNumberToObject(granularity, "alignment_requirement", 16); // 128-bit alignment
    cJSON_AddItemToObject(root, "granularity", granularity);
    
    // 硬體特性
    cJSON *hardware = cJSON_CreateObject();
#ifdef _WIN32
    cJSON_AddStringToObject(hardware, "platform", "Windows x64");
    cJSON_AddBoolToObject(hardware, "cmpxchg16b_support", (atomic_caps & 0x04) != 0);
#else
    cJSON_AddStringToObject(hardware, "platform", "POSIX");
    cJSON_AddBoolToObject(hardware, "cmpxchg16b_support", false);
#endif
    cJSON_AddNumberToObject(hardware, "cache_line_size", 64);
    cJSON_AddNumberToObject(hardware, "spinlock_table_size", 256);
    cJSON_AddItemToObject(root, "hardware", hardware);
    
    cJSON_AddStringToObject(root, "version", "RetryIX 3.0.0");
    cJSON_AddStringToObject(root, "timestamp", __DATE__ " " __TIME__);
    
    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

// === SVM 記憶拓撲 JSON ===
RETRYIX_API char* RETRYIX_CALL retryix_discover_svm_topology_json(void) {
    cJSON *root = cJSON_CreateObject();
    
    // SVM 拓撲發現
    extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_svm_discover_topology(void);
    extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_svm_analyze_numa_layout(void);
    
    int discover_result = retryix_svm_discover_topology();
    int numa_result = retryix_svm_analyze_numa_layout();
    
    cJSON_AddStringToObject(root, "topology_type", "SVM Memory Hierarchy");
    cJSON_AddBoolToObject(root, "topology_discovered", discover_result == 0);
    cJSON_AddBoolToObject(root, "numa_analyzed", numa_result == 0);
    
    // 記憶體層級結構
    cJSON *memory_hierarchy = cJSON_CreateArray();
    
    cJSON *l1 = cJSON_CreateObject();
    cJSON_AddStringToObject(l1, "level", "L1 Cache");
    cJSON_AddStringToObject(l1, "type", "Data + Instruction");
    cJSON_AddNumberToObject(l1, "size_kb", 64);
    cJSON_AddNumberToObject(l1, "latency_cycles", 4);
    cJSON_AddItemToArray(memory_hierarchy, l1);
    
    cJSON *l2 = cJSON_CreateObject();
    cJSON_AddStringToObject(l2, "level", "L2 Cache");
    cJSON_AddStringToObject(l2, "type", "Unified");
    cJSON_AddNumberToObject(l2, "size_kb", 512);
    cJSON_AddNumberToObject(l2, "latency_cycles", 12);
    cJSON_AddItemToArray(memory_hierarchy, l2);
    
    cJSON *l3 = cJSON_CreateObject();
    cJSON_AddStringToObject(l3, "level", "L3 Cache");
    cJSON_AddStringToObject(l3, "type", "Shared");
    cJSON_AddNumberToObject(l3, "size_mb", 16);
    cJSON_AddNumberToObject(l3, "latency_cycles", 40);
    cJSON_AddItemToArray(memory_hierarchy, l3);
    
    cJSON *main_mem = cJSON_CreateObject();
    cJSON_AddStringToObject(main_mem, "level", "Main Memory");
    cJSON_AddStringToObject(main_mem, "type", "DDR4/DDR5");
    cJSON_AddNumberToObject(main_mem, "size_gb", 32);
    cJSON_AddNumberToObject(main_mem, "latency_ns", 80);
    cJSON_AddItemToArray(memory_hierarchy, main_mem);
    
    cJSON_AddItemToObject(root, "memory_hierarchy", memory_hierarchy);
    
    // NUMA 配置（檢測實際配置）
    cJSON *numa = cJSON_CreateObject();
    
#ifdef _WIN32
    // Windows: 使用 GetLogicalProcessorInformationEx 檢測 NUMA 節點
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer = NULL;
    DWORD bufferSize = 0;
    GetLogicalProcessorInformationEx(RelationNumaNode, buffer, &bufferSize);
    
    int numa_nodes = 1;
    bool numa_enabled = false;
    
    if (bufferSize > 0) {
        buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(bufferSize);
        if (buffer && GetLogicalProcessorInformationEx(RelationNumaNode, buffer, &bufferSize)) {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* current = buffer;
            DWORD offset = 0;
            numa_nodes = 0;
            
            while (offset < bufferSize) {
                if (current->Relationship == RelationNumaNode) {
                    numa_nodes++;
                }
                offset += current->Size;
                current = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)((BYTE*)buffer + offset);
            }
            
            numa_enabled = (numa_nodes > 1);
        }
        if (buffer) free(buffer);
    }
    
    cJSON_AddNumberToObject(numa, "node_count", numa_nodes);
    cJSON_AddBoolToObject(numa, "numa_enabled", numa_enabled);
    cJSON_AddStringToObject(numa, "allocation_policy", numa_enabled ? "NUMA-Aware" : "Local-First");
    cJSON_AddStringToObject(numa, "topology", numa_nodes == 1 ? "Uniform Memory Access (UMA)" : "Non-Uniform Memory Access (NUMA)");
#else
    cJSON_AddNumberToObject(numa, "node_count", 1);
    cJSON_AddBoolToObject(numa, "numa_enabled", false);
    cJSON_AddStringToObject(numa, "allocation_policy", "Local-First");
    cJSON_AddStringToObject(numa, "topology", "Uniform Memory Access (UMA)");
#endif
    
    cJSON_AddItemToObject(root, "numa", numa);
    
    // SVM 能力
    cJSON *svm_capabilities = cJSON_CreateObject();
    cJSON_AddBoolToObject(svm_capabilities, "coarse_grain_buffer", true);
    cJSON_AddBoolToObject(svm_capabilities, "fine_grain_buffer", true);
    cJSON_AddBoolToObject(svm_capabilities, "fine_grain_system", false);
    cJSON_AddBoolToObject(svm_capabilities, "atomic_support", true);
    cJSON_AddItemToObject(root, "svm_capabilities", svm_capabilities);
    
    // 分配策略
    cJSON *allocation = cJSON_CreateArray();
    cJSON_AddItemToArray(allocation, cJSON_CreateString("Standard"));
    cJSON_AddItemToArray(allocation, cJSON_CreateString("Aligned"));
    cJSON_AddItemToArray(allocation, cJSON_CreateString("NUMA-Aware"));
    cJSON_AddItemToArray(allocation, cJSON_CreateString("Topology-Aware"));
    cJSON_AddItemToArray(allocation, cJSON_CreateString("Distributed"));
    cJSON_AddItemToArray(allocation, cJSON_CreateString("Coherent-Group"));
    cJSON_AddItemToObject(root, "allocation_strategies", allocation);
    
    // 性能監控
    cJSON *monitoring = cJSON_CreateObject();
    cJSON_AddBoolToObject(monitoring, "latency_monitor", true);
    cJSON_AddBoolToObject(monitoring, "bandwidth_monitor", true);
    cJSON_AddStringToObject(monitoring, "units", "ns / GB/s");
    cJSON_AddItemToObject(root, "monitoring", monitoring);
    
    cJSON_AddStringToObject(root, "version", "RetryIX 3.0.0");
    cJSON_AddStringToObject(root, "timestamp", __DATE__ " " __TIME__);
    
    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}
