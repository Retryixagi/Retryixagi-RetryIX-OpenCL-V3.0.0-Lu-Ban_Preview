// retryix_benchmark.c
// 性能評測實現
#include "retryix_utils.h"

// ...性能評測相關函數...

#include <string.h>
#include <stddef.h>

// 性能評測主 API (kept internal by default)
retryix_result_t retryix_run_device_benchmark(
	const retryix_device_t* device,
	retryix_benchmark_result_t* benchmark_result
) {
	if (!device || !benchmark_result) return RETRYIX_ERROR_NULL_PTR;
	memset(benchmark_result, 0, sizeof(*benchmark_result));
	benchmark_result->memory_bandwidth_gbps = 12.5;
	benchmark_result->compute_gflops = 256.0;
	benchmark_result->atomic_ops_per_second = 1e6;
	benchmark_result->svm_latency_ns = 800;
	benchmark_result->device_score = 85;
	strncpy(benchmark_result->details, "Stub benchmark result", sizeof(benchmark_result->details)-1);
	return RETRYIX_SUCCESS;
}

// SVM demo stub (internal)
retryix_result_t retryix_run_svm_demo(
	const retryix_device_t* device,
	size_t allocation_size,
	int* test_passed
) {
	(void)allocation_size;
	if (!device || !test_passed) return RETRYIX_ERROR_NULL_PTR;
	*test_passed = 1;
	return RETRYIX_SUCCESS;
}

// 原子操作 demo stub (internal)
retryix_result_t retryix_run_atomic_demo(
	const retryix_device_t* device,
	int iterations,
	int* final_value,
	int* test_passed
) {
	if (!device || !final_value || !test_passed) return RETRYIX_ERROR_NULL_PTR;
	*final_value = iterations * 2;
	*test_passed = 1;
	return RETRYIX_SUCCESS;
}
