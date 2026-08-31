#!/usr/bin/env bash
# fetch_dwc2_refs.sh
#
# Downloads reference USB HID / DWC2 bare-metal driver sources to
# third_party/dwc2_refs/ for inspection while implementing bare-metal
# USB HID support for the BCM283x (Raspberry Pi 2B / QEMU raspi2b).
#
# Sources fetched:
#   1. Circle (rsta2/circle) — gold-standard BCM283x bare-metal USB library
#   2. Linux kernel HID core header  — USB HID Boot Protocol field offsets
#   3. USPI (rsta2/uspi)             — standalone DWC2 USB library
#
# None of these sources are compiled into the B-TRON build; they are
# reference only.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DEST="${REPO_ROOT}/third_party/dwc2_refs"

mkdir -p "${DEST}"

echo "=========================================================="
echo " B-TRON USB Reference Sources Download"
echo " Destination: ${DEST}"
echo "=========================================================="

# ──────────────────────────────────────────────────────────────
# Helper: download a single file with curl or wget
# ──────────────────────────────────────────────────────────────
fetch_file() {
    local url="$1"
    local out="$2"
    local desc="${3:-$out}"
    mkdir -p "$(dirname "${DEST}/${out}")"
    echo "[fetch] ${desc}"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "${url}" -o "${DEST}/${out}"
    elif command -v wget >/dev/null 2>&1; then
        wget -q "${url}" -O "${DEST}/${out}"
    else
        echo "[ERROR] Neither curl nor wget found. Install one and retry."
        exit 1
    fi
}

# ──────────────────────────────────────────────────────────────
# 1. Circle bare-metal USB library (rsta2/circle on GitHub)
#    Key files for DWC2 USB host + HID:
#      lib/usb/dwhcidevice.cpp   — DWC2 host controller init & transfer
#      lib/usb/dwhcirootport.cpp — root port enumeration
#      lib/usb/usbhiddevice.cpp  — HID device / boot-protocol
#      lib/usb/usbkeyboard.cpp   — USB HID keyboard driver
#      lib/usb/usbmouse.cpp      — USB HID mouse driver
#      include/circle/usb/dwhci.h— DWC2 register map
# ──────────────────────────────────────────────────────────────
CIRCLE_RAW="https://raw.githubusercontent.com/rsta2/circle/master"
echo ""
echo "── 1/3  Circle bare-metal USB library ──────────────────"

fetch_file "${CIRCLE_RAW}/lib/usb/dwhcidevice.cpp" \
    "circle/lib/usb/dwhcidevice.cpp" \
    "circle: DWC2 host controller (init + transfers)"

fetch_file "${CIRCLE_RAW}/lib/usb/dwhcirootport.cpp" \
    "circle/lib/usb/dwhcirootport.cpp" \
    "circle: Root port enumeration (SET_ADDRESS, SET_CONFIG)"

fetch_file "${CIRCLE_RAW}/lib/usb/usbhiddevice.cpp" \
    "circle/lib/usb/usbhiddevice.cpp" \
    "circle: USB HID base class"

fetch_file "${CIRCLE_RAW}/lib/usb/usbkeyboard.cpp" \
    "circle/lib/usb/usbkeyboard.cpp" \
    "circle: USB HID keyboard (boot protocol)"

fetch_file "${CIRCLE_RAW}/lib/usb/usbmouse.cpp" \
    "circle/lib/usb/usbmouse.cpp" \
    "circle: USB HID mouse (boot protocol)"

fetch_file "${CIRCLE_RAW}/include/circle/usb/dwhci.h" \
    "circle/include/circle/usb/dwhci.h" \
    "circle: DWC2 register definitions"

fetch_file "${CIRCLE_RAW}/include/circle/usb/usb.h" \
    "circle/include/circle/usb/usb.h" \
    "circle: USB standard definitions (descriptors, request types)"

fetch_file "${CIRCLE_RAW}/include/circle/usb/usbhiddevice.h" \
    "circle/include/circle/usb/usbhiddevice.h" \
    "circle: USB HID device header"

# ──────────────────────────────────────────────────────────────
# 2. Linux kernel: USB HID core header (hid.h)
#    Defines HID Boot Protocol field offsets and usage codes
# ──────────────────────────────────────────────────────────────
LINUX_RAW="https://raw.githubusercontent.com/torvalds/linux/master"
echo ""
echo "── 2/3  Linux kernel USB HID headers ───────────────────"

fetch_file "${LINUX_RAW}/include/linux/hid.h" \
    "linux/include/linux/hid.h" \
    "Linux: HID core (boot protocol field offsets, usage tables)"

fetch_file "${LINUX_RAW}/drivers/hid/hid-input.c" \
    "linux/drivers/hid/hid-input.c" \
    "Linux: HID input subsystem (scancode → keysym mapping)"

