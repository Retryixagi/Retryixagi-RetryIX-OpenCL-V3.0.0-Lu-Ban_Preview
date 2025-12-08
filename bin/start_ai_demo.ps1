# RetryIX AI - Unified Demo Launcher
# Launch all AI demos and visualization framework with PowerShell
# Design Principle: One-click startup -> Real-time visualization -> Unified experience

param(
    [switch]$NoVisualizer,
    [switch]$CleanLogs
)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "      RetryIX AI - Unified Demo Launcher" -ForegroundColor Cyan
Write-Host "      Design Principle: One-click startup -> Real-time visualization -> Unified experience" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

# Check Python environment
Write-Host "Checking Python environment..." -ForegroundColor Yellow
try {
    $pythonVersion = python --version 2>&1
    Write-Host "✓ Python version: $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "✗ Python not found, please ensure Python is installed and in PATH" -ForegroundColor Red
    exit 1
}

# Check PySide6
Write-Host "Checking PySide6..." -ForegroundColor Yellow
try {
    python -c "import PySide6; print('PySide6 version:', PySide6.__version__)" 2>&1 | Out-Null
    Write-Host "✓ PySide6 installed" -ForegroundColor Green
} catch {
    Write-Host "✗ PySide6 not installed, installing..." -ForegroundColor Yellow
    pip install PySide6
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ PySide6 installation failed" -ForegroundColor Red
        exit 1
    }
}

# Check executables
$executables = @(
    "demo_ai_network.exe",
    "test_cnn.exe",
    "test_transformer.exe",
    "test_multimodal_topology.exe"
)

Write-Host "Checking AI demo executables..." -ForegroundColor Yellow
$missingExecutables = @()
foreach ($exe in $executables) {
    if (!(Test-Path $exe)) {
        $missingExecutables += $exe
    }
}

if ($missingExecutables.Count -gt 0) {
    Write-Host "✗ Missing executables:" -ForegroundColor Red
    foreach ($exe in $missingExecutables) {
        Write-Host "  - $exe" -ForegroundColor Red
    }
    Write-Host ""
    Write-Host "Please compile C programs first:" -ForegroundColor Yellow
    Write-Host "  .\build_ai.bat" -ForegroundColor Cyan
    exit 1
} else {
    Write-Host "✓ All executables exist" -ForegroundColor Green
}

# Clean old log files
if ($CleanLogs) {
    Write-Host "Cleaning old log files..." -ForegroundColor Yellow
    $logFiles = @(
        "mlp_demo_log.txt",
        "cnn_demo_log.txt",
        "transformer_demo_log.txt"
    )
    foreach ($log in $logFiles) {
        if (Test-Path $log) {
            Remove-Item $log -Force
            Write-Host "  Deleted $log" -ForegroundColor Gray
        }
    }
}

Write-Host ""
Write-Host "Starting AI demo system..." -ForegroundColor Green
Write-Host "========================" -ForegroundColor Green

# Start visualizer if not disabled
if (!$NoVisualizer) {
    Write-Host "Starting unified visualization framework..." -ForegroundColor Cyan
    try {
        $visualizerJob = Start-Job -ScriptBlock {
            param($scriptPath)
            Set-Location (Split-Path $scriptPath)
            python visualizer.py
        } -ArgumentList $MyInvocation.MyCommand.Path

        Write-Host "✓ Visualization framework started (Job ID: $($visualizerJob.Id))" -ForegroundColor Green

        # Wait for visualizer to start
        Start-Sleep -Seconds 2

    } catch {
        Write-Host "✗ Visualization framework failed to start: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "Continuing with AI demos..." -ForegroundColor Yellow
    }
}

# Start AI demo programs
$demoJobs = @()
$demos = @(
    @{Name = "MLP Demo"; Exe = "demo_ai_network.exe"},
    @{Name = "CNN Demo"; Exe = "test_cnn.exe"},
    @{Name = "Transformer Demo"; Exe = "test_transformer.exe"},
    @{Name = "Multimodal Topology Demo"; Exe = "test_multimodal_topology.exe"}
)

