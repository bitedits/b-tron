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
- **Graphics Pipeline**: DMAC Channel 2 (GIF) packet submission for 2D `PRIM_SPRITE` rasterization.
- **Video Timing**: 640 x 480 @ 32-bpp RGBA with hardware VSync retrace and double buffering.
- **Input Channels**: DualShock 2 Pad, OHCI USB Keyboard/Mouse, and EE SIO0 UART console.

### Driver Files

| File | Role |
|:---|:---|
| [`src/drivers/ps2/boot_ps2.s`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/boot_ps2.s) | EE reset vector, `$gp` and `$sp` setup, unrolled BSS wipe, BIOS syscall wrappers (`SetGsCrt`, `PutIMR`), jumps to `ps2_kernel_main`. |
| [`src/drivers/ps2/ps2.ld`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2.ld) | Memory layout script linking `.text` at `0x00100000`. |
| [`src/drivers/ps2/ps2_gs.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_gs.h) | Privileged GS registers (`0x12000000`: `PMODE`, `SMODE2`, `DISPFB1`, `DISPLAY1`, `CSR`) and GIF DMA registers (`0x1000A000`). |
| [`src/drivers/ps2/ps2_gs.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_gs.c) | Hardware display initialization, VSync synchronization (`ps2_gs_vsync`), tear-free double buffering (`ps2_gs_swap_buffers`), GIF DMA sprite rasterizer (`ps2_gs_draw_rect`), and hardware cursor overlay/erasure. |
| [`src/drivers/ps2/ps2_sio.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_sio.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_sio.c) | Cleanroom SIO0 hardware UART driver (`0x1000F180` / `KPUTCHAR`). |
| [`src/drivers/ps2/ps2_pad.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_pad.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_pad.c) | Cleanroom DualShock 2 controller driver: analog stick velocity integration, deadband filtering, button edge detection, and event mapping. |
| [`src/drivers/ps2/ps2_usb.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_usb.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_usb.c) | Cleanroom USB Open Host Controller Interface (OHCI) driver (`0xBF801600`) and standard USB HID Boot Protocol keyboard/mouse decoders. |
| [`src/cores/core_ps2.c`](file:///Users/tonpa/depot/bitedits/btron/src/cores/core_ps2.c) | Platform core adapter: multi-window application suite (Workbench, B-Editor, TAD Cabinet, Settings), Japanese TIP/IME status badge, interactive SIO0 shell, event queue, and RTOS heartbeat. |
| [`scripts/test_ps2.sh`](file:///Users/tonpa/depot/bitedits/btron/scripts/test_ps2.sh) | Automated verification suite checking ELF architecture, entry point, driver symbols, R5900 opcodes, and ISO image (19/19 tests). |

---

### 2.1 Hardware VSync Retrace & Double Buffering

The PlayStation 2 Graphics Synthesizer contains 4 MB of ultra-high-bandwidth embedded DRAM (eDRAM). In standard 640x480 CT32 mode (32 bits per pixel), each frame buffer requires `640 * 480 * 4 = 1,228,800` bytes (150 eDRAM pages of 8192 bytes each).

B-System configures two distinct page regions within eDRAM:
- **Front Buffer Page (`FBP0 = 0`)**: Displayed by PCRTC (`GS_DISPFB1`).
- **Back Buffer Page (`FBP1 = 160`)**: Targeted by the drawing engine (`GS_FRAME_1`).

#### Synchronization Flow (`ps2_gs_swap_buffers`)
1. **Vertical Blank Acknowledge**: Writes `1ULL << 3` to `GS_CSR` (`0x12001000`) to clear any pending VSync interrupt, then polls bit 3 until the GS enters vertical retrace.
2. **PCRTC Display Flip**: Updates `GS_DISPFB1` to point to the freshly rendered buffer.
3. **GIF Drawing Target Flip**: Dispatches a lightweight 2-QW GIF DMA packet over DMAC Channel 2 updating `GS_REG_FRAME_1` (`0x4C`) to the opposite page for subsequent draw calls.
4. **Result**: 100% tear-free, rock-solid 60.0 FPS presentation.

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
   - Real-time display configuration (640x480 NTSC, CT32 RGBA, VSync status, DualShock 2 status, USB status, and current TIP/IME language setting).

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
