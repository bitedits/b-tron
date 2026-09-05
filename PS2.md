# B-System (BTRON 3.20) PlayStation 2 & MIPS Developer Guide

This document is the technical porting reference for kernel and driver developers working on the **Sony PlayStation 2 (Emotion Engine)** and **Bare-Metal MIPS (QEMU Malta & Magnum)** targets of the B-System operating system.

## 1. Architectural Principles: Image > Core > Kernel

Like all B-System workstation ports, the PS2 and MIPS targets adhere strictly to the 3-tier **Cleanroom** architecture:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        Image (Final Run Target)                        │
│             btron-ps2.iso (PCSX2)  /  btron-mips.elf (QEMU)            │
├────────────────────────────────────────────────────────────────────────┤
│                       Cores (src/cores/) Layer                         │
│  Platform Integration: Memory Maps, Hardware Framebuffer, Console      │
│  src/cores/core_ps2.c               │  src/cores/core_mips.c           │
├────────────────────────────────────────────────────────────────────────┤
│                       Kernel (src/kernel/) Layer                       │
│  Pure Portable RTOS Executive: uITRON 3.0 Primitives                   │
│  Tasks, Semaphores, Mutexes, Mailboxes, Memory Pools, Timers, Libstr   │
├────────────────────────────────────────────────────────────────────────┤
│                       Drivers (src/drivers/) Layer                     │
│  Hardware Bootstrap, Linker Scripts, Silicon Registers                 │
│  src/drivers/ps2/ (GS, SIO, Pad, USB)│ src/drivers/mips/ (16550 UART) │
└────────────────────────────────────────────────────────────────────────┘
```

### Cleanroom Philosophy

- **Zero Proprietary SDKs**: 100% cleanroom implementation. No proprietary Sony headers (`sifrpc.h`, etc.), no Sony libraries, and no copyrighted code.
- **Direct Hardware Access**: Privileged MMIO registers for the Graphics Synthesizer (GS), SIO0 UART, DualShock 2 SIO2 controller, and USB OHCI are driven directly.
- **Microkernel Isolation**: `src/kernel/` remains pristine, portable, and untouched across all architectures.

---

## 2. Target 8: Sony PlayStation 2 (`make run-ps2`)

### Hardware Overview

- **CPU**: Sony Emotion Engine (EE) MIPS R5900 (MIPS-III little-endian 64-bit core with 128-bit SIMD vector units @ 294.912 MHz).
- **RAM**: 32 MB RDRAM (Physical addresses `0x00000000` to `0x02000000`).
- **Load Address**: `0x00100000` (1 MB mark into physical RAM, standard PS2 homebrew entry).
- **Stack Pointer**: `0x01FF0000` (top of 32 MB RDRAM with 64 KB safety headroom).
- **Display Controller**: Graphics Synthesizer (GS) with 4 MB embedded DRAM (eDRAM).
- **Graphics Pipeline**: DMAC Channel 2 (GIF) Host-to-Local image blitting (`BITBLTBUF`, `TRXPOS`, `TRXREG`, `TRXDIR`).
- **Video Timing**: 640 x 448 @ 32-bpp RGBA with hardware VSync retrace (Standard NTSC Interlaced Field Mode).
- **Input Channels**: DualShock 2 Pad, OHCI USB Keyboard/Mouse, and EE SIO0 UART console.

### Driver Files

| File | Role |
|:---|:---|
| [`src/drivers/ps2/boot_ps2.s`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/boot_ps2.s) | EE reset vector, `$gp` and `$sp` setup, unrolled BSS wipe, BIOS syscall wrappers (`SetGsCrt`, `PutIMR`), jumps to `ps2_kernel_main`. |
| [`src/drivers/ps2/ps2.ld`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2.ld) | Memory layout script linking `.text` at `0x00100000`. |
| [`src/drivers/ps2/ps2_gs.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_gs.h) | Privileged GS registers (`0x12000000`: `PMODE`, `SMODE2`, `DISPFB1`, `DISPLAY1`, `CSR`) and GIF DMA registers (`0x1000A000`). |
| [`src/drivers/ps2/ps2_gs.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_gs.c) | Hardware display initialization, VSync synchronization (`ps2_gs_vsync`), Host-to-Local GIF DMA blitter (`ps2_gs_flush`), uncached KSEG1 RDRAM framebuffer, and non-destructive cursor restoration. |
| [`src/drivers/ps2/ps2_font.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_font.h) | 8x8 ASCII bitmap font table (128 characters) for crisp, authentic BTRON UI text rendering. |
| [`src/drivers/ps2/ps2_sio.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_sio.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_sio.c) | Cleanroom SIO0 hardware UART driver (`0x1000F180` / `KPUTCHAR`). |
| [`src/drivers/ps2/ps2_pad.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_pad.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_pad.c) | Cleanroom DualShock 2 controller driver: analog stick velocity integration, deadband filtering, button edge detection, and event mapping. |
| [`src/drivers/ps2/ps2_usb.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_usb.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_usb.c) | Cleanroom USB Open Host Controller Interface (OHCI) driver (`0xBF801600`) and standard USB HID Boot Protocol keyboard/mouse decoders. |
| [`src/cores/core_ps2.c`](file:///Users/tonpa/depot/bitedits/btron/src/cores/core_ps2.c) | Platform core adapter: multi-window application suite (Workbench, B-Editor, TAD Cabinet, Settings), Japanese TIP/IME status badge, interactive SIO0 shell, event queue, and RTOS heartbeat. |
| [`scripts/test_ps2.sh`](file:///Users/tonpa/depot/bitedits/btron/scripts/test_ps2.sh) | Automated verification suite checking ELF architecture, entry point, driver symbols, R5900 opcodes, and ISO image (19/19 tests). |

---

### 2.1 Graphics Synthesizer (GS) Framebuffer Architecture

The PlayStation 2 Graphics Synthesizer contains 4 MB of ultra-high-bandwidth embedded DRAM (eDRAM). Memory inside eDRAM is structured into **pages** (2048 32-bit words = 8192 bytes) and **blocks** (64 words = 256 bytes) with non-linear column-interleaved pixel swizzling.

#### The Host-to-Local GIF DMA Blitter Architecture
Rather than rendering individual primitives through the GS rasterizer (which is prone to sub-pixel rounding errors, page-boundary clipping bugs, and high DMA packet overhead), B-System uses the hardware **Host-to-Local Blitter**:

1. **Uncached RDRAM Backing Store (`ps2_fb_memory`)**:
   - The CPU maintains a linear 640 x 448 x 32-bpp frame buffer in RDRAM (`71,680` quadwords = `1,146,880` bytes).
   - All drawing functions (`ps2_gs_draw_rect`, `ps2_draw_char`, `ps2_gs_putpixel`) write through an uncached KSEG1 pointer (`0xA0xxxxxx`), bypassing the EE L1 Data Cache so RDRAM is always 100% coherent before DMA transfers.
2. **GS Transmission Setup Packet (`ps2_gs_flush`)**:
   - A 5-QW setup packet primes the GS:
     - `BITBLTBUF (0x50)`: `DBA = 0`, `DBW = 10` (640 / 64), `DPSM = GS_PSM_CT32 (0)`.
     - `TRXPOS (0x51)`: `DSAX = 0, DSAY = 0`.
     - `TRXREG (0x52)`: `RRW = 640, RRH = 448`.
     - `TRXDIR (0x53)`: `0` (Host $\rightarrow$ Local eDRAM).
3. **Hardware Swizzling on Arrival**:
   - `ps2_fb_memory` is streamed via DMAC Channel 2 in 8 chunks of 8,960 quadwords with `GIFTAG(FLG = IMAGE)`.
   - The GS internal blitter automatically and correctly swizzles linear RDRAM pixels into eDRAM pages and blocks on arrival.
4. **Verified NTSC Video Timings**:
   - `ps2_set_gs_crt(1, 2, 0)`: Interlaced, NTSC, Field mode.
   - `PMODE = 0xFF63ULL`: Circuit 1 + Circuit 2 enabled, `SLBG = 0` (Framebuffer output selected).
   - `SMODE2 = 0x01ULL`: Interlaced, Field mode.
   - `DISPFB1` & `DISPFB2 = 0x1400ULL`: Base address page 0 (`FBP = 0`), buffer width 10 blocks (`FBW = 10`), 32-bit RGBA (`PSM = 0`).
   - `DISPLAY1` & `DISPLAY2 = 0x001bf9ff0983227cULL`: $DX=636$, $DY=50$, $MAGH=3$ (4x), $MAGV=1$ (2x), $DW=2559$, $DH=447$.
5. **Non-Destructive Cursor Restoration**:
   - Mouse cursor drawing backs up background pixels into `cursor_saved[16 * 16]`. Erasing the cursor restores the exact original background pixels with zero smearing or artifacts.

---

### 2.2 DualShock 2 Controller Driver (`ps2_pad`)

The PS2 gamepad input system provides direct analog pointer manipulation and tactile shortcuts:

| Controller Input | B-System Action | Description |
|:---|:---|:---|
| **Left Analog Stick (LX, LY)** | Smooth Mouse Pointer Velocity | Integrated with a deadzone filter (`abs(axis - 128) > 16`) to steer the 16x16 desktop arrow cursor. |
| **Cross (✕)** | Left Mouse Click | Selects icons, clicks buttons, and focuses windows. |
| **Square (□)** | Right Mouse Click | Activates context menus and property dialogs. |
| **Circle (◯)** | Cancel / ESC | Dismisses active dropdown menus and dialogs. |
| **Triangle (△)** | Cycle TIP / IME Mode | Rotates language input: ASCII → Hiragana (`あ`) → Katakana (`ア`) → Tibetan (`བོད`). |
| **Start** | Toggle Desktop Menu | Opens or closes the top system menu bar. |
| **Select / L1 / R1** | Cycle Active Window | Shifts focus between open desktop windows. |
| **D-Pad (Up, Down, Left, Right)** | Discrete Navigation Keys | Injects `BTRON_KEY_UP/DOWN/LEFT/RIGHT` events into the event queue. |

Developers can simulate controller state via the serial shell using:
```bash
pad <btns_hex> [lx ly]
# Example: Click Cross button with centered sticks:
btron-ps2> pad bfff 128 128
```

---

### 2.3 USB Subsystem (OHCI Host Controller)

The PlayStation 2 motherboard includes two USB 1.1 ports driven by an Open Host Controller Interface (OHCI) block at `0x1F801600` (uncached KSEG1: `0xBF801600`).

The cleanroom driver (`ps2_usb.c`):
- Verifies controller presence by checking `HcRevision` (`0x10` or `0x11`).
- Transitions the host controller state machine to `OHCI_CTRL_HCFS_OPER` (Operational).
- Monitors `HcRhPortStatus[0]` and `HcRhPortStatus[1]` for port connection status (`OHCI_PORT_CCS`).
- Implements standard USB HID Boot Protocol parsers:
  - **Keyboard (8-byte report)**: Translates standard HID scan codes (modifiers + keycodes) to B-TRON character events.
  - **Mouse (3-byte / 4-byte report)**: Extracts relative displacements `(dx, dy)` and button states (Left, Right, Middle) and injects `EV_MOUSE_MOVE` and `EV_BUT_DOWN` events.

---

### 2.4 Multi-Window Desktop Application Suite

`core_ps2.c` delivers a complete, multi-window B-System workstation desktop rendered directly through GIF DMA primitives:

1. **Window 1: Workbench System & RTOS Monitor**
   - Live display of CPU architecture (R5900 Little-Endian), 32 MB RDRAM memory pool, 4 MB eDRAM VRAM layout, active RTOS tasks (`ps2_idle`, `ps2_desktop`, `ps2_shell`), and real-time scheduler state.
2. **Window 2: B-Editor (Interactive Text Editor)**
   - Authentic text editor with full editing buffer, typing support from keyboard/SIO0, backspace deletion, and animated caret indicator (`_`).
3. **Window 3: TAD Cabinet & Real Object Browser**
   - Visual file and virtual object manager displaying BTRON record headers (`Readme.tad`, `System/`, `Editor.app`, `Settings.app`, `BootSound.snd`, `Cabinet.vobj`).
4. **Window 4: Control Panel / System Settings**
   - Real-time display configuration (640x448 NTSC, CT32 RGBA, VSync status, DualShock 2 status, USB status, and current TIP/IME language setting).

#### Window Focus & Navigation
- **Click-to-Focus**: Clicking anywhere inside a window's bounds raises it to the foreground (`s_active_window`).
- **Menu Bar**: Top 24px bar includes application menus and the top-right Japanese TIP badge. Clicking the badge cycles input modes immediately.
- **Status Bar**: Bottom 20px bar displays system readiness, mouse coordinates, and navigation guidance.

---

### 2.5 SIO0 Interactive Shell Commands

The SIO0 UART console (`115200 8N1`) provides an interactive debugging shell:

| Command | Action |
|:---|:---|
| `help` | List all available shell commands. |
| `info` | Display hardware specifications, CPU frequency, and memory layout. |
| `apps` | List all desktop applications and identify currently focused window. |
| `open <app>` | Open or focus an application (`workbench`, `editor`, `tad`, `settings`). |
| `focus <1-4>` | Directly focus window 1, 2, 3, or 4. |
| `tip [mode]` | Switch TIP/IME mode (`ascii`, `hira`, `kata`, `tibetan`) or cycle if no argument. |
| `tasks` | Dump active RTOS task table and stack pointers. |
| `mem` | Display physical RDRAM, kernel image, and VRAM memory usage. |
| `desktop` | Force an immediate full-screen redraw of the Workbench desktop. |
| `status` | Show mouse `(x, y)`, active window ID, TIP mode, and event queue count. |
| `mouse <x> <y>` | Set absolute mouse cursor coordinates. |
| `move <dx> <dy>` | Displace mouse cursor relative to current position. |
| `click [1\|2]` | Simulate left (1) or right (2) mouse button click. |
| `key <char>` | Inject a keystroke into the active window. |
| `pad <hex> [lx ly]` | Inject simulated DualShock 2 controller state. |
| `reboot` | Halt the Emotion Engine. |

---

### Running with PCSX2

PCSX2 supports both direct ELF execution and virtual CD/DVD disc images:

- **Direct ELF (`make run-ps2`)** [Recommended]: Executes `btron-ps2.elf` directly using PCSX2's native Host filesystem (`-elf`).
- **Disc ISO (`make run-ps2 ISO=1`)**: Boots `btron-ps2.iso` via virtual CDVD (`-fastboot`).

```bash
# Build ELF and packaged ISO:
make ps2

# Launch ELF directly in PCSX2:
make run-ps2

# Automated verification suite:
make test-ps2
```

---

## 3. Target 9: Bare-Metal MIPS (`make run-mips`)

### Hardware Overview

B-System supports two QEMU MIPS platforms:

1. **MIPS Malta Core LV** (Default):
   - QEMU target: `qemu-system-mipsel -M malta -cpu 24Kf`
   - Memory: 256 MB RAM mapped into KSEG0 at `0x80000000`.
   - Entry point: `0x80100000`.
   - Console: Standard NS16550 UART at ISA COM1 physical `0x180003F8` / KSEG1 `0xB80003F8` (115200 8N1).

2. **MIPS Magnum R4000** (Legacy Jazz Workstation):
   - QEMU target: `qemu-system-mips64el -M magnum -bios ./ntprom.raw -m 64M`
   - Requires `./ntprom.raw` ARC firmware image in the repository root.

```bash
# Build MIPS kernel ELF:
make mips

# Run interactive serial console on QEMU Malta:
make run-mips

# Automated headless regression test:
make test-mips
```

---

## 4. Cross-Compilation Toolchain

Both targets compile natively without external GCC toolchains using LLVM/Clang and GNU Binutils on macOS:

1. **C Compiler (LLVM Clang)**:
   ```bash
   /opt/homebrew/opt/llvm/bin/clang --target=mipsel-unknown-elf -march=mips3 -mabi=32 -ffreestanding -nostdlib
   ```
   - Target: `mipsel-unknown-elf` (MIPS 32-bit little-endian).
   - CPU Architecture: `-march=mips3` (Emotion Engine R5900 compatible, zero invalid MIPS32r2 opcodes).
   - ABI: `-mabi=32` (standard MIPS o32 ABI).

2. **Linker**:
   ```bash
   mipsel-linux-gnu-ld -T <script.ld> <objects...> -o <target.elf>
   ```

---

## 5. Developer Quick Reference

| Command | Action | Platform / Output |
|:---|:---|:---|
| `make ps2` | Build PS2 ELF & ISO | `btron-ps2.elf` and `btron-ps2.iso` |
| `make run-ps2` | Launch in PCSX2 | Direct ELF execution on PCSX2 |
| `make run-ps2 ISO=1` | Launch ISO in PCSX2 | Bootable CDVD execution |
| `make test-ps2` | Run PS2 automated test | 19/19 driver and ELF assertions |
| `make mips` | Build MIPS ELF | `btron-mips.elf` |
| `make run-mips` | Launch in QEMU | Interactive console on Malta (`qemu-system-mipsel`) |
| `make test-mips` | Run MIPS automated test | Headless validation of all 8 kernel boot markers |
| `make test` | Run full test suite | Validates all 12 B-System test suites (100% pass) |
| `make clean` | Clean all outputs | Removes all `.elf`, `.iso`, `.o`, and test binaries |

---

# Credits

* Namdak Tonpa and Grok 4.5
