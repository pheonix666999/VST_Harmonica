#!/bin/bash
# ── Turbo Tuning — macOS Build Script ──
# Builds AU, VST3, and Standalone using CMake + Xcode

echo ""
echo "============================================"
echo "  TURBO TUNING — macOS Build"
echo "============================================"
echo ""

BUILD_DIR="build-xcode"

# Configure
echo "[1/3] Configuring CMake..."
cmake -B "$BUILD_DIR" -G Xcode
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed!"
    exit 1
fi

# Build (Release)
echo ""
echo "[2/3] Building Release..."
cmake --build "$BUILD_DIR" --config Release -- -j$(sysctl -n hw.ncpu)
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed!"
    exit 1
fi

echo ""
echo "[3/3] Build complete!"
echo ""
echo "Output locations:"
echo "  AU:         $BUILD_DIR/TurboTuning_artefacts/Release/AU/"
echo "  VST3:       $BUILD_DIR/TurboTuning_artefacts/Release/VST3/"
echo "  Standalone: $BUILD_DIR/TurboTuning_artefacts/Release/Standalone/"
echo ""
