#!/bin/bash
# Yori Compiler Installer for Linux
# Version 4.5

set -e  # Exit on error

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Functions
info() { echo -e "${CYAN}ℹ ${1}${NC}"; }
success() { echo -e "${GREEN}✓ ${1}${NC}"; }
warn() { echo -e "${YELLOW}⚠ ${1}${NC}"; }
error() { echo -e "${RED}✗ ${1}${NC}"; }

# Banner
echo -e "${CYAN}"
cat << "EOF"
╔══════════════════════════════════════╗
║      YORI COMPILER INSTALLER         ║
║         Linux Version 4.5            ║
╚══════════════════════════════════════╝
EOF
echo -e "${NC}"

# Detect distro
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    DISTRO="unknown"
fi

info "Detected: $DISTRO"

# Root directory
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$ROOT_DIR/bin"
TOOLS_DIR="$ROOT_DIR/tools"

# Create directories
mkdir -p "$BIN_DIR"
mkdir -p "$TOOLS_DIR"

# Check existing installations
echo ""
info "Checking system..."

HAS_GCC=$(command -v g++ >/dev/null 2>&1 && echo "yes" || echo "no")
HAS_CURL=$(command -v curl >/dev/null 2>&1 && echo "yes" || echo "no")
HAS_OLLAMA=$(command -v ollama >/dev/null 2>&1 && echo "yes" || echo "no")

if [ "$HAS_GCC" == "yes" ]; then
    success "g++ detected: $(g++ --version | head -n1)"
else
    warn "g++ not found"
fi

if [ "$HAS_CURL" == "yes" ]; then
    success "curl detected"
else
    warn "curl not found"
fi

if [ "$HAS_OLLAMA" == "yes" ]; then
    success "Ollama detected"
else
    warn "Ollama not found"
fi

# Installation mode selection
echo ""
echo -e "${YELLOW}Installation Options:${NC}"
echo "  [1] Minimal    - Yori only (cloud mode)"
echo "  [2] Local AI   - Yori + Ollama + Model"
echo "  [3] Full       - Everything (recommended)"
echo "  [4] Custom     - Choose components"
echo ""
read -p "Select option [1-4] (default=3): " CHOICE
CHOICE=${CHOICE:-3}

# Set flags based on choice
INSTALL_DEPS=false
INSTALL_OLLAMA=false
DOWNLOAD_MODEL=false

case $CHOICE in
    1)
        info "Installing: Yori only"
        ;;
    2)
        info "Installing: Yori + Local AI"
        INSTALL_DEPS=true
        INSTALL_OLLAMA=true
        DOWNLOAD_MODEL=true
        ;;
    3)
        info "Installing: Full stack"
        INSTALL_DEPS=true
        INSTALL_OLLAMA=true
        DOWNLOAD_MODEL=true
        ;;
    4)
        info "Custom installation"
        read -p "Install system dependencies (g++, curl)? [y/N]: " INST_DEP
        [ "$INST_DEP" == "y" ] && INSTALL_DEPS=true
        
        read -p "Install Ollama? [y/N]: " INST_OLL
        [ "$INST_OLL" == "y" ] && INSTALL_OLLAMA=true && DOWNLOAD_MODEL=true
        ;;
    *)
        warn "Invalid choice. Using Full installation."
        INSTALL_DEPS=true
        INSTALL_OLLAMA=true
        DOWNLOAD_MODEL=true
        ;;
esac

# ============= INSTALL DEPENDENCIES =============

if [ "$INSTALL_DEPS" == true ] && [ "$HAS_GCC" == "no" ]; then
    echo ""
    info "Installing system dependencies..."
    
    case $DISTRO in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y g++ curl build-essential
            ;;
        fedora|rhel|centos)
            sudo dnf install -y gcc-c++ curl
            ;;
        arch|manjaro)
            sudo pacman -S --noconfirm gcc curl
            ;;
        *)
            warn "Unknown distro. Please install g++ and curl manually."
            ;;
    esac
    
    if command -v g++ >/dev/null 2>&1; then
        success "g++ installed"
    else
        error "Failed to install g++. Please install manually."
        exit 1
    fi
fi

# ============= COMPILE YORI =============

echo ""
info "Compiling Yori..."

if [ ! -f "$ROOT_DIR/yoric.cpp" ]; then
    error "yoric.cpp not found in $ROOT_DIR"
    exit 1
fi

if command -v g++ >/dev/null 2>&1; then
    g++ -std=c++17 "$ROOT_DIR/yoric.cpp" -o "$BIN_DIR/yori" -lstdc++fs 2>/dev/null || \
    g++ -std=c++17 "$ROOT_DIR/yoric.cpp" -o "$BIN_DIR/yori"
    
    if [ -f "$BIN_DIR/yori" ]; then
        chmod +x "$BIN_DIR/yori"
        success "Yori compiled successfully"
    else
        error "Failed to compile Yori"
        exit 1
    fi
else
    error "g++ not available. Cannot compile Yori."
    exit 1
fi

# ============= INSTALL OLLAMA =============

if [ "$INSTALL_OLLAMA" == true ] && [ "$HAS_OLLAMA" == "no" ]; then
    echo ""
    info "Installing Ollama..."
    
    if curl -fsSL https://ollama.com/install.sh | sh; then
        success "Ollama installed"
        
        # Start Ollama service
        if systemctl is-active --quiet ollama 2>/dev/null; then
            success "Ollama service is running"
        else
            info "Starting Ollama service..."
            ollama serve &>/dev/null &
            sleep 3
        fi
    else
        error "Failed to install Ollama"
        warn "You can install manually from: https://ollama.com/download"
    fi
