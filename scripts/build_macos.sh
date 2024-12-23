#!/bin/bash

# Initialize variables
BUILD_TYPE="Release"
BUILD_DIR="build"
NUM_CORES=$(sysctl -n hw.ncpu)

# Function to show help message
show_help() {
    echo "Usage: build_macos.sh [options]"
    echo "Options:"
    echo "  --debug     Build in debug mode"
    echo "  --release   Build in release mode (default)"
    echo "  --clean     Clean build directory before building"
    echo "  --help      Show this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --clean)
            rm -rf "${BUILD_DIR}"
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Check for required tools
command -v cmake >/dev/null 2>&1 || {
    echo "Error: CMake not found. Please install CMake."
    echo "Run: brew install cmake"
    exit 1
}

command -v git >/dev/null 2>&1 || {
    echo "Error: Git not found. Please install Git."
    echo "Run: brew install git"
    exit 1
}

# Check for Xcode Command Line Tools
if ! xcode-select -p &>/dev/null; then
    echo "Error: Xcode Command Line Tools not found."
    echo "Run: xcode-select --install"
    exit 1
fi

# Check for Homebrew
if ! command -v brew >/dev/null 2>&1; then
    echo "Warning: Homebrew not found. It's recommended for managing dependencies."
    echo "Install from: https://brew.sh"
fi

# Setup vcpkg if not already present
if [ ! -d "vcpkg" ]; then
    echo "Setting up vcpkg..."
    git clone https://github.com/Microsoft/vcpkg.git
    if [ $? -ne 0 ]; then
        echo "Error: Failed to clone vcpkg repository."
        exit 1
    fi
    ./vcpkg/bootstrap-vcpkg.sh
    if [ $? -ne 0 ]; then
        echo "Error: Failed to bootstrap vcpkg."
        exit 1
    fi
fi

# Set VCPKG_ROOT environment variable
export VCPKG_ROOT="$(pwd)/vcpkg"

# Create build directory
mkdir -p "${BUILD_DIR}"

# Configure CMake
echo "Configuring CMake for ${BUILD_TYPE} build..."
cmake -B "${BUILD_DIR}" -S . \
      -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
      -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"  # Universal binary support

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed."
    exit 1
fi

# Build the project
echo "Building project with ${NUM_CORES} cores..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j "${NUM_CORES}"

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo
echo "Build completed successfully!"
echo "Executable location: ${BUILD_DIR}/GameEnginePhysx"

# Make the executable runnable
chmod +x "${BUILD_DIR}/GameEnginePhysx"

# Check if this is an Apple Silicon Mac
if [[ $(uname -m) == 'arm64' ]]; then
    echo "Note: Running on Apple Silicon Mac. The application has been built as a universal binary."
fi