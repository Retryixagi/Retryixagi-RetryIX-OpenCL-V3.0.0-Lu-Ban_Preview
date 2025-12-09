# VirtualDrv 現代化建置腳本
# 使用 PowerShell 提供更好的錯誤處理和配置管理

param(
    [switch]$Clean,
    [switch]$Verbose,
    [string]$Configuration = "Release"
)

# 建置配置
$BuildConfig = @{
    VCVersion = "14.44.35207"
    WindowsSDK = "10.0.26100.0"
    KMDFVersion = "1.35"
    OutputFile = "VirtualDrv.sys"
}

function Write-Status {
    param([string]$Message, [string]$Color = "White")
    if ($Verbose) {
        Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] $Message" -ForegroundColor $Color
    }
}

function Test-Prerequisites {
    Write-Status "檢查建置環境先決條件..." "Yellow"

    # 檢查 MSVC 安裝
    $vcPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\$($BuildConfig.VCVersion)"
    if (!(Test-Path $vcPath)) {
        throw "找不到 MSVC $BuildConfig.VCVersion。請確認 Visual Studio 安裝。"
    }

    # 檢查 Windows SDK
    $sdkPath = "C:\Program Files (x86)\Windows Kits\10\Include\$($BuildConfig.WindowsSDK)"
    if (!(Test-Path $sdkPath)) {
        throw "找不到 Windows SDK $BuildConfig.WindowsSDK。請安裝 Windows SDK。"
    }

    Write-Status "先決條件檢查通過" "Green"
}

function Set-BuildEnvironment {
    Write-Status "設定建置環境..." "Yellow"

    $vcToolsDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\$($BuildConfig.VCVersion)"
    $windowsSdkDir = "C:\Program Files (x86)\Windows Kits\10"

    # 設定環境變數
    $env:PATH = "$vcToolsDir\bin\Hostx64\x64;$env:PATH"
    $env:INCLUDE = "$vcToolsDir\include;$windowsSdkDir\Include\$($BuildConfig.WindowsSDK)\km;$windowsSdkDir\Include\$($BuildConfig.WindowsSDK)\km\crt;$windowsSdkDir\Include\$($BuildConfig.WindowsSDK)\shared;$windowsSdkDir\Include\wdf\kmdf\$($BuildConfig.KMDFVersion);$env:INCLUDE"
    $env:LIB = "$vcToolsDir\lib\x64;$windowsSdkDir\Lib\$($BuildConfig.WindowsSDK)\km\x64;$windowsSdkDir\Lib\wdf\kmdf\x64\$($BuildConfig.KMDFVersion);$env:LIB"

    Write-Status "建置環境設定完成" "Green"
}

function Invoke-Clean {
    Write-Status "清理建置檔案..." "Yellow"

    $filesToClean = @(
        "VirtualDrv.obj",
        "VirtualDrv.sys",
        "*.pdb",
        "*.log"
    )

    foreach ($file in $filesToClean) {
        if (Test-Path $file) {
            Remove-Item $file -Force
            Write-Status "已刪除: $file" "Gray"
        }
    }

    Write-Status "清理完成" "Green"
}

function Invoke-Build {
    Write-Status "開始編譯 VirtualDrv.sys..." "Yellow"

    $sourceFile = "VirtualDrv.c"
    $outputFile = $BuildConfig.OutputFile

    # 編譯參數
    $clArgs = @(
        "/kernel",           # 核心模式編譯
        "/Zl",              # 無預設程式庫
        "/GS-",             # 停用緩衝區檢查
        "/X",               # 忽略標準 include 路徑
        "/wd4819",          # 停用編碼警告
        "/D_AMD64_",        # AMD64 架構
        "/D_WIN64",         # 64位元 Windows
        "/I..\realdrv"      # 額外 include 路徑
    )

    # 連結參數
    $linkArgs = @(
        "/ENTRY:DriverEntry",
        "/DRIVER",
        "/SUBSYSTEM:NATIVE",
        "wdfdriverentry.lib",
        "wdfldr.lib",
        "ntoskrnl.lib",
        "hal.lib",
        "wmilib.lib",
        "BufferOverflowK.lib",
        "/OUT:$outputFile"
    )

    # 建置命令
    $buildCommand = "cl.exe $($clArgs -join ' ') $sourceFile /link $($linkArgs -join ' ')"

    if ($Verbose) {
        Write-Status "執行命令: $buildCommand" "Gray"
    }

    # 執行編譯
    $process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c $buildCommand" -NoNewWindow -Wait -PassThru

    if ($process.ExitCode -eq 0) {
        Write-Status "編譯成功！" "Green"

        # 顯示檔案資訊
        $fileInfo = Get-Item $outputFile
        Write-Status "輸出檔案: $($fileInfo.FullName)" "Cyan"
        Write-Status "檔案大小: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" "Cyan"
        Write-Status "修改時間: $($fileInfo.LastWriteTime)" "Cyan"

        return $true
    } else {
        Write-Status "編譯失敗 (錯誤碼: $($process.ExitCode))" "Red"
        return $false
    }
}

function Test-BuildOutput {
    Write-Status "驗證建置輸出..." "Yellow"

    if (!(Test-Path $BuildConfig.OutputFile)) {
        throw "找不到輸出檔案: $($BuildConfig.OutputFile)"
    }

    $fileInfo = Get-Item $BuildConfig.OutputFile
    if ($fileInfo.Length -eq 0) {
        throw "輸出檔案為空"
    }

    Write-Status "建置輸出驗證通過" "Green"
}

# 主建置邏輯
try {
    Write-Status "=== VirtualDrv 現代化建置腳本開始 ===" "Cyan"

    if ($Clean) {
        Invoke-Clean
        exit 0
    }

    Test-Prerequisites
    Set-BuildEnvironment

    if ($Clean) {
        Invoke-Clean
    }

    $buildSuccess = Invoke-Build

    if ($buildSuccess) {
        Test-BuildOutput
        Write-Status "=== 建置成功完成 ===" "Green"
        exit 0
    } else {
        Write-Status "=== 建置失敗 ===" "Red"
        exit 1
    }

} catch {
    Write-Status "建置過程中發生錯誤: $($_.Exception.Message)" "Red"
    Write-Status "=== 建置失敗 ===" "Red"
    exit 1
}