# ──────────────────────────────────────────────────────────────
# 3. USPI — standalone DWC2 USB library (rsta2/uspi on GitHub)
#    A minimal, dependency-free C implementation of DWC2 for Pi
# ──────────────────────────────────────────────────────────────
USPI_RAW="https://raw.githubusercontent.com/rsta2/uspi/master"
echo ""
echo "── 3/3  USPI standalone DWC2 USB library ───────────────"

fetch_file "${USPI_RAW}/lib/dwhcidevice.c" \
    "uspi/lib/dwhcidevice.c" \
    "USPI: DWC2 host controller (pure C)"

fetch_file "${USPI_RAW}/lib/dwhcirootport.c" \
    "uspi/lib/dwhcirootport.c" \
    "USPI: Root port enumeration (pure C)"

fetch_file "${USPI_RAW}/lib/usbhiddevice.c" \
    "uspi/lib/usbhiddevice.c" \
    "USPI: USB HID device (pure C)"

fetch_file "${USPI_RAW}/lib/usbkeyboard.c" \
    "uspi/lib/usbkeyboard.c" \
    "USPI: USB keyboard driver (pure C)"

fetch_file "${USPI_RAW}/lib/usbmouse.c" \
    "uspi/lib/usbmouse.c" \
    "USPI: USB mouse driver (pure C)"

fetch_file "${USPI_RAW}/include/uspi/dwhci.h" \
    "uspi/include/uspi/dwhci.h" \
    "USPI: DWC2 register map (pure C header)"

fetch_file "${USPI_RAW}/include/uspi/usb.h" \
    "uspi/include/uspi/usb.h" \
    "USPI: USB standard definitions"

# ──────────────────────────────────────────────────────────────
# Write an index README
# ──────────────────────────────────────────────────────────────
cat > "${DEST}/README.md" << 'EOF'
# DWC2 / USB HID Reference Sources

These sources are downloaded for **reference only** — they are NOT compiled
into the B-TRON build.

## Structure

```
dwc2_refs/
├── circle/         # rsta2/circle: gold-standard BCM283x bare-metal C++ USB
│   ├── include/circle/usb/
│   │   ├── dwhci.h          DWC2 register map
│   │   ├── usb.h            USB standard definitions
│   │   └── usbhiddevice.h   HID device header
│   └── lib/usb/
│       ├── dwhcidevice.cpp   DWC2 host controller init & transfer engine
│       ├── dwhcirootport.cpp Root port enumeration (SET_ADDRESS, SET_CONFIG)
│       ├── usbhiddevice.cpp  HID base class (boot protocol)
│       ├── usbkeyboard.cpp   Keyboard driver (HID boot protocol)
│       └── usbmouse.cpp      Mouse driver (HID boot protocol)
├── uspi/           # rsta2/uspi: same logic, pure C, no C++ dependencies
│   ├── include/uspi/
│   │   ├── dwhci.h
│   │   └── usb.h
│   └── lib/
│       ├── dwhcidevice.c
│       ├── dwhcirootport.c
│       ├── usbhiddevice.c
│       ├── usbkeyboard.c
│       └── usbmouse.c
└── linux/          # Linux kernel USB HID layer for scancode reference
    ├── include/linux/hid.h
    └── drivers/hid/hid-input.c
```

## Key DWC2 Concepts

- **Peripheral base (QEMU raspi2b)**: `0x20000000` (BCM2835 map)
- **DWC2 OTG offset**: `+ 0x00980000` → `0x20980000`
- **Enumeration**: SET_ADDRESS → SET_CONFIGURATION → SET_PROTOCOL (0 = Boot) → SET_IDLE
- **Keyboard Boot Protocol**: 8-byte report: [modifiers][reserved][key0..key5]
- **Mouse Boot Protocol**:    4-byte report: [buttons][dx][dy][wheel]
- **PID toggle**: alternate DATA0/DATA1 on each successful interrupt IN

## Licenses

- Circle: GPLv3 (rsta2/circle) — do not include code in proprietary builds
- USPI: MIT-like custom (rsta2/uspi)
- Linux: GPLv2

These sources are in `third_party/` for educational reference only.
EOF

echo ""
echo "=========================================================="
echo " Done! Reference sources saved to:"
echo "   ${DEST}"
echo ""
echo " Key files to study for DWC2 enumeration:"
echo "   uspi/lib/dwhcirootport.c  — SET_ADDRESS / SET_CONFIG (pure C)"
echo "   uspi/lib/dwhcidevice.c    — control + interrupt transfers"
echo "   uspi/lib/usbkeyboard.c    — HID Boot Protocol keyboard"
echo "   uspi/lib/usbmouse.c       — HID Boot Protocol mouse"
echo "=========================================================="
