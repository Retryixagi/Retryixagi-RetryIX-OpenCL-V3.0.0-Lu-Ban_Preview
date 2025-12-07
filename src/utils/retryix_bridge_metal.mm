// retryix_bridge_metal.mm
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "retryix_bridge_metal.h"
#include <string.h>

static id<MTLDevice> rix_pick_device(void) {
#if TARGET_OS_OSX
  if (@available(macOS 10.11, *)) {
    NSArray<id<MTLDevice>>* devs = MTLCopyAllDevices();
    if (devs.count > 0) return devs.firstObject;   // 簡單取第一張，可擴充策略
  }
#endif
  return MTLCreateSystemDefaultDevice();
}

static BOOL rix_has_unified(id<MTLDevice> dev) {
  if (!dev) return NO;
  if (@available(macOS 10.15, iOS 13.0, *)) {
    return dev.hasUnifiedMemory;   // Apple Silicon 為 YES
  }
  // 舊版無此屬性，保守回 NO
  return NO;
}

extern "C" RETRYIX_API retryix_result_t RETRYIX_CALL
retryix_discover_apple_metal_platforms(
  retryix_platform_t* platforms, int max_platforms, int* platform_count)
{
  if (!platforms || !platform_count || max_platforms <= 0)
    return RETRYIX_ERROR_NULL_PTR;

  int out = 0;
#if TARGET_OS_OSX
  if (@available(macOS 10.11, *)) {
    NSArray<id<MTLDevice>>* devs = MTLCopyAllDevices();
    for (id<MTLDevice> dev in devs) {
      if (out >= max_platforms) break;
      retryix_platform_t* p = &platforms[out];
      memset(p, 0, sizeof(*p));
      snprintf(p->vendor,  sizeof(p->vendor),  "Apple");
      snprintf(p->name,    sizeof(p->name),    "%s", dev.name.UTF8String);
      snprintf(p->version, sizeof(p->version), "Metal unified:%d lowPower:%d",
               (int)rix_has_unified(dev), (int)dev.isLowPower);
      snprintf(p->profile, sizeof(p->profile), "Metal");
      p->device_count = 1;
      ++out;
    }
  }
#endif
  if (out == 0) {
    id<MTLDevice> dev = rix_pick_device();
    if (!dev) { *platform_count = 0; return RETRYIX_ERROR_NO_PLATFORM; }
    retryix_platform_t* p = &platforms[0];
    memset(p, 0, sizeof(*p));
    snprintf(p->vendor,  sizeof(p->vendor),  "Apple");
    snprintf(p->name,    sizeof(p->name),    "%s", dev.name.UTF8String);
    snprintf(p->version, sizeof(p->version), "Metal unified:%d lowPower:%d",
             (int)rix_has_unified(dev), (int)dev.isLowPower);
    snprintf(p->profile, sizeof(p->profile), "Metal");
    p->device_count = 1;
    out = 1;
  }
  *platform_count = out;
  return RETRYIX_SUCCESS;
}

extern "C" RETRYIX_API int RETRYIX_CALL
retryix_metal_alloc_shared(size_t bytes, size_t alignment,
                           void** out_cpu_ptr, void** out_buf_handle)
{
  if (!out_cpu_ptr || !out_buf_handle) return -1;
  id<MTLDevice> dev = rix_pick_device();
  if (!dev) return -2;

  MTLResourceOptions opt = MTLResourceStorageModeShared; // Unified memory
  id<MTLBuffer> buf = [dev newBufferWithLength:bytes options:opt];
  if (!buf) return -3;

  void* cpu = [buf contents];
  if (!cpu) { [buf release]; return -4; }

  // 記錄對齊情形：Metal 一般會提供頁對齊，但不能保證指定 alignment
  // 你的 codec 若需要嚴格 4KB，可在 kernel 上用 misalign 參數處理。
  (void)alignment;

  // 交出所有權給呼叫端（opaque handle）
  *out_cpu_ptr   = cpu;
  *out_buf_handle = (__bridge_retained void*)buf; // retain 以便跨 C 邊界
  return 0;
}

extern "C" RETRYIX_API int RETRYIX_CALL
retryix_metal_free_shared(void* buf_handle)
{
  if (!buf_handle) return -1;
  id<MTLBuffer> buf = (__bridge_transfer id<MTLBuffer>)buf_handle; // 交回 ARC 管理並釋放
  buf = nil;
  return 0;
}
