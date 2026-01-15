# Yori Uninstaller Script
Write-Host "  Starting Yori Uninstallation..." -ForegroundColor Cyan

# 1. REMOVE AI MODEL (Free up space)
if (Get-Command "ollama" -ErrorAction SilentlyContinue) {
    Write-Host " Removing AI Model (qwen2.5-coder) to free up space..." -ForegroundColor Yellow
    # We ignore errors in case the model is already gone
    ollama rm qwen2.5-coder:3b 2>$null 
    Write-Host "   Model removed." -ForegroundColor Green
}

# 2. REMOVE LOCAL TOOLS (MinGW)
if (Test-Path ".\tools") {
    Write-Host " Removing C++ Compiler tools..." -ForegroundColor Yellow
    Remove-Item -Path ".\tools" -Recurse -Force
    Write-Host "   Tools folder removed." -ForegroundColor Green
}

# 3. REMOVE SHORTCUT
if (Test-Path ".\Yori_Terminal.bat") {
    Write-Host "⚡ Removing Terminal Shortcut..." -ForegroundColor Yellow
    Remove-Item -Path ".\Yori_Terminal.bat" -Force
}

# 4. CLEANUP BINARIES (Optional - Asks user)
$deleteBin = Read-Host " Do you want to delete the 'bin' folder (yori.exe)? (Y/N)"
if ($deleteBin -eq 'Y' -or $deleteBin -eq 'y') {
    if (Test-Path ".\bin") {
        Write-Host "⚠️  Deleting bin folder..." -ForegroundColor Red
        Remove-Item -Path ".\bin" -Recurse -Force
    }
} else {
    Write-Host "   'bin' folder kept safe (your projects are there)." -ForegroundColor Gray
}

Write-Host "-----------------------------------------------------"
Write-Host " Uninstallation of Yori environment complete." -ForegroundColor Green
Write-Host "ℹ  NOTE: To completely remove the 'Ollama' background service,"
Write-Host "    please go to Windows Settings > Apps > Installed Apps."
Write-Host "-----------------------------------------------------"
Write-Host "Press Enter to close."
Read-Host