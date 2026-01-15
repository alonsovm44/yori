# Yori Installer Script
Write-Host " Starting Yori installation..." -ForegroundColor Cyan

# 1. CHECK OLLAMA
if (Get-Command "ollama" -ErrorAction SilentlyContinue) {
    Write-Host " Ollama is already installed." -ForegroundColor Green
} else {
    Write-Host "⬇ Downloading Ollama..." -ForegroundColor Yellow
    Invoke-WebRequest -Uri "https://ollama.com/download/OllamaSetup.exe" -OutFile "OllamaSetup.exe"
    Write-Host " Installing Ollama (Please accept the installation prompt)..." -ForegroundColor Yellow
    Start-Process -FilePath ".\OllamaSetup.exe" -Wait
    Remove-Item ".\OllamaSetup.exe"
}

# 2. DOWNLOAD QWEN MODEL (The Brain)
Write-Host " Verifying AI model (Qwen 2.5)..." -ForegroundColor Cyan
# Start the service just in case
Start-Process "ollama" "serve" -WindowStyle Hidden -ErrorAction SilentlyContinue
Start-Sleep -Seconds 5

# Pull the model
Write-Host "   This might take a few minutes depending on your internet connection..." -ForegroundColor Yellow
ollama pull qwen2.5-coder:3b

# 3. SETUP MINGW (C++ Compiler)
$gccPath = ".\tools\w64devkit"
if (-Not (Test-Path "$gccPath\bin\g++.exe")) {
    Write-Host " Downloading Portable C++ Compiler..." -ForegroundColor Yellow
    # Using w64devkit (Portable MinGW ~80MB)
    $url = "https://github.com/skeeto/w64devkit/releases/download/v1.20.0/w64devkit-1.20.0.zip"
    Invoke-WebRequest -Uri $url -OutFile "mingw.zip"
    
    Write-Host " Unzipping compiler..."
    Expand-Archive -Path "mingw.zip" -DestinationPath ".\tools" -Force
    Remove-Item "mingw.zip"
} else {
    Write-Host " C++ Compiler detected." -ForegroundColor Green
}

# 4. CREATE ENVIRONMENT (Yori Terminal)
Write-Host " Creating shortcut..." -ForegroundColor Cyan

# Get absolute paths
$root = Get-Location
$yoriBin = "$root\bin"
$mingwBin = "$root\tools\w64devkit\bin"

# Create the .bat file that configures the PATH only for this session
$batContent = "@echo off
title Yori Terminal
set PATH=$mingwBin;$yoriBin;%PATH%
echo ==========================================
echo      WELCOME TO YORI COMPILER
echo ==========================================
echo.
echo Available Commands:
echo   yori hello_world.yori      (Compile locally)
echo   yori hello_world.yori -u   (Update/Edit mode)
echo.
cmd /k"

Set-Content -Path "Yori_Terminal.bat" -Value $batContent

Write-Host "Installation Complete! Double-click 'Yori_Terminal.bat' to start." -ForegroundColor Green
Write-Host "Press Enter to exit."
Read-Host