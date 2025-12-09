@echo off
REM 編譯VirtualDrv驅動程式 - 使用MSVC 2026 + WDK環境

echo 設定MSVC 2026 + WDK環境...

REM 設定VC Tools路徑 (MSVC 2026)
set VCToolsInstallDir=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207

REM 設定PATH
set PATH=%VCToolsInstallDir%\bin\Hostx64\x64;%PATH%

REM 設定INCLUDE路徑
set INCLUDE=%VCToolsInstallDir%\include;%INCLUDE%

REM 設定LIB路徑
set LIB=%VCToolsInstallDir%\lib\x64;%LIB%

REM 設定Windows SDK路徑
set WindowsSdkDir=C:\Program Files (x86)\Windows Kits\10\

echo 編譯VirtualDrv驅動程式...

REM 切換到驅動程式目錄
cd /d "%~dp0"

REM 編譯驅動程式
cl.exe /kernel /Zl /GS- /X /wd4819 /D_AMD64_ /D_WIN64 ^
    /I"%WindowsSdkDir%Include\10.0.26100.0\km" ^
    /I"%WindowsSdkDir%Include\10.0.26100.0\km\crt" ^
    /I"%WindowsSdkDir%Include\10.0.26100.0\shared" ^
    /I"%WindowsSdkDir%Include\wdf\kmdf\1.35" ^
    /I"..\realdrv" ^
    VirtualDrv.c ^
    /link ^
    /LIBPATH:"%WindowsSdkDir%Lib\10.0.26100.0\km\x64" ^
    /LIBPATH:"%WindowsSdkDir%Lib\wdf\kmdf\x64\1.35" ^
    /ENTRY:DriverEntry ^
    /DRIVER ^
    /SUBSYSTEM:NATIVE ^
    wdfdriverentry.lib ^
    wdfldr.lib ^
    ntoskrnl.lib ^
    hal.lib ^
    wmilib.lib ^
    BufferOverflowK.lib ^
    /OUT:VirtualDrv.sys

if %ERRORLEVEL% EQU 0 (
    echo ✓ VirtualDrv.sys 編譯成功！
    dir VirtualDrv.sys
) else (
    echo ✗ 編譯失敗！
)

echo.
pause