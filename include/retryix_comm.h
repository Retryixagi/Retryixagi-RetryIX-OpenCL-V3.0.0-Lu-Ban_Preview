// retryix_comm.h
#pragma once

typedef enum {
    COMM_STATUS_OK = 0,
    COMM_STATUS_FAIL = -1,
    COMM_STATUS_TIMEOUT = -2
} CommStatus;

typedef void (*RetryIXCommCallback)(const char* topic, const char* message);

int retryix_comm_init(const char* config_path);
int retryix_comm_send(const char* topic, const char* message);
int retryix_comm_register(const char* topic, RetryIXCommCallback callback);
int retryix_comm_poll();  // 可阻塞等待或非阻塞查詢事件
int retryix_comm_shutdown();
