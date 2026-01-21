# Yori Compiler Installer v4.5
# Interactive installation with user choice

param(
    [string]$Mode = "interactive"
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
if (-not $root) { $root = Get-Location }

# Colors
function Write-Success { param($msg) Write-Host "✓ $msg" -ForegroundColor Green }
function Write-Info { param($msg) Write-Host "ℹ $msg" -ForegroundColor Cyan }
function Write-Warn { param($msg) Write-Host "⚠ $msg" -ForegroundColor Yellow }
function Write-Fail { param($msg) Write-Host "✗ $msg" -ForegroundColor Red }

# Banner
Write-Host @"
╔══════════════════════════════════════╗
║      YORI COMPILER INSTALLER         ║
║           Version 4.5                ║
╚══════════════════════════════════════╝
"@ -ForegroundColor Cyan

# Check existing installations
$hasOllama = Get-Command "ollama" -ErrorAction SilentlyContinue
$hasGCC = Get-Command "g++" -ErrorAction SilentlyContinue

Write-Host ""
Write-Info "Checking system..."
if ($hasOllama) { Write-Success "Ollama detected" } else { Write-Warn "Ollama not found" }
if ($hasGCC) { Write-Success "g++ detected" } else { Write-Warn "g++ not found" }

# Installation mode selection
if ($Mode -eq "interactive") {
    Write-Host ""
    Write-Host "Installation Options:" -ForegroundColor Yellow
    Write-Host "  [1] Minimal    - Yori only (~5MB, cloud mode only)"
    Write-Host "  [2] Local AI   - Yori + Ollama + Model (~2.5GB)"
    Write-Host "  [3] Full       - Everything (~2.6GB, recommended)"
    Write-Host "  [4] Custom     - Choose components"
    Write-Host ""
    
    $choice = Read-Host "Select option [1-4]"
    
    switch ($choice) {
        1 { $Mode = "minimal" }
        2 { $Mode = "local" }
        3 { $Mode = "full" }
        4 { $Mode = "custom" }
        default { 
            Write-Fail "Invalid choice. Defaulting to Full."
            $Mode = "full"
        }
    }
}

# Component flags
$installYori = $true
$installOllama = $false
$installGCC = $false
$downloadModel = $false

# Set components based on mode
switch ($Mode) {
    "minimal" {
        Write-Info "Installing: Yori only"
    }
    "local" {
        Write-Info "Installing: Yori + Local AI stack"
        $installOllama = -not $hasOllama
        $downloadModel = $true
        if (-not $hasGCC) {
            $askGCC = Read-Host "g++ not found. Install portable compiler? (Y/n)"
            $installGCC = ($askGCC -ne 'n' -and $askGCC -ne 'N')
        }
    }
    "full" {
        Write-Info "Installing: Full stack"
        $installOllama = -not $hasOllama
        $installGCC = -not $hasGCC
        $downloadModel = $true
    }
    "custom" {
        Write-Info "Custom installation"
        if (-not $hasOllama) {
            $installOllama = (Read-Host "Install Ollama? (Y/n)") -ne 'n'
            $downloadModel = $installOllama
        }
        if (-not $hasGCC) {
            $installGCC = (Read-Host "Install portable g++? (Y/n)") -ne 'n'
        }
    }
}

# Create directories
New-Item -ItemType Directory -Force -Path "$root\bin" | Out-Null
New-Item -ItemType Directory -Force -Path "$root\tools" | Out-Null

# ============= INSTALL COMPONENTS =============

# 1. YORI EXECUTABLE
Write-Host ""
Write-Info "Installing Yori compiler..."

if (Test-Path "$root\yoric.cpp") {
    Write-Info "Building from source..."
    try {
        $compiler = if ($hasGCC) { "g++" } else { "$root\tools\w64devkit\bin\g++.exe" }
        
        if (Test-Path $compiler) {
            & $compiler -std=c++17 "$root\yoric.cpp" -o "$root\bin\yori.exe"
            if ($LASTEXITCODE -eq 0) {
                Write-Success "Yori compiled successfully"
            } else {
                throw "Compilation failed"
            }
        } else {
            Write-Warn "Compiler not available yet. Will compile after g++ installation."
        }
    } catch {
        Write-Fail "Failed to compile Yori: $_"
        Write-Info "You can compile manually later with: g++ -std=c++17 yoric.cpp -o bin\yori.exe"
    }
} else {
    Write-Warn "yoric.cpp not found. Skipping compilation."
}

# 2. OLLAMA (if needed)
if ($installOllama) {
    Write-Host ""
    Write-Info "Installing Ollama (~500MB)..."
    
    try {
        $ollamaUrl = "https://ollama.com/download/OllamaSetup.exe"
        $ollamaInstaller = "$root\OllamaSetup.exe"
        
        Write-Info "Downloading..."
        Invoke-WebRequest -Uri $ollamaUrl -OutFile $ollamaInstaller -UseBasicParsing
        
        Write-Info "Running installer (accept prompts)..."
        Start-Process -FilePath $ollamaInstaller -Wait
        
        Remove-Item $ollamaInstaller -ErrorAction SilentlyContinue
        Write-Success "Ollama installed"
        
        # Verify
        Start-Sleep -Seconds 3
        if (Get-Command "ollama" -ErrorAction SilentlyContinue) {
            Write-Success "Ollama verified"
        } else {
            Write-Warn "Ollama installed but not in PATH. Restart terminal."
        }
    } catch {
        Write-Fail "Failed to install Ollama: $_"
        Write-Info "You can install manually from: https://ollama.com/download"
    }
}

# 3. AI MODEL (if needed)
if ($downloadModel) {
    Write-Host ""
    Write-Info "Downloading AI model (~2GB, this may take a while)..."
    
    try {
        # Start Ollama service
        Start-Process "ollama" "serve" -WindowStyle Hidden -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 5
        
        # Check if model exists
        $models = ollama list 2>$null | Out-String
        
        if ($models -match "qwen2.5-coder:3b") {
            Write-Success "Model already downloaded"
        } else {
            Write-Info "Pulling qwen2.5-coder:3b..."
            Write-Host "   This download is ~2GB. Progress will show in Ollama."
            
            ollama pull qwen2.5-coder:3b
            
            if ($LASTEXITCODE -eq 0) {
                Write-Success "Model downloaded successfully"
            } else {
                throw "Model pull failed"
            }
        }
    } catch {
        Write-Fail "Failed to download model: $_"
        Write-Info "You can download manually with: ollama pull qwen2.5-coder:3b"
    }
}

# 4. PORTABLE GCC (if needed)
if ($installGCC) {
    Write-Host ""
    Write-Info "Installing portable C++ compiler (~80MB)..."
    
    $gccPath = "$root\tools\w64devkit"
    
    if (Test-Path "$gccPath\bin\g++.exe") {
        Write-Success "Compiler already installed"
    } else {
        try {
            $gccUrl = "https://github.com/skeeto/w64devkit/releases/download/v1.20.0/w64devkit-1.20.0.zip"
            $gccZip = "$root\mingw.zip"
            
            Write-Info "Downloading..."
            Invoke-WebRequest -Uri $gccUrl -OutFile $gccZip -UseBasicParsing
            
            Write-Info "Extracting..."
            Expand-Archive -Path $gccZip -DestinationPath "$root\tools" -Force
            
            Remove-Item $gccZip -ErrorAction SilentlyContinue
            
            # Verify
            if (Test-Path "$gccPath\bin\g++.exe") {
                Write-Success "Compiler installed"
                
                # Try to compile Yori if we skipped it earlier
                if (Test-Path "$root\yoric.cpp" -and -not (Test-Path "$root\bin\yori.exe")) {
                    Write-Info "Compiling Yori..."
                    & "$gccPath\bin\g++.exe" -std=c++17 "$root\yoric.cpp" -o "$root\bin\yori.exe"
                    if ($LASTEXITCODE -eq 0) {
                        Write-Success "Yori compiled"
                    }
                }
            } else {
                throw "Compiler files not found after extraction"
            }
        } catch {
            Write-Fail "Failed to install compiler: $_"
            Write-Info "You can install MinGW manually from: https://www.mingw-w64.org/"
        }
    }
}

# 5. CONFIGURATION FILE
Write-Host ""
Write-Info "Creating configuration..."

$configPath = "$root\config.json"

if (-not (Test-Path $configPath)) {
    $config = @{
        local = @{
            model_id = "qwen2.5-coder:3b"
            api_url = "http://localhost:11434/api/generate"
        }
        cloud = @{
            api_key = "YOUR_API_KEY_HERE"
            model_id = "gemini-1.5-flash"
        }
        toolchains = @{
            cpp = @{
                build_cmd = "g++ -std=c++17"
            }
        }
    }
    
    $config | ConvertTo-Json -Depth 5 | Set-Content $configPath
    Write-Success "Config created at: $configPath"
} else {
    Write-Info "Config already exists (keeping existing)"
}

# 6. TERMINAL SHORTCUT
Write-Host ""
Write-Info "Creating terminal shortcut..."

$yoriBin = "$root\bin"
$mingwBin = "$root\tools\w64devkit\bin"

$batContent = @"
@echo off
title Yori Compiler Terminal
set PATH=$mingwBin;$yoriBin;%PATH%

echo ╔══════════════════════════════════════╗
echo ║      YORI COMPILER TERMINAL          ║
echo ║           Version 4.5                ║
echo ╚══════════════════════════════════════╝
echo.
echo Quick Start:
echo   yori script.py -o app.exe -c       (Local compilation)
echo   yori script.py -o app.exe -cloud   (Cloud compilation)
echo   yori --help                        (Show all options)
echo.
echo For cloud mode, edit config.json with your API key.
echo.

cmd /k
"@

Set-Content -Path "$root\Yori_Terminal.bat" -Value $batContent
Write-Success "Terminal shortcut created: Yori_Terminal.bat"

# 7. ADD TO PATH (optional)
Write-Host ""
$addPath = Read-Host "Add Yori to system PATH? (Y/n)"

if ($addPath -ne 'n' -and $addPath -ne 'N') {
    try {
        $scope = "User"
        $currentPath = [Environment]::GetEnvironmentVariable("Path", $scope)
        $newPath = $currentPath
        
        if ($newPath -notlike "*$yoriBin*") { $newPath += ";$yoriBin" }
        if ($installGCC -and $newPath -notlike "*$mingwBin*") { $newPath += ";$mingwBin" }
        
        if ($newPath -ne $currentPath) {
            [Environment]::SetEnvironmentVariable("Path", $newPath, $scope)
            Write-Success "Added to PATH (restart terminal to use)"
        } else {
            Write-Info "Already in PATH"
        }
    } catch {
        Write-Warn "Failed to modify PATH: $_"
        Write-Info "You can add manually or use Yori_Terminal.bat"
    }
}

# 8. INSTALLATION SUMMARY
Write-Host ""
Write-Host "╔══════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║     INSTALLATION COMPLETE!           ║" -ForegroundColor Green
Write-Host "╚══════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""

Write-Host "Installed Components:" -ForegroundColor Yellow
if (Test-Path "$root\bin\yori.exe") { Write-Success "Yori Compiler" } else { Write-Warn "Yori (compile manually)" }
if ($installOllama -or $hasOllama) { Write-Success "Ollama (Local AI)" }
if ($downloadModel) { Write-Success "AI Model" }
if ($installGCC -or $hasGCC) { Write-Success "C++ Compiler" }

Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host "  1. Double-click 'Yori_Terminal.bat' to start"
Write-Host "  2. Try: yori --init    (create example project)"
Write-Host "  3. Try: yori hello.yori -o hello.exe -c"

if ($Mode -eq "minimal" -or (-not $installOllama -and -not $hasOllama)) {
    Write-Host ""
    Write-Warn "Local mode requires Ollama + Model"
    Write-Host "   For cloud mode, edit config.json with your Gemini API key"
}

Write-Host ""
Write-Host "Documentation: https://github.com/alonsovm44/yori" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press Enter to exit..."
Read-Host