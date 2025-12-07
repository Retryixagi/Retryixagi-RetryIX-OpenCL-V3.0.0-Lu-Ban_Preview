#include "retryix_bridge_cpu_intel.h"
#include <string.h>

// CPUID 探測（簡易範例，實際需補全能力偵測）

RETRYIX_API retryix_result_t RETRYIX_CALL
retryix_discover_cpu_intel(
  retryix_platform_t* platforms,
  int max_platforms,
  int* platform_count)
{
  // 假設無 Intel CPU，回傳 0
  if (platform_count) *platform_count = 0;
  return RETRYIX_SUCCESS;
}
