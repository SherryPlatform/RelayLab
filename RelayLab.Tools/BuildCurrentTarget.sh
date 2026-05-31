#!/bin/sh

set -e

ThisFileDirectory="$(dirname "$(realpath "$0")")"

SourceDirectory="$ThisFileDirectory/RelayLab.Tools"

OutputDirectory="$ThisFileDirectory/Output"

BinariesDirectory="$OutputDirectory/Binaries"
ObjectsDirectory="$OutputDirectory/Objects"

TargetName="RelayLab.Tools"

TargetPath="$BinariesDirectory/$TargetName"

TargetFlags="-O2 -Wno-multichar"

IncludePath="
    -I${SourceDirectory}/Mile.Mobility
    -I${SourceDirectory}/Mile.HyperV
    -I${SourceDirectory}/Mile.Posix.Helpers
"

CC="${CC:-cc}"
CXX="${CXX:-c++}"

ARCH="$(uname -m)"
ARCH_DEFINES=""

case "$ARCH" in
    x86_64|amd64)
        ARCH_DEFINES="-D_M_AMD64"
        ;;
    aarch64|arm64)
        ARCH_DEFINES="-D_M_ARM64"
        ;;
esac

CFLAGS="$TargetFlags $IncludePath $ARCH_DEFINES $CFLAGS"
CXXFLAGS="-std=c++20 $TargetFlags $IncludePath $ARCH_DEFINES $CXXFLAGS"

mkdir -p "$BinariesDirectory"
mkdir -p "$ObjectsDirectory"

echo "CC $SourceDirectory/Mile.Mobility/Mile.Mobility.Utilities.FixedInteger.c"
"$CC" $CFLAGS -c \
    "$SourceDirectory/Mile.Mobility/Mile.Mobility.Utilities.FixedInteger.c" \
    -o "$ObjectsDirectory/Mile.Mobility.Utilities.FixedInteger.o"

echo "CXX $SourceDirectory/Mile.Posix.Helpers/Mile.Helpers.CppBase.cpp"
"$CXX" $CXXFLAGS -c \
    "$SourceDirectory/Mile.Posix.Helpers/Mile.Helpers.CppBase.cpp" \
    -o "$ObjectsDirectory/Mile.Helpers.CppBase.o"

echo "CXX $SourceDirectory/main.cpp"
"$CXX" $CXXFLAGS -c \
    "$SourceDirectory/main.cpp" \
    -o "$ObjectsDirectory/main.o"

echo "LINK $TargetPath"
"$CXX" \
    "$ObjectsDirectory/main.o" \
    "$ObjectsDirectory/Mile.Mobility.Utilities.FixedInteger.o" \
    "$ObjectsDirectory/Mile.Helpers.CppBase.o" \
    -Wl,--gc-sections \
    -o "$TargetPath"

echo "TARGET $TargetPath"
