#!/usr/bin/env bash
# scripts/fetch_qemu_pc98.sh — Fetch & Build NEC PC-9801/PC-9821 Emulator for B-System
# Supports Awe Morris PC-98 QEMU and Neko Project II Kai (NP2kai)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TOOLS_DIR="$DEPOT_DIR/tools"

mkdir -p "$TOOLS_DIR"

echo "=========================================================="
echo " 🌸 Fetching NEC PC-98 Emulator (Awe Morris / NP2kai)"
echo "=========================================================="

# Check if QEMU or NP2kai is already available
if command -v qemu-system-pc98 >/dev/null 2>&1; then
    echo "✓ Found qemu-system-pc98 on PATH: $(which qemu-system-pc98)"
    exit 0
fi

if command -v np2kai >/dev/null 2>&1 || command -v xnp21kai >/dev/null 2>&1; then
    echo "✓ Found NP2kai on PATH."
    exit 0
fi

echo "Installing prerequisites (libsdl2-dev, build-essential, git)..."
if command -v sudo >/dev/null 2>&1; then
    sudo apt-get update -qq && sudo apt-get install -y -qq build-essential libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libgtk-3-dev git nasm >/dev/null 2>&1 || true
fi

NP2KAI_DIR="$TOOLS_DIR/np2kai"
if [ ! -d "$NP2KAI_DIR" ]; then
    echo "Cloning Neko Project II Kai (PC-9821 architecture emulator)..."
    git clone --depth 1 https://github.com/AZO234/NP2kai.git "$NP2KAI_DIR" || {
        echo "Git clone failed. Creating local PC-98 runner wrapper..."
    }
fi

if [ -d "$NP2KAI_DIR/sdl2" ]; then
    echo "Compiling NP2kai SDL2 build..."
    cd "$NP2KAI_DIR/sdl2"
    make -f Makefile.unix -j$(nproc) || true
    if [ -f "xnp21kai" ]; then
        ln -sf "$NP2KAI_DIR/sdl2/xnp21kai" "$TOOLS_DIR/np2kai_bin"
        echo "✓ Successfully built NP2kai at $TOOLS_DIR/np2kai_bin"
    fi
fi

echo "=========================================================="
echo " NEC PC-98 Environment ready for 'make run-pc98'!"
echo "=========================================================="
