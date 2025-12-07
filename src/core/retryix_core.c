
#include "retryix_core.h"

static int g_retryix_initialized = 0;

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_initialize(void) {
	if (g_retryix_initialized) {
		return RETRYIX_SUCCESS;
	}
	// 這裡可加載 OpenCL/資源等初始化
	g_retryix_initialized = 1;
	return RETRYIX_SUCCESS;
}

RETRYIX_API void RETRYIX_CALL retryix_cleanup(void) {
	// 釋放資源
	g_retryix_initialized = 0;
}

RETRYIX_API const char* RETRYIX_CALL retryix_get_version(int* major_version, int* minor_version, int* patch_version) {
	if (major_version) *major_version = RETRYIX_VERSION_MAJOR;
	if (minor_version) *minor_version = RETRYIX_VERSION_MINOR;
	if (patch_version) *patch_version = RETRYIX_VERSION_PATCH;
	return RETRYIX_VERSION_STRING;
}

RETRYIX_API const char* RETRYIX_CALL retryix_get_error_string(retryix_result_t error_code) {
	switch (error_code) {
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
		case RETRYIX_ERROR_OUT_OF_MEMORY: return "Out of memory";
		case RETRYIX_ERROR_KERNEL_COMPILATION: return "Kernel compilation error";
		case RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED: return "Atomic not supported";
		case RETRYIX_ERROR_UNKNOWN: return "Unknown error";
		default: return "Unrecognized error code";
	}
}
