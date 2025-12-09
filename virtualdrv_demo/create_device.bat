@echo off
REM 創建VirtualDrv根設備的批處理文件
REM 需要以管理員身份運行

echo 創建VirtualDrv根設備...

REM 創建設備註冊表項
reg add "HKLM\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000" /f
reg add "HKLM\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000" /v DeviceDesc /t REG_SZ /d "RetryIX Virtual Driver (test layer)" /f
reg add "HKLM\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000" /v HardwareID /t REG_MULTI_SZ /d "Root\VirtualRetryix" /f
reg add "HKLM\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000" /v ClassGUID /t REG_SZ /d "{4D36E978-E325-11CE-BFC1-08002BE10318}" /f
reg add "HKLM\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000" /v Class /t REG_SZ /d "Sample" /f
reg add "HKLM\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000" /v Service /t REG_SZ /d "VirtualDrv" /f
reg add "HKLM\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000" /v Mfg /t REG_SZ /d "RetryIX Test Drivers" /f

echo 設備創建完成。現在重新掃描設備...

REM 重新掃描PnP設備（需要管理員權限）
pnputil /scan-devices

echo 完成！請檢查設備管理器中的"Sample"類別。
pause</content>
<parameter name="filePath">F:\1208\drivers\virtualdrv\create_device.bat