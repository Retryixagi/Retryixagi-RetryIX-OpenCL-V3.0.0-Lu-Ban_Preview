#ifndef RETRYIX_BUILD_DLL
#define RETRYIX_BUILD_DLL_TEMP
#define RETRYIX_BUILD_DLL
#endif
#include "../../include/retryix_export.h"
#ifdef RETRYIX_BUILD_DLL_TEMP
#undef RETRYIX_BUILD_DLL
#undef RETRYIX_BUILD_DLL_TEMP
#endif
#include "../../include/retryix_svm.h"

// 所有 clean/free 相關 API 安全 stub，尚未支援，僅回傳 NOT_SUPPORTED

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_reset_statistics(retryix_svm_context_t* context) {
	(void)context;
	return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_pool_clear(retryix_svm_context_t* context) {
	(void)context;
	return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_pool_shrink(retryix_svm_context_t* context) {
	(void)context;
	return RETRYIX_SVM_ERROR_NOT_SUPPORTED;
}
