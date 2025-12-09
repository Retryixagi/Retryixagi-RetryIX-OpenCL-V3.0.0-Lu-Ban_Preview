// VirtualDrv.h - small public header used by the test client
#pragma once
#include <ntdef.h>

#define VIRTDRV_TYPE 40100

// IOCTLs implemented by the virtual driver
#define IOCTL_VIRTDRV_SET_MODE CTL_CODE(VIRTDRV_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIRTDRV_INVOKE_SPECIAL CTL_CODE(VIRTDRV_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIRTDRV_QUERY_MODE CTL_CODE(VIRTDRV_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
// Set an NT device path to forward requests to (UNICODE wide string in the input buffer)
#define IOCTL_VIRTDRV_SET_TARGET CTL_CODE(VIRTDRV_TYPE, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
// 優化：添加性能統計查詢
#define IOCTL_VIRTDRV_QUERY_STATS CTL_CODE(VIRTDRV_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Mode values for the virtual driver
#define VIRTDRV_MODE_SIMULATE 0    // simulated responses
#define VIRTDRV_MODE_FAIL       1  // force failures (for tests)
#define VIRTDRV_MODE_FORWARD    2  // forward to real driver (optional)

typedef struct _VIRTDRV_SET_MODE_IN {
    UINT32 Mode;
} VIRTDRV_SET_MODE_IN;

typedef struct _VIRTDRV_INVOKE_INPUT {
    UINT32 CommandId; // numeric command identifier
    UINT32 Reserved;  // alignment / reserved
    UINT64 Payload;   // optional argument/opaque value (or small immediate)
    // NOTE: larger command inputs can be appended after this header in the user buffer
} VIRTDRV_INVOKE_INPUT;

typedef struct _VIRTDRV_INVOKE_OUTPUT {
    INT32 Result;     // public result code for tests
    UINT64 Info;      // extra information or debug value
} VIRTDRV_INVOKE_OUTPUT;

// 優化：性能統計結構
typedef struct _VIRTDRV_STATS {
    UINT32 IoctlCount;
    UINT32 ForwardCount;
    UINT32 SimulateCount;
    UINT32 FailCount;
} VIRTDRV_STATS;
