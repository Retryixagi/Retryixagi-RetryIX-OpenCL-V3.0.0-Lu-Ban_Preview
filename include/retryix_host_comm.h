
#pragma once

// 保證所有 API 皆為 C linkage，C++ 編譯器可正確連結
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MSG_QUEUE 128

typedef struct {
    char data[256];
    int length;
    int type;
} comm_packet_t;

typedef enum {
    COMM_SUCCESS = 0,
    COMM_ERROR_INIT = -1,
    COMM_ERROR_QUEUE_FULL = -2,
    COMM_ERROR_RECV = -3
} comm_result_t;


// int  retryix_host_comm_init(void); // 移除重複且型態不符宣告


// 主要 API，C/C++ 皆可用
comm_result_t retryix_host_comm_init(const char* channel_name);
void retryix_host_comm_cleanup(void);

comm_result_t comm_init(const char* channel_name);
comm_result_t comm_send(const comm_packet_t* packet);
comm_result_t comm_recv(comm_packet_t* out_packet);
void comm_cleanup();


#ifdef __cplusplus
} // extern "C"
#endif
