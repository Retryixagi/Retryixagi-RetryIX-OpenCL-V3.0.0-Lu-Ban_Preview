
// retryix_platform.c
// 平台查詢實現
#include "retryix_opencl_compat.h"
#include "retryix_device.h"
#include <string.h>

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
) {
    if (!platforms || !platform_count || max_platforms <= 0)
        return RETRYIX_ERROR_NULL_PTR;

    cl_uint cl_platforms_count = 0;
    cl_int cl_status = clGetPlatformIDs(0, NULL, &cl_platforms_count);
    if (cl_status != CL_SUCCESS || cl_platforms_count == 0)
        return RETRYIX_ERROR_NO_PLATFORM;

    cl_platforms_count = (cl_platforms_count > (cl_uint)max_platforms) ? (cl_uint)max_platforms : cl_platforms_count;
    cl_platform_id cl_platforms[RETRYIX_MAX_PLATFORMS] = {0};
    cl_status = clGetPlatformIDs(cl_platforms_count, cl_platforms, NULL);
    if (cl_status != CL_SUCCESS)
        return RETRYIX_ERROR_OPENCL;

    for (cl_uint i = 0; i < cl_platforms_count; ++i) {
        retryix_platform_t* plat = &platforms[i];
        memset(plat, 0, sizeof(retryix_platform_t));
        plat->id = cl_platforms[i];

        clGetPlatformInfo(plat->id, CL_PLATFORM_NAME, sizeof(plat->name), plat->name, NULL);
        clGetPlatformInfo(plat->id, CL_PLATFORM_VENDOR, sizeof(plat->vendor), plat->vendor, NULL);
        clGetPlatformInfo(plat->id, CL_PLATFORM_VERSION, sizeof(plat->version), plat->version, NULL);
        clGetPlatformInfo(plat->id, CL_PLATFORM_PROFILE, sizeof(plat->profile), plat->profile, NULL);
        clGetPlatformInfo(plat->id, CL_PLATFORM_EXTENSIONS, sizeof(plat->extensions), plat->extensions, NULL);

        // 查詢設備數量
        cl_uint dev_count = 0;
        clGetDeviceIDs(plat->id, CL_DEVICE_TYPE_ALL, 0, NULL, &dev_count);
        plat->device_count = (int)dev_count;
    }

    *platform_count = (int)cl_platforms_count;
    return RETRYIX_SUCCESS;
}
