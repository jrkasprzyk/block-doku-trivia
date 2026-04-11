#!/bin/bash
# Build script for Git Bash / MSYS2 where vcvars64.bat can't run directly.
# Sets MSVC environment variables manually, then builds.

MSVC_ROOT="C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.50.35717"
SDK_VER="10.0.26100.0"
SDK_INC="C:/Program Files (x86)/Windows Kits/10/Include/$SDK_VER"
SDK_LIB="C:/Program Files (x86)/Windows Kits/10/Lib/$SDK_VER"
SDK_BIN="C:/Program Files (x86)/Windows Kits/10/bin/$SDK_VER/x64"

export INCLUDE="$MSVC_ROOT/include;$SDK_INC/ucrt;$SDK_INC/shared;$SDK_INC/um;$SDK_INC/winrt"
export LIB="$MSVC_ROOT/lib/x64;$SDK_LIB/ucrt/x64;$SDK_LIB/um/x64"
export PATH="$MSVC_ROOT/bin/Hostx64/x64:$SDK_BIN:$PATH"

cmake --build build "$@"
