// retryix_error.c
// 錯誤處理模組
#include <stdio.h>
#include "retryix_core.h"


// 將錯誤碼轉換為可讀字串
const char* retryix_strerror(retryix_result_t err) {
	switch (err) {
		case RETRYIX_SUCCESS: return "Success";
		case RETRYIX_ERROR_NULL_PTR: return "Null pointer";
		case RETRYIX_ERROR_NO_DEVICE: return "No device found";
		case RETRYIX_ERROR_NO_PLATFORM: return "No platform found";
		case RETRYIX_ERROR_OPENCL: return "OpenCL error";
		case RETRYIX_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
		case RETRYIX_ERROR_FILE_IO: return "File I/O error";
		case RETRYIX_ERROR_SVM_NOT_SUPPORTED: return "SVM not supported";
		case RETRYIX_ERROR_INVALID_DEVICE: return "Invalid device";
		case RETRYIX_ERROR_INVALID_PARAMETER: return "Invalid parameter";
		case RETRYIX_ERROR_KERNEL_COMPILATION: return "Kernel compilation error";
		case RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED: return "Atomic not supported";
		case RETRYIX_ERROR_OUT_OF_MEMORY: return "Out of memory";
		case RETRYIX_ERROR_UNKNOWN: return "Unknown error";
		default: return "Unrecognized error code";
	}
}

// 檢查錯誤並輸出訊息 (可用於 debug)
void retryix_check_error(retryix_result_t err, const char* context) {
	if (err != RETRYIX_SUCCESS) {
		fprintf(stderr, "[RetryIX Error] %s: %s (%d)\n", context ? context : "", retryix_strerror(err), err);
	}
}
