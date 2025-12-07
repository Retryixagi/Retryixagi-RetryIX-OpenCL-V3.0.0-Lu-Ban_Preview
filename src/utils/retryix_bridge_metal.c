// retryix_bridge_metal.h
#include "retryix_platform.h"
#ifdef __cplusplus
extern "C" {
#endif
RETRYIX_API retryix_result_t RETRYIX_CALL
retryix_discover_apple_metal_platforms(
  retryix_platform_t* platforms, int max_platforms, int* platform_count);

/* 共享記憶體（StorageModeShared）分配/釋放
   out_cpu_ptr  : [buffer contents]（CPU 指標）
   out_buf_handle: MTLBuffer* 的 opaque handle（void*），交給 free 使用 */
RETRYIX_API int RETRYIX_CALL
retryix_metal_alloc_shared(size_t bytes, size_t alignment,
                           void** out_cpu_ptr, void** out_buf_handle);
RETRYIX_API int RETRYIX_CALL
retryix_metal_free_shared(void* buf_handle);
#ifdef __cplusplus
}
#endif
