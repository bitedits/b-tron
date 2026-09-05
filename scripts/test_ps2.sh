#!/usr/bin/env bash
# scripts/test_ps2.sh — Cleanroom PlayStation 2 ELF & Boot Verification Suite
#
# Copyright 2026 Synrc Research Center. MIT License.

set -euo pipefail

ELF="btron-ps2.elf"
ISO="btron-ps2.iso"
READELF="mipsel-linux-gnu-readelf -W"
OBJDUMP="mipsel-linux-gnu-objdump"

echo "=========================================================="
echo " Testing B-System PS2 Kernel (ELF & Toolchain Validation) "
echo "=========================================================="

if [ ! -f "$ELF" ]; then
    echo "[ERROR] $ELF not found. Build with 'make ps2' first."
    exit 1
fi

TOTAL=0
PASSED=0

assert_check() {
    local desc="$1"
    local cmd="$2"
    TOTAL=$((TOTAL + 1))
    echo -n "  [TEST $TOTAL] $desc ... "
    if eval "$cmd" >/dev/null 2>&1; then
        echo "[PASS]"
        PASSED=$((PASSED + 1))
    else
        echo "[FAIL]"
        echo "        Failed command: $cmd"
        exit 1
    fi
}

# 1. Check ELF Class
assert_check "ELF is 32-bit Little-Endian (ELF32 LSB)" \
    "$READELF -h $ELF | grep -E 'Class:[[:space:]]+ELF32' && $READELF -h $ELF | grep -E 'Data:[[:space:]]+2.*little endian'"

# 2. Check Machine Architecture
assert_check "ELF Machine architecture is MIPS" \
    "$READELF -h $ELF | grep -E 'Machine:[[:space:]]+MIPS'"

# 3. Check Entry Point
assert_check "ELF Entry Point is 0x00100000" \
    "$READELF -h $ELF | grep -E 'Entry point address:[[:space:]]+0x100000'"

# 4. Check Mandatory Symbols
assert_check "Symbol '_start' exists in symbol table" \
    "$READELF -s $ELF | grep -E '[[:space:]]_start$'"

assert_check "Symbol 'ps2_kernel_main' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_kernel_main$'"

assert_check "Symbol 'ps2_gs_init' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_gs_init$'"

assert_check "Symbol 'ps2_gs_draw_rect' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_gs_draw_rect$'"

assert_check "Symbol 'ps2_sio_putc' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_sio_putc$'"

assert_check "Symbol 'ps2_sio_getc' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_sio_getc$'"

assert_check "Symbol 'ps2_set_gs_crt' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_set_gs_crt$'"

assert_check "Symbol 'ps2_gs_put_imr' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_gs_put_imr$'"

assert_check "Symbol 'ps2_gs_vsync' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_gs_vsync$'"

assert_check "Symbol 'ps2_gs_swap_buffers' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_gs_swap_buffers$'"

assert_check "Symbol 'ps2_pad_init' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_pad_init$'"

assert_check "Symbol 'ps2_pad_poll' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_pad_poll$'"

assert_check "Symbol 'ps2_usb_init' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_usb_init$'"

assert_check "Symbol 'ps2_usb_poll' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_usb_poll$'"

assert_check "Symbol 'ps2_usb_hid_to_btron_key' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_usb_hid_to_btron_key$'"

assert_check "Symbol 'ps2_usb_process_keyboard_report' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_usb_process_keyboard_report$'"

assert_check "Symbol 'ps2_usb_inject_keyboard' exists" \
    "$READELF -s $ELF | grep -E '[[:space:]]ps2_usb_inject_keyboard$'"

# 5. Check Instruction Encoding for Emotion Engine MIPS-III Compatibility
# Emotion Engine R5900 does not support MIPS32r2 opcodes like seb, seh, ins, ext
assert_check "Zero invalid MIPS32r2 opcodes (seb, seh, ins, ext)" \
    "! ($OBJDUMP -d $ELF | grep -E '\<(seb|seh|ins|ext)\>')"

# 6. Check Bootable ISO Disc Packaging
if [ -f "$ISO" ]; then
    assert_check "ISO disc image exists and is non-empty" \
        "[ -s $ISO ]"
fi

echo "----------------------------------------------------------"
echo " PS2 Test Results: $PASSED / $TOTAL tests passed (100%)"
echo "----------------------------------------------------------"
echo "[CI-TEST] SUCCESS — PlayStation 2 ELF & drivers validated!"
