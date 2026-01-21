# Yori Installer Script
Write-Host " Starting Yori installation..." -ForegroundColor Cyan

# Set root to script directory for portability
$root = $PSScriptRoot
if (-not $root) { $root = Get-Location }

# 1. CHECK OLLAMA
if (Get-Command "ollama" -ErrorAction SilentlyContinue) {
    Write-Host " Ollama is already installed." -ForegroundColor Green
} else {
    Write-Host "⬇ Downloading Ollama..." -ForegroundColor Yellow
    try {
        Invoke-WebRequest -Uri "https://ollama.com/download/OllamaSetup.exe" -OutFile "$root\OllamaSetup.exe" -UseBasicParsing
        Write-Host " Installing Ollama (Please accept the installation prompt)..." -ForegroundColor Yellow
        Start-Process -FilePath "$root\OllamaSetup.exe" -Wait
        Remove-Item "$root\OllamaSetup.exe" -ErrorAction SilentlyContinue
    } catch {
        Write-Host " Failed to download or install Ollama. Check internet connection." -ForegroundColor Red
        exit 1
    }
}

# 2. DOWNLOAD QWEN MODEL (The Brain)
Write-Host " Verifying AI model (Qwen 2.5)..." -ForegroundColor Cyan
# Start the service just in case
Start-Process "ollama" "serve" -WindowStyle Hidden -ErrorAction SilentlyContinue
Start-Sleep -Seconds 5

$models = ollama list 2>$null
if ($models -match "qwen2.5-coder:3b") {
    Write-Host "   Model already pulled " -ForegroundColor Green
} else {
    Write-Host "   This might take a few minutes depending on your internet connection..." -ForegroundColor Yellow
    ollama pull qwen2.5-coder:3b
    
    $models = ollama list 2>$null
    if ($models -notmatch "qwen2.5-coder:3b") {
        Write-Host " Warning: Model pull might have failed. Run 'ollama pull qwen2.5-coder:3b' manually." -ForegroundColor Red
    }
}

# 3. SETUP MINGW (C++ Compiler)
$gccPath = "$root\tools\w64devkit"
if (-Not (Test-Path "$gccPath\bin\g++.exe")) {
    Write-Host " Downloading Portable C++ Compiler..." -ForegroundColor Yellow
    # Using w64devkit (Portable MinGW ~80MB)
    $url = "https://github.com/skeeto/w64devkit/releases/download/v1.20.0/w64devkit-1.20.0.zip"
    
    try {
        Invoke-WebRequest -Uri $url -OutFile "$root\mingw.zip" -UseBasicParsing
        Write-Host " Unzipping compiler..."
        Expand-Archive -Path "$root\mingw.zip" -DestinationPath "$root\tools" -Force
        Remove-Item "$root\mingw.zip" -ErrorAction SilentlyContinue
    } catch {
        Write-Host " Failed to download compiler. Check internet connection." -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host " C++ Compiler detected." -ForegroundColor Green
}

# Verify Compiler
if (Test-Path "$gccPath\bin\g++.exe") {
    try {
        $gccVersion = & "$gccPath\bin\g++.exe" --version 2>&1
        if ($gccVersion -match "g\+\+") {
            Write-Host "   Compiler verified." -ForegroundColor Green
        } else { throw "Compiler check failed" }
    } catch {
        Write-Host " Compiler setup failed. g++.exe is not executable." -ForegroundColor Red
    }
}

# 4. CREATE ENVIRONMENT (Yori Terminal)
Write-Host " Creating shortcut..." -ForegroundColor Cyan

# Get absolute paths
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
echo   yori file.yori -cloud      (Use Gemini Flash - Faster)
echo.
cmd /k"

Set-Content -Path "$root\Yori_Terminal.bat" -Value $batContent

# 5. ADD TO PATH PERMANENTLY
Write-Host " Optional: Add Yori to PATH to run it from any terminal." -ForegroundColor Cyan
$addToPath = Read-Host " Add to PATH? (Y/N)"
if ($addToPath -eq 'Y' -or $addToPath -eq 'y') {
    $scope = "User"
    $currentPath = [Environment]::GetEnvironmentVariable("Path", $scope)
    $newPath = $currentPath
    
    if ($newPath -notlike "*$yoriBin*") { $newPath += ";$yoriBin" }
    if ($newPath -notlike "*$mingwBin*") { $newPath += ";$mingwBin" }
    
    if ($newPath -ne $currentPath) {
        [Environment]::SetEnvironmentVariable("Path", $newPath, $scope)
        Write-Host "   Success! Added to User PATH. Please restart your terminals." -ForegroundColor Green
    } else {
        Write-Host "   Already in PATH." -ForegroundColor Yellow
    }
}

Write-Host "Installation Complete! Double-click 'Yori_Terminal.bat' to start." -ForegroundColor Green
Write-Host "Press Enter to exit."
Read-Host