#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VirtualDrv 性能統計測試腳本
測試新的性能監控功能
"""

import ctypes
import struct
import time

# VirtualDrv IOCTL 定義
VIRTDRV_TYPE = 40100
IOCTL_VIRTDRV_SET_MODE = (0x80000000 | (VIRTDRV_TYPE << 16) | (0x900 << 2) | 0)
IOCTL_VIRTDRV_INVOKE_SPECIAL = (0x80000000 | (VIRTDRV_TYPE << 16) | (0x901 << 2) | 0)
IOCTL_VIRTDRV_QUERY_MODE = (0x80000000 | (VIRTDRV_TYPE << 16) | (0x902 << 2) | 0)
IOCTL_VIRTDRV_QUERY_STATS = (0x80000000 | (VIRTDRV_TYPE << 16) | (0x904 << 2) | 0)  # 新增

# 模式定義
VIRTDRV_MODE_SIMULATE = 0
VIRTDRV_MODE_FAIL = 1
VIRTDRV_MODE_FORWARD = 2

# 結構定義
class VIRTDRV_INVOKE_INPUT(ctypes.Structure):
    _fields_ = [
        ("CommandId", ctypes.c_uint32),
        ("Reserved", ctypes.c_uint32),
        ("Payload", ctypes.c_uint64)
    ]

class VIRTDRV_INVOKE_OUTPUT(ctypes.Structure):
    _fields_ = [
        ("Result", ctypes.c_int32),
        ("Info", ctypes.c_uint64)
    ]

class VIRTDRV_STATS(ctypes.Structure):
    _fields_ = [
        ("IoctlCount", ctypes.c_uint32),
        ("ForwardCount", ctypes.c_uint32),
        ("SimulateCount", ctypes.c_uint32),
        ("FailCount", ctypes.c_uint32)
    ]

def test_performance_stats():
    """測試性能統計功能"""
    print("🚀 VirtualDrv 性能統計測試")
    print("=" * 50)

    try:
        # 開啟驅動程式
        driver = ctypes.windll.kernel32.CreateFileW(
            "\\\\.\\VirtualDrv",
            0xC0000000,  # GENERIC_READ | GENERIC_WRITE
            0,  # 不共享
            None,
            3,  # OPEN_EXISTING
            0,
            None
        )

        if driver == -1:
            print("❌ 無法開啟 VirtualDrv 驅動程式")
            return False

        print("✅ 成功開啟 VirtualDrv 驅動程式")

        # 設定為模擬模式
        mode_buffer = ctypes.c_uint32(VIRTDRV_MODE_SIMULATE)
        bytes_returned = ctypes.c_uint32(0)

        result = ctypes.windll.kernel32.DeviceIoControl(
            driver,
            IOCTL_VIRTDRV_SET_MODE,
            ctypes.byref(mode_buffer),
            ctypes.sizeof(mode_buffer),
            None,
            0,
            ctypes.byref(bytes_returned),
            None
        )

        if not result:
            print("❌ 無法設定模擬模式")
            ctypes.windll.kernel32.CloseHandle(driver)
            return False

        print("✅ 設定為模擬模式")

        # 執行一些測試操作來產生統計數據
        print("\n📊 產生測試數據...")

        for i in range(10):
            # 模擬命令 0
            input_data = VIRTDRV_INVOKE_INPUT()
            input_data.CommandId = 0
            input_data.Payload = i

            output_data = VIRTDRV_INVOKE_OUTPUT()

            result = ctypes.windll.kernel32.DeviceIoControl(
                driver,
                IOCTL_VIRTDRV_INVOKE_SPECIAL,
                ctypes.byref(input_data),
                ctypes.sizeof(input_data),
                ctypes.byref(output_data),
                ctypes.sizeof(output_data),
                ctypes.byref(bytes_returned),
                None
            )

            if result:
                print(f"  命令 {i}: 成功 (Info: 0x{output_data.Info:016X})")
            else:
                print(f"  命令 {i}: 失敗")

            time.sleep(0.1)  # 小延遲

        # 查詢性能統計
        print("\n📈 查詢性能統計...")
        stats = VIRTDRV_STATS()

        result = ctypes.windll.kernel32.DeviceIoControl(
            driver,
            IOCTL_VIRTDRV_QUERY_STATS,
            None,
            0,
            ctypes.byref(stats),
            ctypes.sizeof(stats),
            ctypes.byref(bytes_returned),
            None
        )

        if result:
            print("✅ 性能統計查詢成功:")
            print(f"  IOCTL 總數: {stats.IoctlCount}")
            print(f"  轉發操作: {stats.ForwardCount}")
            print(f"  模擬操作: {stats.SimulateCount}")
            print(f"  失敗操作: {stats.FailCount}")
        else:
            print("❌ 性能統計查詢失敗")

        # 切換到失敗模式並測試
        print("\n🔄 切換到失敗模式...")
        mode_buffer = ctypes.c_uint32(VIRTDRV_MODE_FAIL)

        result = ctypes.windll.kernel32.DeviceIoControl(
            driver,
            IOCTL_VIRTDRV_SET_MODE,
            ctypes.byref(mode_buffer),
            ctypes.sizeof(mode_buffer),
            None,
            0,
            ctypes.byref(bytes_returned),
            None
        )

        if result:
            print("✅ 設定為失敗模式")

            # 執行失敗測試
            input_data = VIRTDRV_INVOKE_INPUT()
            input_data.CommandId = 0

            result = ctypes.windll.kernel32.DeviceIoControl(
                driver,
                IOCTL_VIRTDRV_INVOKE_SPECIAL,
                ctypes.byref(input_data),
                ctypes.sizeof(input_data),
                ctypes.byref(output_data),
                ctypes.sizeof(output_data),
                ctypes.byref(bytes_returned),
                None
            )

            # 再次查詢統計
            result = ctypes.windll.kernel32.DeviceIoControl(
                driver,
                IOCTL_VIRTDRV_QUERY_STATS,
                None,
                0,
                ctypes.byref(stats),
                ctypes.sizeof(stats),
                ctypes.byref(bytes_returned),
                None
            )

            if result:
                print("📈 更新後的性能統計:")
                print(f"  IOCTL 總數: {stats.IoctlCount}")
                print(f"  轉發操作: {stats.ForwardCount}")
                print(f"  模擬操作: {stats.SimulateCount}")
                print(f"  失敗操作: {stats.FailCount}")

        # 關閉驅動程式
        ctypes.windll.kernel32.CloseHandle(driver)
        print("\n✅ 測試完成")
        return True

    except Exception as e:
        print(f"❌ 測試過程中發生錯誤: {e}")
        return False

if __name__ == "__main__":
    success = test_performance_stats()
    exit(0 if success else 1)