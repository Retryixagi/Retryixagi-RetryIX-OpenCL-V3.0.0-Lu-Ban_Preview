#ifndef RETRYIX_HOST_H
#define RETRYIX_HOST_H

#include "retryix_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 RetryIX Host（使用內建 fallback kernel）
 * @return 0 成功，非 0 失敗
 */
RETRYIX_API int RETRYIX_CALL retryix_init_minimal(void);

/**
 * @brief 從文件初始化 RetryIX Host
 * @param kernel_path 核心文件路徑
 * @param build_opts 編譯選項
 * @return 0 成功，非 0 失敗
 */
RETRYIX_API int RETRYIX_CALL retryix_init_from_file(const char* kernel_path, const char* build_opts);

/**
 * @brief 發送命令到 RetryIX Host
 * @param message 要發送的命令
 * @param response 回應緩衝區
 * @param response_size 回應緩衝區大小
 * @return 0 成功，非 0 失敗
 */
RETRYIX_API int RETRYIX_CALL retryix_send_command(const char* message, char* response, size_t response_size);

/**
 * @brief 關閉 RetryIX Host (host-specific)
 * @return 0 成功，非 0 失敗
 */
RETRYIX_API int RETRYIX_CALL retryix_host_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RETRYIX_HOST_H */