foreach ($demo in $demos) {
    Write-Host "Starting $($demo.Name)..." -ForegroundColor Cyan
    try {
        $job = Start-Job -ScriptBlock {
            param($exePath, $demoName)
            Set-Location (Split-Path $exePath)
            & $exePath 2>&1 | ForEach-Object {
                Write-Host "[$demoName] $_" -ForegroundColor Gray
            }
        } -ArgumentList (Join-Path $PWD $demo.Exe), $demo.Name

        $demoJobs += @{Name = $demo.Name; Job = $job}
        Write-Host "✓ $($demo.Name) started (Job ID: $($job.Id))" -ForegroundColor Green

    } catch {
        Write-Host "✗ $($demo.Name) failed to start: $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "System Status:" -ForegroundColor Cyan
Write-Host "==============" -ForegroundColor Cyan

if (!$NoVisualizer) {
    Write-Host "• Unified visualization framework: Running" -ForegroundColor Green
} else {
    Write-Host "• Unified visualization framework: Skipped" -ForegroundColor Yellow
}

foreach ($demoJob in $demoJobs) {
    $status = if ($demoJob.Job.State -eq "Running") { "Running" } else { $demoJob.Job.State }
    $color = if ($status -eq "Running") { "Green" } else { "Yellow" }
    Write-Host "• $($demoJob.Name): $status" -ForegroundColor $color
}

Write-Host ""
Write-Host "Controls:" -ForegroundColor Cyan
Write-Host "=========" -ForegroundColor Cyan
Write-Host "• Press Ctrl+C to stop all demos" -ForegroundColor White
Write-Host "• Visualization window shows real-time charts" -ForegroundColor White
Write-Host "• Console displays detailed status messages" -ForegroundColor White
Write-Host ""

# Monitor job status
Write-Host "Monitoring demo processes..." -ForegroundColor Yellow
Write-Host "(Press Ctrl+C to exit)" -ForegroundColor Gray
Write-Host ""

try {
    while ($true) {
        $runningJobs = $demoJobs | Where-Object { $_.Job.State -eq "Running" }
        $completedJobs = $demoJobs | Where-Object { $_.Job.State -ne "Running" }

        if ($runningJobs.Count -eq 0) {
            Write-Host "All AI demos completed!" -ForegroundColor Green
            break
        }

        # Display running jobs
        foreach ($job in $runningJobs) {
            Write-Host "▶ $($job.Name) running..." -ForegroundColor Cyan
        }

        # Check completed jobs
        foreach ($job in $completedJobs) {
            if ($job.Job.State -eq "Completed") {
                Write-Host "✓ $($job.Name) completed" -ForegroundColor Green
            } elseif ($job.Job.State -eq "Failed") {
                Write-Host "✗ $($job.Name) failed" -ForegroundColor Red
            }
        }

        Start-Sleep -Seconds 5
        Write-Host "" # Empty line
    }

} catch {
    Write-Host ""
    Write-Host "Stop signal received, cleaning up..." -ForegroundColor Yellow
} finally {
    # Clean up jobs
    Write-Host "Stopping all jobs..." -ForegroundColor Yellow

    if (!$NoVisualizer) {
        Write-Host "Stopping visualization framework..." -ForegroundColor Gray
        Stop-Job $visualizerJob -ErrorAction SilentlyContinue
        Remove-Job $visualizerJob -ErrorAction SilentlyContinue
    }

    foreach ($demoJob in $demoJobs) {
        Write-Host "Stopping $($demoJob.Name)..." -ForegroundColor Gray
        Stop-Job $demoJob.Job -ErrorAction SilentlyContinue
        Remove-Job $demoJob.Job -ErrorAction SilentlyContinue
    }

    Write-Host "Cleanup completed." -ForegroundColor Green
    Write-Host ""
    Write-Host "Thank you for using RetryIX AI unified demo system!" -ForegroundColor Cyan
}