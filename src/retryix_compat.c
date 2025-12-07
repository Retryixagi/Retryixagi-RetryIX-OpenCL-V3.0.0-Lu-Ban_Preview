// Compatibility wrappers: export legacy symbol names with the correct
// public signatures. Implementations build JSON directly from
// retryix_device_t and fill retryix_diagnostic_info_t to avoid calling
// helpers that have incompatible prototypes.

#include "retryix.h"
#include "retryix_device.h"
#include "retryix_utils.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

// Provide a small, portable implementation of retryix_safe_strcpy in case
// the project does not compile/emit it from another translation unit.
// Signature must match the header: RETRYIX_API retryix_result_t RETRYIX_CALL retryix_safe_strcpy(char*, const char*, size_t)
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return RETRYIX_ERROR_NULL_PTR;
    // ensure null-termination
    size_t to_copy = strlen(src);
    if (to_copy >= dest_size) {
        // copy as much as fits (leave room for null)
        if (dest_size > 0) {
            memcpy(dest, src, dest_size - 1);
            dest[dest_size - 1] = '\0';
        }
        return RETRYIX_ERROR_BUFFER_TOO_SMALL;
    }
    memcpy(dest, src, to_copy + 1);
    return RETRYIX_SUCCESS;
}

// retryix_device_to_json: produce a compact JSON representation
// of the provided retryix_device_t into the caller buffer.
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_device_to_json(
    const retryix_device_t* device,
    char* json_output,
    size_t max_json_len
) {
    if (!device || !json_output || max_json_len == 0) return RETRYIX_ERROR_NULL_PTR;

    int written = 0;
    size_t remaining = max_json_len;

    written = snprintf(json_output, remaining,
        "{\n  \"name\": \"%s\",\n  \"vendor\": \"%s\",\n  \"version\": \"%s\",\n",
        device->name, device->vendor, device->version);
    if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;
    remaining -= (size_t)written;

    written = snprintf(json_output + (max_json_len - remaining), remaining,
        "  \"driver_version\": \"%s\",\n  \"type\": %llu,\n  \"global_memory\": %llu,\n",
        device->driver_version,
        (unsigned long long)device->type,
        (unsigned long long)device->global_memory);
    if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;
    remaining -= (size_t)written;

    written = snprintf(json_output + (max_json_len - remaining), remaining,
        "  \"capabilities\": %u,\n  \"svm_capabilities\": %llu,\n",
        (unsigned int)device->capabilities,
        (unsigned long long)device->svm_capabilities);
    if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;
    remaining -= (size_t)written;

    written = snprintf(json_output + (max_json_len - remaining), remaining,
        "  \"performance_score\": %.2f,\n  \"is_available\": %d,\n  \"is_preferred\": %d,\n",
        device->performance_score,
        device->is_available,
        device->is_preferred);
    if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;
    remaining -= (size_t)written;

    written = snprintf(json_output + (max_json_len - remaining), remaining,
        "  \"custom_flags\": %u,\n  \"is_amd_rx5000\": %d\n}",
        device->custom_flags,
        device->is_amd_rx5000);
    if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;

    return RETRYIX_SUCCESS;
}

// retryix_check_device_compatibility: populate diagnostic info based
// on fields in retryix_device_t and perform simple heuristics.
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_device_compatibility(
    const retryix_device_t* device,
    retryix_diagnostic_info_t* diag
) {
    if (!device || !diag) return RETRYIX_ERROR_NULL_PTR;

    memset(diag, 0, sizeof(*diag));

    diag->opencl_available = 1;
    diag->platforms_found = 1;
    diag->devices_found = 1;

    if (device->capabilities & RETRYIX_CAP_SVM_FINE) diag->svm_support_level = 2;
    else if (device->capabilities & RETRYIX_CAP_SVM_COARSE) diag->svm_support_level = 1;
    else diag->svm_support_level = 0;

    diag->atomic_support = ((device->capabilities & (RETRYIX_CAP_ATOMIC_INT32|RETRYIX_CAP_ATOMIC_INT64)) != 0) ? 1 : 0;

    retryix_safe_strcpy(diag->driver_versions, device->driver_version, sizeof(diag->driver_versions));

    if (!device->is_available) {
        snprintf(diag->compatibility_issues, sizeof(diag->compatibility_issues),
                 "Device marked unavailable by runtime checks (performance_score=%.2f)",
                 device->performance_score);
        retryix_safe_strcpy(diag->recommendations, "Ensure drivers and runtimes are installed and accessible by this process.", sizeof(diag->recommendations));
        return RETRYIX_ERROR_INVALID_DEVICE;
    }

    // No issues found
    diag->compatibility_issues[0] = '\0';
    diag->recommendations[0] = '\0';

    return RETRYIX_SUCCESS;
}
