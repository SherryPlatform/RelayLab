#!/bin/sh
## 
## PROJECT:   RelayLab
## FILE:      build-headers.sh
## PURPOSE:   This script prepares kernel headers for building external modules.
## 
## LICENSE:   The MIT License
## 
## MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
## 

# Terminate script on error
set -e

KERNEL_SOURCE=""
KERNEL_CONFIG=""
KERNEL_ARCH="x86"
BUILD_OUTPUT=""
HEADERS_OUTPUT=""

usage() {
    echo "This script prepares kernel headers for building external modules."
    echo ""
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --source <path>         Set kernel source directory."
    echo "  --config <path>         Set kernel config file."
    echo "  --arch <arch>           Set kernel architecture (default: x86)."
    echo "  --build-output <path>   Set kernel binary build output directory."
    echo "  --output <path>         Set kernel headers output directory."
    echo "  --help                  Show this help message."
    exit 1
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --source)
            KERNEL_SOURCE="$2"
            shift 2
            ;;
        --config)
            KERNEL_CONFIG="$2"
            shift 2
            ;;
        --build-output)
            BUILD_OUTPUT="$2"
            shift 2
            ;;
        --output)
            HEADERS_OUTPUT="$2"
            shift 2
            ;;
        --arch)
            KERNEL_ARCH="$2"
            shift 2
            ;;
        --help)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Validate required options

if [ ! -d "$KERNEL_SOURCE" ]; then
    echo "Error: --source option with a valid path is required."
    echo "Current: $KERNEL_SOURCE"
    usage
fi
if [ -z "$KERNEL_CONFIG" ]; then
    echo "Error: --config option with a valid path is required."
    echo "Current: $KERNEL_CONFIG"
    usage
fi
if [ -z "$BUILD_OUTPUT" ]; then
    echo "Error: --build-output option with a valid path is required."
    echo "Current: $BUILD_OUTPUT"
    usage
fi
if [ -z "$HEADERS_OUTPUT" ]; then
    echo "Error: --output option is required."
    usage
fi

# Ensure the kernel headers output directory is clean

if [ -d "$HEADERS_OUTPUT" ]; then
    rm -rf "$HEADERS_OUTPUT"
fi
mkdir -p "$HEADERS_OUTPUT"

# Prepare the kernel source for modules

make \
    -C "$KERNEL_SOURCE" \
    KCONFIG_CONFIG="$KERNEL_CONFIG" \
    O="$HEADERS_OUTPUT" \
    HOSTLDFLAGS="-s" \
    modules_prepare

# Remove immediate files that are not needed

find "$HEADERS_OUTPUT" -type f -name ".*.cmd" -delete
find "$HEADERS_OUTPUT" -type f -name ".checked-atomic*" -delete
find "$HEADERS_OUTPUT" -type f -name "*.o" -delete
find "$HEADERS_OUTPUT" -depth -type d -empty -delete
rm "$HEADERS_OUTPUT/source"
rm "$HEADERS_OUTPUT/Makefile"
rm "$HEADERS_OUTPUT/.missing-syscalls.d"

# Copy the kernel headers to the output directory

cp -r \
    "$KERNEL_SOURCE/include" \
    "$KERNEL_SOURCE/Makefile" \
    "$HEADERS_OUTPUT"
cp -r \
    "$KERNEL_SOURCE/arch/$KERNEL_ARCH/include" \
    "$HEADERS_OUTPUT/arch/$KERNEL_ARCH"
find "$KERNEL_SOURCE/arch/$KERNEL_ARCH" \
    -maxdepth 1 \
    -type f \
    \( -name "Kbuild*" -o -name "Kconfig*" -o -name "Makefile*" \) \
    -exec cp {} "$HEADERS_OUTPUT/arch/$KERNEL_ARCH/" \;
find "$KERNEL_SOURCE/scripts" \
    -maxdepth 1 \
    -type f \
    -not \( -name "*.c" -o -name "*.h" -o -name "*.rs" \) \
    -exec cp {} "$HEADERS_OUTPUT/scripts/" \;
cp "$KERNEL_SOURCE/scripts/module-common.c" "$HEADERS_OUTPUT/scripts/"
cp -r \
    "$KERNEL_SOURCE/tools/include" \
    "$HEADERS_OUTPUT/tools"
mkdir -p "$HEADERS_OUTPUT/tools/arch/$KERNEL_ARCH"
cp -r \
    "$KERNEL_SOURCE/tools/arch/$KERNEL_ARCH/include" \
    "$HEADERS_OUTPUT/tools/arch/$KERNEL_ARCH"
cp \
    "$BUILD_OUTPUT/Module.symvers" \
    "$HEADERS_OUTPUT"

# Print success message

echo "Kernel headers for external modules have been prepared successfully."
echo "Headers are located at: $HEADERS_OUTPUT"
