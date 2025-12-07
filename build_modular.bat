@echo off
setlocal
chcp 65001 >nul 2>&1

REM ========================================
REM RetryIX Production Build - 純模組化版本
REM 只使用 *_module.c 文件,簡化編譯流程
REM ========================================

setlocal EnableDelayedExpansion

set "MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
set "MSVC_CL=%MSVC_ROOT%\bin\Hostx64\x64\cl.exe"
set "MSVC_LINK=%MSVC_ROOT%\bin\Hostx64\x64\link.exe"

REM 編譯參數
REM Ensure we compile with RETRYIX_BUILD_DLL so public API symbols are exported
set CFLAGS=/c /Iinclude /Ithird_party\cJSON /I"%VULKAN_SDK%\Include" /DRETRYIX_BUILD_DLL /DRETRYIX_NO_OPENCL /D_CRT_SECURE_NO_WARNINGS /O2 /W3 /nologo /utf-8 /wd4392

echo ========================================
echo RetryIX 純模組化編譯
echo ========================================
echo.

REM 清理舊檔案
if exist obj rmdir /s /q obj
if exist lib\retryix.dll del /q lib\retryix.dll
if exist lib\retryix.lib del /q lib\retryix.lib
mkdir obj 2>nul
mkdir lib 2>nul

REM === 第三方庫 ===
echo [THIRD_PARTY] cJSON.c
"%MSVC_CL%" %CFLAGS% /Foobj\cJSON.obj third_party\cJSON\cJSON.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [THIRD_PARTY] cJSON_Utils.c
"%MSVC_CL%" %CFLAGS% /Foobj\cJSON_Utils.obj third_party\cJSON\cJSON_Utils.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR
echo.

REM === 8個核心模組 ===
echo [MODULE] atomic_module.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_atomic_module.obj src\modules\retryix_atomic_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [MODULE] bridge_module.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_bridge_module.obj src\modules\retryix_bridge_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [MODULE] core_module.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_core_module.obj src\modules\retryix_core_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [MODULE] device_module.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_device_module.obj src\modules\retryix_device_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [MODULE] kernel_module.c + Vulkan compute
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_kernel_module.obj src\modules\retryix_kernel_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [VULKAN] kernel_vulkan_compute.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_kernel_vulkan_compute.obj src\kernel\retryix_kernel_vulkan_compute.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [MODULE] system_module.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_system_module.obj src\modules\retryix_system_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [MODULE] topology_module.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_topology_module.obj src\modules\retryix_topology_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

echo [MODULE] zerocopy_module.c
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_zerocopy_module.obj src\modules\retryix_zerocopy_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

REM === 補充文件 (唯一不衝突的額外函數) ===
echo [EXTRA] retryix_control.c (6 control/query functions)
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_control.obj src\control\retryix_control.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

REM === 高級原子操作 (128/256-bit) ===
echo [ADVANCED] atomic_advanced_module.c (14 high-level atomic ops: 128/256-bit)
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_atomic_advanced_module.obj src\modules\retryix_atomic_advanced_module.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

REM === SVM-aware canonical 128/256-bit atomic implementation ===
echo [SVM] retryix_svm_atomic.c (canonical 128/256-bit implementation)
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_svm_atomic.obj src\svm\retryix_svm_atomic.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

REM === Utils Stubs - JSON 釋放支援 ===
echo [UTILS] retryix_stubs.c - 2 functions: free_json, placeholder
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_stubs.obj src\utils\retryix_stubs.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

REM === 多模態拓樸發現 - 網路/音訊/GPU JSON ===
echo [TOPOLOGY EXT] retryix_topology_ext.c - 3 functions: network/audio/multimodal JSON
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_topology_ext.obj src\topology\retryix_topology_ext.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

REM === GPU 硬體控制層 - Layer 0 寄存器級別控制 ===
echo [GPU HW] retryix_gpu_hw_windows.c - GPU register-level control with WinRing0
"%MSVC_CL%" %CFLAGS% /Foobj\retryix_gpu_hw_windows.obj src\device\retryix_gpu_hw_windows.c
if %errorlevel% neq 0 goto :CLEANUP_ERROR

REM === 以下與模組重複,無法添加 ===
REM retryix_api.c - 4個函數與 core_module 重複
REM retryix_core.c - 3個函數與 core_module 重複

REM === 以下有重複或依賴問題,暫時註釋 ===
REM retryix_kernel.c - 與 kernel_module 重複 retryix_mem_* 函數
REM retryix_memory_simple_new.c - 與 kernel_module 重複
REM retryix_kernel_builtin.c - 依賴 retryix_kernel_compile (未實現)

echo.
echo [COMPILE] 所有模組+補充文件編譯完成
echo.

REM === 連結 DLL ===
echo [LINK] 連結 retryix.dll...
"%MSVC_LINK%" /DLL /OUT:lib\retryix.dll ^
    /IMPLIB:lib\retryix.lib ^
    /NOLOGO /MACHINE:X64 ^
    obj\*.obj ^
    advapi32.lib dxgi.lib third_party\vulkan\vulkan-1.lib

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] 連結失敗
    goto :CLEANUP_ERROR
)

echo.
echo ========================================
echo 模組化編譯成功! (完整版:含多模態拓樸+128/256位元原子操作)
echo ========================================
echo.
echo DLL: lib\retryix.dll
echo LIB: lib\retryix.lib
echo.
echo 模組清單:
echo   [1] atomic_module.c         (13 functions: 32/64-bit)
echo   [2] atomic_advanced_module  (7 functions: 128/256-bit) [NEW]
echo   [3] bridge_module.c         (25 functions)
echo   [4] core_module.c           (9 functions)
echo   [5] device_module.c         (16 functions)
echo   [6] kernel_module.c         (12 functions)
echo   [7] system_module.c         (17 functions)
echo   [8] topology_module.c       (28 functions)
echo   [9] zerocopy_module.c       (29 functions)
echo   [+] control.c               (6 functions)
echo   [+] retryix_stubs.c         (2 functions: JSON free) [NEW]
echo   [+] retryix_topology_ext.c  (3 functions: multimodal topology JSON) [NEW]
echo   [+] cJSON + cJSON_Utils     (92 functions)
echo.
echo 預計導出: ~265 函數 (173 RetryIX + 92 cJSON)
echo.
goto :END

:CLEANUP_ERROR
echo.
echo [ERROR] 編譯失敗,清理中...
if exist obj rmdir /s /q obj
exit /b 1

:END
endlocal
