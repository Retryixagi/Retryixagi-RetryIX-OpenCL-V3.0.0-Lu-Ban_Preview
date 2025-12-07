
#ifndef RETRYIX_BENCHMARK_H
#define RETRYIX_BENCHMARK_H

#include <stdio.h>
#include "retryix_utils.h" // 只擴充 benchmark 結構

#ifdef __cplusplus
extern "C" {
#endif

// 擴充 benchmark 結構（如有需要）
typedef struct {
    retryix_benchmark_result_t base; // 官方 benchmark 結構
    char extra_details[256];         // 自訂擴充欄位
} my_benchmark_result_t;

// 自訂 helper function 範例
static inline void my_benchmark_print(const my_benchmark_result_t* result) {
    printf("Memory Bandwidth: %.2f GB/s\n", result->base.memory_bandwidth_gbps);
    printf("Compute GFLOPS: %.2f\n", result->base.compute_gflops);
    printf("Atomic Ops/sec: %.2f\n", result->base.atomic_ops_per_second);
    printf("SVM Latency: %.2f ns\n", result->base.svm_latency_ns);
    printf("Device Score: %d\n", result->base.device_score);
    printf("Details: %s\n", result->base.details);
    printf("Extra: %s\n", result->extra_details);
}

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_BENCHMARK_H
