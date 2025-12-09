// VirtualDrv.c - A safe KMDF-style virtual driver layer used for tests
// The virtual driver supports a small set of IOCTLs that let user-mode code
// control the driver's mode (simulate / force fail / forward-to-real).
// This driver intentionally avoids any destructive operations and is intended
// for test environments only.

// KMDF driver definitions - prevent MSVC from auto-including user-mode CRT
#define _CRT_USE_WINAPI_FAMILY_DESKTOP_APP 1
#define _VCRUNTIME_DISABLED_WARNINGS 1

#pragma warning(disable:4005)   // 巨集重複定義
#pragma warning(disable:4083)   // 巨集格式不符
#pragma warning(disable:4819)   // 非950字碼頁

#include <ntddk.h>
#include <wdf.h>
#include "VirtualDrv.h"
#include "..\realdrv\RealDrv.h" // for IOCTL_REALDRV_* definitions

#ifndef IOCTL_MAP_PHYSICAL_MEMORY
#define IOCTL_MAP_PHYSICAL_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#ifndef IOCTL_UNMAP_PHYSICAL_MEMORY
#define IOCTL_UNMAP_PHYSICAL_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

// 優化：性能計數器變數 (結構已在標頭檔案中定義)
static VIRTDRV_STATS g_stats = {0};

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD VirtEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL VirtEvtIoDeviceControl;

static volatile LONG g_mode = VIRTDRV_MODE_SIMULATE;
static WDFIOTARGET g_forward_target = NULL;

// helper to translate a user-supplied device path (\.\Name) to the kernel-side DosDevices name
static
VOID
_translate_user_device_name_to_nt(_In_ wchar_t* src, _Out_ UNICODE_STRING* out) {
    // if src begins with "\\.\\" convert to "\\DosDevices\\"
    // caller must allocate or ensure src is NUL-terminated
    if (wcsncmp(src, L"\\\\.\\", 4) == 0) {
        wchar_t tmp[512];
        _snwprintf_s(tmp, sizeof(tmp)/sizeof(wchar_t), _TRUNCATE, L"\\DosDevices\\%s", src+4);
        RtlInitUnicodeString(out, tmp);
        return;
    }
    RtlInitUnicodeString(out, src);
}

// 優化：模組化IOCTL處理函數
static NTSTATUS _HandleSetMode(WDFREQUEST Request, size_t InputBufferLength) {
    VIRTDRV_SET_MODE_IN* in = NULL;
    if (InputBufferLength < sizeof(VIRTDRV_SET_MODE_IN) || InputBufferLength == 0) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    NTSTATUS status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIRTDRV_SET_MODE_IN), (PVOID*)&in, NULL);
    if (!NT_SUCCESS(status)) return status;

    // Atomic update for mode
    if (in->Mode == VIRTDRV_MODE_SIMULATE || in->Mode == VIRTDRV_MODE_FAIL || in->Mode == VIRTDRV_MODE_FORWARD) {
        InterlockedExchange(&g_mode, (LONG)in->Mode);
        return STATUS_SUCCESS;
    } else {
        return STATUS_INVALID_PARAMETER;
    }
}

static NTSTATUS _HandleQueryMode(WDFREQUEST Request) {
    LONG mode = InterlockedCompareExchange(&g_mode, g_mode, g_mode);
    LONG* out = NULL;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(Request, sizeof(LONG), (PVOID*)&out, NULL);
    if (!NT_SUCCESS(status)) return status;
    *out = mode;
    WdfRequestSetInformation(Request, sizeof(LONG));
    return STATUS_SUCCESS;
}

static NTSTATUS _HandleQueryStats(WDFREQUEST Request) {
    VIRTDRV_STATS* out = NULL;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(Request, sizeof(VIRTDRV_STATS), (PVOID*)&out, NULL);
    if (!NT_SUCCESS(status)) return status;

    // 原子讀取統計數據
    out->IoctlCount = InterlockedCompareExchange(&g_stats.IoctlCount, 0, 0);
    out->ForwardCount = InterlockedCompareExchange(&g_stats.ForwardCount, 0, 0);
    out->SimulateCount = InterlockedCompareExchange(&g_stats.SimulateCount, 0, 0);
    out->FailCount = InterlockedCompareExchange(&g_stats.FailCount, 0, 0);

    WdfRequestSetInformation(Request, sizeof(VIRTDRV_STATS));
    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WDF_DRIVER_CONFIG_INIT(&config, VirtEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("[VirtualDrv] WdfDriverCreate failed 0x%08X\n", status));
    }
    return status;
}

NTSTATUS VirtEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit) {
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("[VirtualDrv] WdfDeviceCreate failed 0x%08X\n", status));
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
    ioQueueConfig.EvtIoDeviceControl = VirtEvtIoDeviceControl;

    status = WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("[VirtualDrv] WdfIoQueueCreate failed 0x%08X\n", status));
        return status;
    }

    KdPrint(("[VirtualDrv] Virtual driver loaded - safe testing layer active\n"));
    return STATUS_SUCCESS;
}

