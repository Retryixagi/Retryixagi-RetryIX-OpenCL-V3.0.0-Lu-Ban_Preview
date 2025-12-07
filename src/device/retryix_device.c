
#include "retryix_opencl_compat.h"
#include "retryix_device.h"
// Implementations that populate retryix_device_t using simulation.
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
// Bridge headers for vendor-specific fallback discovery
#include "retryix_bridge_cuda.h"
#include "retryix_bridge_rocm.h"
#include "retryix_bridge_intel_l0.h"
#include "retryix_bridge_opencl_intel.h"
#ifdef __APPLE__
#include "retryix_bridge_metal.h"
#else
/* Provide a weak prototype so the bridge can be called if present at link time on non-Apple builds */
extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_apple_metal_platforms(
	retryix_platform_t* platforms, int max_platforms, int* platform_count);
#endif

/* Runtime symbol resolver: provide a single file-scope helper so we avoid
   defining functions inside other functions (MSVC does not support nested
   function definitions). This helper uses GetProcAddress on Windows and
   dlsym on POSIX systems. */
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static void* rix_get_sym(const char* name) {
#if defined(_WIN32)
	HMODULE m = GetModuleHandleA(NULL);
	return m ? (void*)GetProcAddress(m, name) : NULL;
#else
	return dlsym(RTLD_DEFAULT, name);
#endif
}

/* Typedef for bridge discover functions */
typedef retryix_result_t (*discover_fn)(retryix_platform_t*, int, int*);

// Simple debug logger enabled by setting the environment variable RETRYIX_DEBUG (any value)
#define DEBUG_LOG(fmt, ...) do { if (getenv("RETRYIX_DEBUG")) fprintf(stderr, fmt, ##__VA_ARGS__); } while(0)

