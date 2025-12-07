#include "retryix_bridge_opencl_intel.h"
#include <string.h>

// 動態載入 OpenCL，簡易範例（實際需補全能力偵測）

RETRYIX_API retryix_result_t RETRYIX_CALL
retryix_discover_opencl_intel_platforms(
  retryix_platform_t* platforms,
  int max_platforms,
  int* platform_count)
{
  (void)platforms;
  (void)max_platforms;
  // 假設無 Intel OpenCL，回傳 0
  if (platform_count) *platform_count = 0;
  return RETRYIX_SUCCESS;
}
