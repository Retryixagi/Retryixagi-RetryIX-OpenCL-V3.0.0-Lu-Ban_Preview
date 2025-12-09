# 創建VirtualDrv根設備的PowerShell腳本

# 需要管理員權限運行
#Requires -RunAsAdministrator

Write-Host "創建VirtualDrv根設備..." -ForegroundColor Yellow

# 創建設備實例路徑
$devicePath = "HKLM:\SYSTEM\CurrentControlSet\Enum\Root\VirtualRetryix\0000"

# 檢查是否已存在
if (Test-Path $devicePath) {
    Write-Host "設備已存在，刪除舊設備..." -ForegroundColor Yellow
    Remove-Item -Path $devicePath -Recurse -Force
}

# 創建設備金鑰
New-Item -Path $devicePath -Force | Out-Null

# 設定設備屬性
New-ItemProperty -Path $devicePath -Name "DeviceDesc" -Value "RetryIX Virtual Driver (test layer)" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $devicePath -Name "HardwareID" -Value "Root\VirtualRetryix" -PropertyType MultiString -Force | Out-Null
New-ItemProperty -Path $devicePath -Name "CompatibleIDs" -Value @() -PropertyType MultiString -Force | Out-Null
New-ItemProperty -Path $devicePath -Name "ClassGUID" -Value "{4D36E978-E325-11CE-BFC1-08002BE10318}" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $devicePath -Name "Class" -Value "Sample" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $devicePath -Name "Driver" -Value "{4D36E978-E325-11CE-BFC1-08002BE10318}\0000" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $devicePath -Name "Mfg" -Value "RetryIX Test Drivers" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $devicePath -Name "Service" -Value "VirtualDrv" -PropertyType String -Force | Out-Null

# 創建LogConf子金鑰（如果需要）
$logConfPath = "$devicePath\LogConf"
New-Item -Path $logConfPath -Force | Out-Null

Write-Host "設備創建完成。現在重新掃描設備..." -ForegroundColor Green

# 重新掃描PnP設備
pnputil /scan-devices

Write-Host "完成！請檢查設備管理器中的'Sample'類別。" -ForegroundColor Green</content>
<parameter name="filePath">F:\1208\drivers\virtualdrv\create_device.ps1