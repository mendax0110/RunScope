#!/bin/bash

set -e

echo "RunScope Dependency Installer"
echo "=============================="
echo ""

detect_os()
{
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            OS=$ID
        else
            OS="unknown"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    else
        OS="unknown"
    fi
    echo "$OS"
}

install_ubuntu()
{
    echo "Installing dependencies for Ubuntu/Debian..."
    
    sudo apt-get update
    
    echo "Installing build essentials..."
    sudo apt-get install -y build-essential cmake git
    
    echo "Installing OpenGL and X11 libraries..."
    sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
    sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
    
    echo "Installing additional development tools..."
    sudo apt-get install -y pkg-config
    
    echo "Ubuntu dependencies installed successfully!"
}

install_macos()
{
    echo "Installing dependencies for macOS..."
    
    echo "Installing GLFW dependencies..."
    brew install glfw
    
    echo "macOS dependencies installed successfully!"
}

OS=$(detect_os)

case "$OS" in
    ubuntu|debian)
        install_ubuntu
        ;;
    macos)
        install_macos
        ;;
    *)
        echo "Unsupported operating system: $OS"
        echo "Please install the following manually:"
        echo "  - CMake 3.20+"
        echo "  - C++20 compatible compiler"
        echo "  - OpenGL development libraries"
        echo "  - X11 development libraries (Linux only)"
        exit 1
        ;;
esac

echo ""
echo "All dependencies installed!"
echo ""
echo "You can now build RunScope:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  cmake --build ."
echo ""