VOID VirtEvtIoDeviceControl(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t OutputBufferLength,
                            _In_ size_t InputBufferLength, _In_ ULONG IoControlCode) {
    UNREFERENCED_PARAMETER(Queue);
    NTSTATUS status = STATUS_SUCCESS;
    size_t bytesReturned = 0;

    // 優化：更新IOCTL計數器
    InterlockedIncrement(&g_stats.IoctlCount);

    switch (IoControlCode) {
        case IOCTL_VIRTDRV_SET_MODE: {
            status = _HandleSetMode(Request, InputBufferLength);
            break;
        }

        case IOCTL_VIRTDRV_QUERY_MODE: {
            status = _HandleQueryMode(Request);
            break;
        }

        case IOCTL_VIRTDRV_QUERY_STATS: {
            status = _HandleQueryStats(Request);
            break;
        }

        case IOCTL_VIRTDRV_SET_TARGET: {
            // Input: wide-char (UTF-16) NUL-terminated device name (e.g. "\\\\.\\RetryixGpuDriver")
            PWCHAR name = NULL;
            status = WdfRequestRetrieveInputBuffer(Request, 2, (PVOID*)&name, NULL);
            if (!NT_SUCCESS(status)) break;

            // Convert to kernel-style UNICODE_STRING and attempt to open target
            UNICODE_STRING ntName;
            RtlZeroMemory(&ntName, sizeof(ntName));
            _translate_user_device_name_to_nt(name, &ntName);

            // Close previous target if present
            if (g_forward_target) {
                WdfIoTargetClose(g_forward_target);
                WdfObjectDelete(g_forward_target);
                g_forward_target = NULL;
            }

            // Create a new I/O target and attempt to open by name
            status = WdfIoTargetCreate(WdfIoQueueGetDevice(Queue), WDF_NO_OBJECT_ATTRIBUTES, &g_forward_target);
            if (!NT_SUCCESS(status)) {
                g_forward_target = NULL;
                break;
            }

            WDF_IO_TARGET_OPEN_PARAMS openParams;
            WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams, &ntName, (GENERIC_READ | GENERIC_WRITE));
            status = WdfIoTargetOpen(g_forward_target, &openParams);
            if (!NT_SUCCESS(status)) {
                WdfObjectDelete(g_forward_target);
                g_forward_target = NULL;
                break;
            }

            bytesReturned = 0;
            break;
        }

        case IOCTL_VIRTDRV_INVOKE_SPECIAL: {
            VIRTDRV_INVOKE_INPUT* in = NULL;
            VIRTDRV_INVOKE_OUTPUT* out = NULL;

            if (InputBufferLength < sizeof(VIRTDRV_INVOKE_INPUT) || OutputBufferLength < sizeof(VIRTDRV_INVOKE_OUTPUT)) {
                status = STATUS_BUFFER_TOO_SMALL; break;
            }

            status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIRTDRV_INVOKE_INPUT), (PVOID*)&in, NULL);
            if (!NT_SUCCESS(status)) break;
            status = WdfRequestRetrieveOutputBuffer(Request, sizeof(VIRTDRV_INVOKE_OUTPUT), (PVOID*)&out, NULL);
            if (!NT_SUCCESS(status)) break;

            // Behavior depends on current mode
            LONG mode = InterlockedCompareExchange(&g_mode, g_mode, g_mode);

            if (mode == VIRTDRV_MODE_FAIL) {
                InterlockedIncrement(&g_stats.FailCount);
                out->Result = -1; // test-visible failure
                out->Info = 0xDEADF00DBAD0C0DEULL;
                bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                break;
            }

            if (mode == VIRTDRV_MODE_SIMULATE) {
                InterlockedIncrement(&g_stats.SimulateCount);
                // Provide deterministic simulated behavior for a few command ids
                switch (in->CommandId) {
                    case 0:
                        out->Result = 0; out->Info = in->Payload ^ 0xBEEF; break;
                    case 1:
                        out->Result = 0; out->Info = 0xCAFEBABEULL; break;
                    case 2: {
                        // Delay for payload milliseconds (testing latency injection)
                        UINT32 ms = (UINT32)in->Payload;
                        LARGE_INTEGER interval;
                        interval.QuadPart = -(10 * 1000LL * (LONGLONG)ms); // ms -> 100ns units
                        KeDelayExecutionThread(KernelMode, FALSE, &interval);
                        out->Result = 0; out->Info = ms;
                        break;
                    }
                    case 3: {
                        // Inject a test error code encoded in payload (low 32 bits)
                        INT32 err = (INT32)(in->Payload & 0xFFFFFFFF);
                        out->Result = err;
                        out->Info = (UINT64)0xBADF00DULL;
                        break;
                    }
                    default:
                        out->Result = 0; out->Info = 0x0ULL; break;
                }
                bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                break;
            }

            if (mode == VIRTDRV_MODE_FORWARD) {
                InterlockedIncrement(&g_stats.ForwardCount);
                // Forward to real driver if a forward target has been configured
                if (!g_forward_target) {
                    out->Result = -3; // no forward target configured
                    out->Info = 0;
                    bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    break;
                }

                // Support a small set of real IOCTL mappings for demonstration
                if (in->CommandId == 12) {
                    // Map to IOCTL_REALDRV_ENUM_GPUS (no input, small output)
                    WDF_MEMORY_DESCRIPTOR outMem;
                    NTSTATUS rc;
                    UCHAR buffer[512] = {0};
                    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outMem, buffer, sizeof(buffer));

                    rc = WdfIoTargetSendIoctlSynchronously(g_forward_target, NULL, IOCTL_REALDRV_ENUM_GPUS, NULL, &outMem, NULL, NULL);
                    if (NT_SUCCESS(rc)) {
                        // Success — return a simple 64-bit checksum of the string as Info
                        UINT64 checksum = 1469598103934665603ULL; // FNV offset basis
                        for (size_t i = 0; i < sizeof(buffer) && buffer[i]; ++i) {
                            checksum ^= (UCHAR)buffer[i];
                            checksum *= 1099511628211ULL; // FNV prime
                        }
                        out->Result = 0;
                        out->Info = checksum;
                        bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    } else {
                        out->Result = (INT32)rc;
                        out->Info = 0;
                        bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    }
                    break;
                }

                // Forwarded low-level IOCTLs for real driver (examples)
                if (in->CommandId == 13) {
                    // READ PCI CONFIG: expect additional input bytes containing REALDRV_PCI_INPUT
                    if (InputBufferLength < sizeof(VIRTDRV_INVOKE_INPUT) + sizeof(ULONG)*4) {
                        out->Result = -5; out->Info = 0; bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT); break;
                    }
                    // extra data follows the header
                    PUCHAR base = (PUCHAR)in;
                    PUCHAR extra = base + sizeof(VIRTDRV_INVOKE_INPUT);
                    // interpret as four bytes: bus, device, function, offset
                    UCHAR bus = extra[0];
                    UCHAR device = extra[1];
                    UCHAR function = extra[2];
                    UCHAR offset = extra[3];

                    // build input for real driver IOCTL
                    typedef struct { UCHAR Bus; UCHAR Device; UCHAR Function; UCHAR Offset; } pci_in_t;
                    typedef struct { UINT32 Value; } pci_out_t;
                    pci_in_t inbuf = { bus, device, function, offset };
                    pci_out_t outbuf = {0};

                    WDF_MEMORY_DESCRIPTOR inMem, outMem;
                    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inMem, &inbuf, sizeof(inbuf));
                    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outMem, &outbuf, sizeof(outbuf));

                    NTSTATUS rc = WdfIoTargetSendIoctlSynchronously(g_forward_target, NULL, IOCTL_REALDRV_READ_PCI_CONFIG, &inMem, &outMem, NULL, NULL);
                    if (NT_SUCCESS(rc)) {
                        out->Result = 0; out->Info = (UINT64)outbuf.Value; bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    } else {
                        out->Result = (INT32)rc; out->Info = 0; bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    }
                    break;
                }

                if (in->CommandId == 14) {
                    // MAP_PHYSICAL: DANGEROUS - return safe error in virtual driver
                    // This would map arbitrary physical memory which can cause BSOD
                    out->Result = (INT32)STATUS_ACCESS_DENIED; // Safe denial
                    out->Info = 0xDEADBEEF; // Safe marker
                    bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    break;
                }

                if (in->CommandId == 16) {
                    // READ_PHYS_MEMORY: DANGEROUS - return safe simulated data
                    // This would read arbitrary physical memory which can cause BSOD
                    if (InputBufferLength < sizeof(VIRTDRV_INVOKE_INPUT) + sizeof(UINT64) + sizeof(UINT32)) {
                        out->Result = -5; out->Info = 0; bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT); break;
                    }

                    // Return safe simulated checksum instead of reading real physical memory
                    UINT64 hash = 0x123456789ABCDEF0ULL; // Safe simulated value
                    out->Result = 0;
                    out->Info = hash;
                    bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    break;
                }

                if (in->CommandId == 15) {
                    // UNMAP_PHYSICAL: DANGEROUS - return safe success
                    // This would unmap arbitrary memory which can cause BSOD
                    out->Result = 0; // Pretend success
                    out->Info = 0xCAFEBABE; // Safe marker
                    bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                    break;
                }

                // Unsupported forwarded command
                out->Result = -4;
                out->Info = 0;
                bytesReturned = sizeof(VIRTDRV_INVOKE_OUTPUT);
                break;
            }

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}