fi

# ============= DOWNLOAD MODEL =============

if [ "$DOWNLOAD_MODEL" == true ]; then
    echo ""
    info "Downloading AI model (~2GB, this may take a while)..."
    
    # Ensure Ollama is running
    if ! pgrep -x "ollama" > /dev/null; then
        ollama serve &>/dev/null &
        sleep 3
    fi
    
    if ollama list 2>/dev/null | grep -q "qwen2.5-coder:3b"; then
        success "Model already downloaded"
    else
        info "Pulling qwen2.5-coder:3b..."
        echo "   This download is ~2GB. Please wait..."
        
        if ollama pull qwen2.5-coder:3b; then
            success "Model downloaded successfully"
        else
            error "Failed to download model"
            warn "You can download manually with: ollama pull qwen2.5-coder:3b"
        fi
    fi
fi

# ============= CREATE CONFIG =============

echo ""
info "Creating configuration..."

CONFIG_FILE="$ROOT_DIR/config.json"

if [ ! -f "$CONFIG_FILE" ]; then
    cat > "$CONFIG_FILE" << 'EOFCONFIG'
{
    "local": {
        "model_id": "qwen2.5-coder:3b",
        "api_url": "http://localhost:11434/api/generate"
    },
    "cloud": {
        "api_key": "YOUR_API_KEY_HERE",
        "model_id": "gemini-1.5-flash"
    },
    "toolchains": {
        "cpp": {
            "build_cmd": "g++ -std=c++17"
        }
    }
}
EOFCONFIG
    success "Config created: $CONFIG_FILE"
else
    info "Config already exists (keeping existing)"
fi

# ============= ADD TO PATH =============

echo ""
read -p "Add Yori to PATH? [Y/n]: " ADD_PATH
ADD_PATH=${ADD_PATH:-y}

if [ "$ADD_PATH" == "y" ] || [ "$ADD_PATH" == "Y" ]; then
    # Determine shell config file
    if [ -n "$ZSH_VERSION" ]; then
        SHELL_RC="$HOME/.zshrc"
    elif [ -n "$BASH_VERSION" ]; then
        SHELL_RC="$HOME/.bashrc"
    else
        SHELL_RC="$HOME/.profile"
    fi
    
    # Check if already in PATH
    if grep -q "export PATH=.*$BIN_DIR" "$SHELL_RC" 2>/dev/null; then
        info "Already in PATH ($SHELL_RC)"
    else
        echo "" >> "$SHELL_RC"
        echo "# Yori Compiler" >> "$SHELL_RC"
        echo "export PATH=\"$BIN_DIR:\$PATH\"" >> "$SHELL_RC"
        success "Added to PATH ($SHELL_RC)"
        warn "Restart your terminal or run: source $SHELL_RC"
    fi
fi

# ============= CREATE EXAMPLE PROJECT =============

echo ""
read -p "Create example project? [Y/n]: " CREATE_EXAMPLE
CREATE_EXAMPLE=${CREATE_EXAMPLE:-y}

if [ "$CREATE_EXAMPLE" == "y" ] || [ "$CREATE_EXAMPLE" == "Y" ]; then
    EXAMPLE_FILE="$ROOT_DIR/hello.yori"
    
    if [ ! -f "$EXAMPLE_FILE" ]; then
        cat > "$EXAMPLE_FILE" << 'EOFEXAMPLE'
// Welcome to Yori!
// This is a simple example program.

PROGRAM: HelloWorld

// Ask the user for their name
// Print a greeting message
// Print numbers from 1 to 5
EOFEXAMPLE
        success "Created example: hello.yori"
    else
        info "Example already exists"
    fi
fi

# ============= INSTALLATION SUMMARY =============

echo ""
echo -e "${GREEN}"
cat << "EOF"
╔══════════════════════════════════════╗
║     INSTALLATION COMPLETE!           ║
╚══════════════════════════════════════╝
EOF
echo -e "${NC}"

echo -e "${YELLOW}Installed Components:${NC}"
[ -f "$BIN_DIR/yori" ] && success "Yori Compiler"
command -v g++ >/dev/null 2>&1 && success "C++ Compiler (g++)"
command -v ollama >/dev/null 2>&1 && success "Ollama (Local AI)"
[ "$DOWNLOAD_MODEL" == true ] && success "AI Model (qwen2.5-coder:3b)"

echo ""
echo -e "${YELLOW}Quick Start:${NC}"
echo "  1. Activate PATH:"
echo "     source $SHELL_RC"
echo ""
echo "  2. Try Yori:"
echo "     yori hello.yori -o hello -c        (local mode)"
echo "     yori hello.yori -o hello -c -cloud (cloud mode)"
echo ""
echo "  3. Get help:"
echo "     yori --help"

if [ "$CHOICE" == 1 ] || [ "$INSTALL_OLLAMA" == false ]; then
    echo ""
    warn "Local mode requires Ollama + Model"
    echo "   For cloud mode, edit config.json with your Gemini API key"
fi

echo ""
echo -e "${CYAN}Documentation: https://github.com/alonsovm44/yori${NC}"
echo ""