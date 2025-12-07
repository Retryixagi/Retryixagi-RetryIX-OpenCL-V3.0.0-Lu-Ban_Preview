
/* retryix_atomic.h
 * Portable, fine-grained atomic operations for RetryIX.
 * This header defines a backend-agnostic C API for host/SVM usage,
 * and establishes enums that map to device-side order/scope semantics.
 *
 * Backends (define exactly one when building the runtime/device wrappers):
 *   - RETRYIX_BACKEND_OPENCL
 *   - RETRYIX_BACKEND_CUDA
 *   - RETRYIX_BACKEND_HIP     (ROCm/HIP)  <-- ROCm first-class supported
 *   - RETRYIX_BACKEND_L0      (oneAPI Level Zero)
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#if __has_include("retryix_export.h")
  #include "retryix_export.h"
#else
  #include "retryix_export_fallback.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum retryix_mem_order_e {
  RETRYIX_ORDER_RELAXED = 0,
  RETRYIX_ORDER_ACQUIRE = 1,
  RETRYIX_ORDER_RELEASE = 2,
  RETRYIX_ORDER_ACQ_REL = 3,
  RETRYIX_ORDER_SEQ_CST = 4
} retryix_mem_order;

typedef enum retryix_mem_scope_e {
  RETRYIX_SCOPE_WI      = 0,  /* work-item / thread */
  RETRYIX_SCOPE_WG      = 1,  /* work-group / block */
  RETRYIX_SCOPE_DEVICE  = 2,  /* device */
  RETRYIX_SCOPE_ALL_SVM = 3   /* cross-device system / SVM */
} retryix_mem_scope;

/* Integer atomics */
RETRYIX_API int32_t  RETRYIX_CALL retryix_atomic_fetch_add_i32 (volatile int32_t*  p, int32_t  v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_fetch_add_u32 (volatile uint32_t* p, uint32_t v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API int64_t  RETRYIX_CALL retryix_atomic_fetch_add_i64 (volatile int64_t*  p, int64_t  v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API uint64_t RETRYIX_CALL retryix_atomic_fetch_add_u64 (volatile uint64_t* p, uint64_t v, retryix_mem_order mo, retryix_mem_scope sc);

RETRYIX_API int32_t  RETRYIX_CALL retryix_atomic_exchange_i32  (volatile int32_t*  p, int32_t  v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API int64_t  RETRYIX_CALL retryix_atomic_exchange_i64  (volatile int64_t*  p, int64_t  v, retryix_mem_order mo, retryix_mem_scope sc);

RETRYIX_API int      RETRYIX_CALL retryix_atomic_compare_exchange_i32(
  volatile int32_t* p, int32_t* expected, int32_t desired,
  retryix_mem_order mo_success, retryix_mem_order mo_fail, retryix_mem_scope sc);
RETRYIX_API int      RETRYIX_CALL retryix_atomic_compare_exchange_i64(
  volatile int64_t* p, int64_t* expected, int64_t desired,
  retryix_mem_order mo_success, retryix_mem_order mo_fail, retryix_mem_scope sc);

/* Bitwise & min/max */
RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_fetch_and_u32 (volatile uint32_t* p, uint32_t v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_fetch_or_u32  (volatile uint32_t* p, uint32_t v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_fetch_xor_u32 (volatile uint32_t* p, uint32_t v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_min_u32       (volatile uint32_t* p, uint32_t v, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API uint32_t RETRYIX_CALL retryix_atomic_max_u32       (volatile uint32_t* p, uint32_t v, retryix_mem_order mo, retryix_mem_scope sc);

/* Floating */
RETRYIX_API float    RETRYIX_CALL retryix_atomic_fetch_add_f32 (volatile float*    p, float    v, retryix_mem_order mo, retryix_mem_scope sc);
/* f64 as optional */
RETRYIX_API double   RETRYIX_CALL retryix_atomic_fetch_add_f64 (volatile double*   p, double   v, retryix_mem_order mo, retryix_mem_scope sc);

/* Host↔Device SVM synchronization helpers (host-side) */
RETRYIX_API void     RETRYIX_CALL retryix_svm_signal_u32(volatile uint32_t* flag, uint32_t value, retryix_mem_order mo, retryix_mem_scope sc);
RETRYIX_API uint32_t RETRYIX_CALL retryix_svm_wait_u32  (volatile const uint32_t* flag, uint32_t expected, retryix_mem_order mo, retryix_mem_scope sc, uint64_t timeout_ns);

/* Optional: subgroup-aggregated helpers will live in SDK (header-only) */ 

#ifdef __cplusplus
} /* extern "C" */
#endif