// Fallback file logger (append) to ensure logs are captured even if stderr/env isn't available
static void device_file_log(const char* fmt, ...) {
	const char* path = getenv("RETRYIX_DEBUG_FILE");
	char* buf = NULL;
	if (!path || !path[0]) {
		const char* tmpdir = NULL;
		// prefer RETRYIX_TMP, then TMP, then TEMP
		tmpdir = getenv("RETRYIX_TMP");
		if (!tmpdir) tmpdir = getenv("TMP");
		if (!tmpdir) tmpdir = getenv("TEMP");
		if (tmpdir && tmpdir[0]) {
			size_t n = strlen(tmpdir) + 1 + strlen("device_debug.log") + 1;
			buf = (char*)malloc(n);
			if (buf) {
				// ensure separator
				if (tmpdir[strlen(tmpdir)-1] == '/' || tmpdir[strlen(tmpdir)-1] == '\\')
					snprintf(buf, n, "%sdevice_debug.log", tmpdir);
				else
					snprintf(buf, n, "%s/%s", tmpdir, "device_debug.log");
				path = buf;
			} else {
				path = "device_debug.log";
			}
		} else {
			path = "device_debug.log"; // relative to working directory
		}
	}
	FILE* f = fopen(path, "a");
	if (!f) {
		int e = errno;
		DEBUG_LOG("[device_file_log] fopen('%s') failed errno=%d (%s)\n", path, e, strerror(e));
		fprintf(stderr, "[device_file_log] fopen('%s') failed errno=%d (%s)\n", path, e, strerror(e));
		return;
	}
	va_list ap;
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fclose(f);
	if (buf) free(buf);
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_devices_for_platform(
	cl_platform_id platform_id,
	retryix_device_t* devices,
	int max_devices,
	int* device_count
) {
	if (!devices || !device_count || max_devices <= 0) return RETRYIX_ERROR_NULL_PTR;
	*device_count = 0;

	cl_uint num_devices = 0;
	device_file_log("[retryix_discover_devices_for_platform] entry platform_id=%p\n", (void*)platform_id);
	cl_int status = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
	DEBUG_LOG("[retryix_discover_devices_for_platform] platform_id=%p clGetDeviceIDs(status=%d) num_devices=%u\n", (void*)platform_id, (int)status, (unsigned)num_devices);
	device_file_log("[retryix_discover_devices_for_platform] platform_id=%p clGetDeviceIDs(status=%d) num_devices=%u\n", (void*)platform_id, (int)status, (unsigned)num_devices);
	if (status != CL_SUCCESS || num_devices == 0) {
		return RETRYIX_ERROR_NO_DEVICE;
	}

	// limit to max_devices
	cl_uint to_fetch = (num_devices > (cl_uint)max_devices) ? (cl_uint)max_devices : num_devices;
	cl_device_id *devs = (cl_device_id*)malloc(sizeof(cl_device_id) * to_fetch);
	if (!devs) return RETRYIX_ERROR_UNKNOWN;
	status = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, to_fetch, devs, NULL);
	DEBUG_LOG("[retryix_discover_devices_for_platform] fetched %u devices, clGetDeviceIDs(status=%d)\n", (unsigned)to_fetch, (int)status);
	device_file_log("[retryix_discover_devices_for_platform] fetched %u devices, clGetDeviceIDs(status=%d)\n", (unsigned)to_fetch, (int)status);
	if (status != CL_SUCCESS) { free(devs); return RETRYIX_ERROR_OPENCL; }

	int out = 0;
	for (cl_uint i = 0; i < to_fetch; ++i) {
		retryix_device_t* d = &devices[out];
		memset(d, 0, sizeof(*d));
		d->struct_size = sizeof(retryix_device_t);
		d->struct_version = 0x00010000;
		// device id
		d->id = devs[i];
		d->platform_id = platform_id;

		// queries
		char buf[2048];
		if (clGetDeviceInfo(devs[i], CL_DEVICE_NAME, sizeof(buf), buf, NULL) == CL_SUCCESS)
			strncpy(d->name, buf, RETRYIX_MAX_NAME_LEN-1);
		if (clGetDeviceInfo(devs[i], CL_DEVICE_VENDOR, sizeof(buf), buf, NULL) == CL_SUCCESS)
			strncpy(d->vendor, buf, RETRYIX_MAX_NAME_LEN-1);
		if (clGetDeviceInfo(devs[i], CL_DEVICE_VERSION, sizeof(buf), buf, NULL) == CL_SUCCESS)
			strncpy(d->version, buf, RETRYIX_MAX_VERSION_LEN-1);
		if (clGetDeviceInfo(devs[i], CL_DRIVER_VERSION, sizeof(buf), buf, NULL) == CL_SUCCESS)
			strncpy(d->driver_version, buf, RETRYIX_MAX_VERSION_LEN-1);
		if (clGetDeviceInfo(devs[i], CL_DEVICE_EXTENSIONS, sizeof(d->extensions), d->extensions, NULL) != CL_SUCCESS)
			d->extensions[0] = '\0';

		cl_device_type dtype = 0;
		clGetDeviceInfo(devs[i], CL_DEVICE_TYPE, sizeof(dtype), &dtype, NULL);
		d->type = dtype;

		cl_uint cu = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, NULL); d->compute_units = cu;
		cl_uint freq = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(freq), &freq, NULL); d->max_frequency = freq;
		cl_ulong glmem = 0; clGetDeviceInfo(devs[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(glmem), &glmem, NULL); d->global_memory = glmem;
		cl_ulong lmem = 0; clGetDeviceInfo(devs[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(lmem), &lmem, NULL); d->local_memory = lmem;
		size_t wg = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(wg), &wg, NULL); d->max_work_group_size = wg;
		cl_uint dims = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(dims), &dims, NULL); d->max_work_item_dimensions = dims;
		if (dims > 0) clGetDeviceInfo(devs[i], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(d->max_work_item_sizes), &d->max_work_item_sizes, NULL);

		// svm caps (if supported)
		cl_bitfield svm_caps = 0;
		clGetDeviceInfo(devs[i], CL_DEVICE_SVM_CAPABILITIES, sizeof(svm_caps), &svm_caps, NULL);
		d->svm_capabilities = svm_caps;

		// availability
		cl_bool avail = CL_FALSE; clGetDeviceInfo(devs[i], CL_DEVICE_AVAILABLE, sizeof(avail), &avail, NULL); d->is_available = (avail == CL_TRUE) ? 1 : 0;

		d->performance_score = 50.0f; // placeholder
		d->is_preferred = 0;
		d->custom_flags = 0;
		d->is_amd_rx5000 = 0;

	DEBUG_LOG("[retryix_discover_devices_for_platform] device[%u]=%p name='%s' available=%d\n", (unsigned)i, (void*)devs[i], d->name, d->is_available);
	device_file_log("[retryix_discover_devices_for_platform] device[%u]=%p name='%s' available=%d\n", (unsigned)i, (void*)devs[i], d->name, d->is_available);
	out++;
	}

	free(devs);
	*device_count = out;
	return (out > 0) ? RETRYIX_SUCCESS : RETRYIX_ERROR_NO_DEVICE;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_all_devices(
	retryix_device_t* devices,
	int max_devices,
	int* device_count
) {
	if (!devices || !device_count || max_devices <= 0) return RETRYIX_ERROR_NULL_PTR;
	*device_count = 0;

	cl_uint num_platforms = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &num_platforms);
	DEBUG_LOG("[retryix_discover_all_devices] clGetPlatformIDs(status=%d) num_platforms=%u\n", (int)status, (unsigned)num_platforms);
	if (status != CL_SUCCESS || num_platforms == 0) return RETRYIX_ERROR_NO_PLATFORM;

	cl_platform_id *platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
	if (!platforms) return RETRYIX_ERROR_UNKNOWN;
	status = clGetPlatformIDs(num_platforms, platforms, NULL);
	DEBUG_LOG("[retryix_discover_all_devices] clGetPlatformIDs(fetch) status=%d\n", (int)status);
	if (status != CL_SUCCESS) { free(platforms); return RETRYIX_ERROR_OPENCL; }

	int out = 0;
	for (cl_uint p = 0; p < num_platforms && out < max_devices; ++p) {
	cl_uint num_devices = 0;
	device_file_log("[retryix_discover_all_devices] platform[%u]=%p entry\n", (unsigned)p, (void*)platforms[p]);
	status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
	DEBUG_LOG("[retryix_discover_all_devices] platform[%u]=%p clGetDeviceIDs(status=%d) num_devices=%u\n", (unsigned)p, (void*)platforms[p], (int)status, (unsigned)num_devices);
	device_file_log("[retryix_discover_all_devices] platform[%u]=%p clGetDeviceIDs(status=%d) num_devices=%u\n", (unsigned)p, (void*)platforms[p], (int)status, (unsigned)num_devices);
	if (status != CL_SUCCESS || num_devices == 0) continue;

	cl_device_id *devs = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
		if (!devs) continue;
		status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, num_devices, devs, NULL);
	DEBUG_LOG("[retryix_discover_all_devices] platform[%u] fetched %u devices, clGetDeviceIDs(status=%d)\n", (unsigned)p, (unsigned)num_devices, (int)status);
	device_file_log("[retryix_discover_all_devices] platform[%u] fetched %u devices, clGetDeviceIDs(status=%d)\n", (unsigned)p, (unsigned)num_devices, (int)status);
	if (status != CL_SUCCESS) { free(devs); continue; }

		for (cl_uint i = 0; i < num_devices && out < max_devices; ++i) {
			retryix_device_t* d = &devices[out];
			memset(d, 0, sizeof(*d));
			d->struct_size = sizeof(retryix_device_t);
			d->struct_version = 0x00010000;
			d->id = devs[i];
			d->platform_id = platforms[p];

			char buf[2048];
			if (clGetDeviceInfo(devs[i], CL_DEVICE_NAME, sizeof(buf), buf, NULL) == CL_SUCCESS)
				strncpy(d->name, buf, RETRYIX_MAX_NAME_LEN-1);
			if (clGetDeviceInfo(devs[i], CL_DEVICE_VENDOR, sizeof(buf), buf, NULL) == CL_SUCCESS)
				strncpy(d->vendor, buf, RETRYIX_MAX_NAME_LEN-1);
			if (clGetDeviceInfo(devs[i], CL_DEVICE_VERSION, sizeof(buf), buf, NULL) == CL_SUCCESS)
				strncpy(d->version, buf, RETRYIX_MAX_VERSION_LEN-1);
			if (clGetDeviceInfo(devs[i], CL_DRIVER_VERSION, sizeof(buf), buf, NULL) == CL_SUCCESS)
				strncpy(d->driver_version, buf, RETRYIX_MAX_VERSION_LEN-1);
			if (clGetDeviceInfo(devs[i], CL_DEVICE_EXTENSIONS, sizeof(d->extensions), d->extensions, NULL) != CL_SUCCESS)
				d->extensions[0] = '\0';

			cl_device_type dtype = 0; clGetDeviceInfo(devs[i], CL_DEVICE_TYPE, sizeof(dtype), &dtype, NULL); d->type = dtype;
			cl_uint cu = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, NULL); d->compute_units = cu;
			cl_uint freq = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(freq), &freq, NULL); d->max_frequency = freq;
			cl_ulong glmem = 0; clGetDeviceInfo(devs[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(glmem), &glmem, NULL); d->global_memory = glmem;
			cl_ulong lmem = 0; clGetDeviceInfo(devs[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(lmem), &lmem, NULL); d->local_memory = lmem;
			size_t wg = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(wg), &wg, NULL); d->max_work_group_size = wg;
			cl_uint dims = 0; clGetDeviceInfo(devs[i], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(dims), &dims, NULL); d->max_work_item_dimensions = dims;
			if (dims > 0) clGetDeviceInfo(devs[i], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(d->max_work_item_sizes), &d->max_work_item_sizes, NULL);

			cl_bitfield svm_caps = 0; clGetDeviceInfo(devs[i], CL_DEVICE_SVM_CAPABILITIES, sizeof(svm_caps), &svm_caps, NULL); d->svm_capabilities = svm_caps;
			cl_bool avail = CL_FALSE; clGetDeviceInfo(devs[i], CL_DEVICE_AVAILABLE, sizeof(avail), &avail, NULL); d->is_available = (avail == CL_TRUE) ? 1 : 0;

			d->performance_score = 50.0f;
			d->is_preferred = 0;
			d->custom_flags = 0;
			d->is_amd_rx5000 = 0;

			DEBUG_LOG("[retryix_discover_all_devices] device out=%d platform=%u id=%p name='%s' available=%d\n", out, (unsigned)p, (void*)devs[i], d->name, d->is_available);
			device_file_log("[retryix_discover_all_devices] device out=%d platform=%u id=%p name='%s' available=%d\n", out, (unsigned)p, (void*)devs[i], d->name, d->is_available);
			out++;
		}
		free(devs);
	}


	free(platforms);

		// If OpenCL discovered nothing, try vendor-specific bridge fallbacks
		if (out == 0) {
			device_file_log("[retryix_discover_all_devices] OpenCL found no devices; trying vendor bridge fallbacks\n");
			retryix_platform_t plats[16];
			int pc = 0;

			/* runtime bridge discovery: rix_get_sym and discover_fn are defined
			   at file scope above to avoid nested function definitions and
			   repeated includes. */

			#define TRY_BRIDGE_NAME(symname) \
			do { \
				pc = 0; memset(plats, 0, sizeof(plats)); \
				discover_fn fn = (discover_fn)rix_get_sym(symname); \
				if (fn) { \
					if (fn(plats, (int)(sizeof(plats)/sizeof(plats[0])), &pc) == RETRYIX_SUCCESS && pc > 0) { \
						for (int _i = 0; _i < pc && out < max_devices; ++_i) { \
							retryix_platform_t* pplat = &plats[_i]; \
							retryix_device_t* d = &devices[out]; \
							memset(d, 0, sizeof(*d)); \
							d->struct_size = sizeof(retryix_device_t); \
							d->struct_version = 0x00010000; \
							d->id = NULL; \
							d->platform_id = NULL; \
							strncpy(d->name, pplat->name, RETRYIX_MAX_NAME_LEN-1); \
							strncpy(d->vendor, pplat->vendor, RETRYIX_MAX_NAME_LEN-1); \
							strncpy(d->version, pplat->version, RETRYIX_MAX_VERSION_LEN-1); \
							d->type = CL_DEVICE_TYPE_GPU; /* best-effort guess */ \
							d->is_available = 1; \
							d->performance_score = 40.0f; \
							d->is_preferred = 0; \
							device_file_log("[retryix_discover_all_devices] bridge-added device name='%s' vendor='%s'\n", d->name, d->vendor); \
							out++; \
						} \
					} \
				} \
			} while (0)

			TRY_BRIDGE_NAME("retryix_discover_cuda_platforms");
			TRY_BRIDGE_NAME("retryix_discover_rocm_platforms");
			TRY_BRIDGE_NAME("retryix_discover_intel_l0_platforms");
			TRY_BRIDGE_NAME("retryix_discover_opencl_intel_platforms");
			TRY_BRIDGE_NAME("retryix_discover_apple_metal_platforms");

			#undef TRY_BRIDGE_NAME
		}

	*device_count = out;
	return (out > 0) ? RETRYIX_SUCCESS : RETRYIX_ERROR_NO_DEVICE;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_select_best_device(
	const retryix_device_t* devices,
	int device_count,
	retryix_device_t* best_device
) {
	if (!devices || !best_device || device_count <= 0) return RETRYIX_ERROR_NULL_PTR;
	return RETRYIX_ERROR_NO_DEVICE;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_filter_devices_by_type(
	const retryix_device_t* devices,
	int device_count,
	cl_device_type device_type,
	retryix_device_t* filtered_devices,
	int max_filtered,
	int* filtered_count
) {
	if (!devices || !filtered_devices || !filtered_count || device_count <= 0 || max_filtered <= 0) return RETRYIX_ERROR_NULL_PTR;
	*filtered_count = 0;
	return RETRYIX_ERROR_NO_DEVICE;
}

RETRYIX_API int RETRYIX_CALL retryix_device_supports_capability(
	const retryix_device_t* device,
	retryix_device_capabilities_t capability
) {
	if (!device) return 0;
	return 0;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_svm_support(
	const retryix_device_t* device,
	int* supports_coarse,
	int* supports_fine,
	int* supports_atomic
) {
	if (!device || !supports_coarse || !supports_fine || !supports_atomic) return RETRYIX_ERROR_NULL_PTR;
	*supports_coarse = 0;
	*supports_fine = 0;
	*supports_atomic = 0;
	return RETRYIX_ERROR_SVM_NOT_SUPPORTED;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_atomic_support(
	const retryix_device_t* device,
	int* supports_int32,
	int* supports_int64
) {
	if (!device || !supports_int32 || !supports_int64) return RETRYIX_ERROR_NULL_PTR;
	*supports_int32 = 0;
	*supports_int64 = 0;
	return RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_export_devices_json(
	const retryix_device_t* devices,
	int device_count,
	char* json_output,
	size_t max_json_len
) {
	if (!devices || !json_output || device_count <= 0 || max_json_len == 0) return RETRYIX_ERROR_NULL_PTR;

	size_t remaining = max_json_len;
	char* p = json_output;
	int written = 0;
	written = snprintf(p, remaining, "[");
	if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;
	p += written; remaining -= (size_t)written;

	for (int i = 0; i < device_count; ++i) {
		const retryix_device_t* d = &devices[i];
		written = snprintf(p, remaining,
			"{\"name\":\"%s\",\"vendor\":\"%s\",\"version\":\"%s\",\"driver_version\":\"%s\",\"type\":%llu,\"global_memory\":%llu,\"svm_capabilities\":%llu,\"performance_score\":%.2f}%s",
			d->name, d->vendor, d->version, d->driver_version,
			(unsigned long long)d->type, (unsigned long long)d->global_memory,
			(unsigned long long)d->svm_capabilities, d->performance_score,
			(i == device_count-1) ? "" : ",");
		if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;
		p += written; remaining -= (size_t)written;
	}

	written = snprintf(p, remaining, "]");
	if (written < 0 || (size_t)written >= remaining) return RETRYIX_ERROR_BUFFER_TOO_SMALL;
	return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_save_devices_to_file(
	const retryix_device_t* devices,
	int device_count,
	const char* filename
) {
	if (!devices || !filename || device_count <= 0) return RETRYIX_ERROR_NULL_PTR;
	return RETRYIX_ERROR_FILE_IO;
}
