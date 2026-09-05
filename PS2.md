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
│  src/drivers/ps2/ (GS, SIO)         │  src/drivers/mips/ (16550 UART)  │
└────────────────────────────────────────────────────────────────────────┘
```

### Cleanroom Philosophy

- **Zero Proprietary SDKs**: 100% cleanroom implementation. No proprietary Sony headers (`sifrpc.h`, etc.), no Sony libraries, and no copyrighted code.

- **Direct Hardware Access**: Privileged MMIO registers for the Graphics Synthesizer (GS), SIO, and MIPS CP0 status/timer registers are driven directly.

## 2. Target 8: Sony PlayStation 2 (`make run-ps2`)

### Hardware Overview

- **CPU**: Sony Emotion Engine (EE) MIPS R5900 (MIPS-III little-endian 64-bit core with 128-bit SIMD vector units @ 294.912 MHz).
- **RAM**: 32 MB RDRAM (Physical addresses `0x00000000` to `0x02000000`).
- **Load Address**: `0x00100000` (1 MB mark into physical RAM, standard PS2 homebrew entry).
- **Stack Pointer**: `0x01FF0000` (top of 32 MB RDRAM with 64 KB safety headroom).
- **Display Controller**: Graphics Synthesizer (GS) with 4 MB embedded DRAM (eDRAM).

### Driver Files

| File | Role |
|:---|:---|
| [`src/drivers/ps2/boot_ps2.s`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/boot_ps2.s) | EE reset vector, `$gp` and `$sp` setup, unrolled BSS wipe, jumps to `ps2_kernel_main`. |
| [`src/drivers/ps2/ps2.ld`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2.ld) | Memory layout script linking `.text` at `0x00100000`. |
| [`src/drivers/ps2/ps2_gs.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_gs.h) | Privileged GS register declarations (`0x12000000`: `PMODE`, `SMODE2`, `DISPFB1`, `DISPLAY1`, `CSR`). |
| [`src/drivers/ps2/ps2_gs.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_gs.c) | 640x480 @ 32-bpp progressive RGBA display init, VBlank synchronization, framebuffer management. |
| [`src/drivers/ps2/ps2_sio.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_sio.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/ps2/ps2_sio.c) | Cleanroom console output. |
| [`src/cores/core_ps2.c`](file:///Users/tonpa/depot/bitedits/btron/src/cores/core_ps2.c) | Platform core adapter: renders B-System Workbench desktop, handles system ticks, and runs executive loop. |

### Architecture & Compiler Details

- **Architecture**: The Emotion Engine R5900 implements **MIPS-III** with Sony 128-bit multimedia extensions.
- **Compiler Flags**: `-march=mips3 -mabi=32` is strictly required. Compiling with `mips32r2` causes the compiler to emit instructions unsupported by the R5900 (such as `seb` / sign-extend-byte), which trip Coprocessor / TLB exceptions in PCSX2.
- **Register Addressing**: GS privileged registers are mapped directly into user space at `0x12000000` (CSR @ `0x12001000`).

### Running with PCSX2

PCSX2 supports both direct ELF execution and virtual CD/DVD disc images:

- **Direct ELF (`make run-ps2`)** [Recommended]: Executes `btron-ps2.elf` directly using PCSX2's native Host filesystem (`-elf`). This completely bypasses optical drive/disc formatting issues and eliminates "No Image" errors.
- **Disc ISO (`make run-ps2 ISO=1`)**: Boots `btron-ps2.iso` via virtual CDVD (`-fastboot`).

### Building & Running PS2

```bash
# Build ELF and packaged ISO:
make ps2

# Launch ELF directly in PCSX2 (No disc/image errors):
make run-ps2

# Or launch via ISO image:
make run-ps2 ISO=1
```

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

### Driver Files

| File | Role |
|:---|:---|
| [`src/drivers/mips/boot_mips.s`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/mips/boot_mips.s) | Reset entry point, sets up `$sp = 0x81F00000` in KSEG0, unrolled BSS wipe, jumps to `mips_kernel_main`. |
| [`src/drivers/mips/mips_qemu.ld`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/mips/mips_qemu.ld) | Memory layout script linking `.text` at KSEG0 cached address `0x80100000`. |
| [`src/drivers/mips/mips_uart.h`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/mips/mips_uart.h) & [`.c`](file:///Users/tonpa/depot/bitedits/btron/src/drivers/mips/mips_uart.c) | Cleanroom NS16550 UART driver for COM1 (`0xB80003F8`). Non-blocking polling & character echo. |
| [`src/cores/core_mips.c`](file:///Users/tonpa/depot/bitedits/btron/src/cores/core_mips.c) | Platform core adapter: boot banner, interactive serial CLI shell, RTOS scheduler heartbeat. |
| [`scripts/test_mips.sh`](file:///Users/tonpa/depot/bitedits/btron/scripts/test_mips.sh) | Automated headless CI test runner checking boot markers in QEMU Malta. |

### Building & Running MIPS

```bash
# Build MIPS kernel ELF:
make mips

# Run interactive serial console on QEMU Malta:
make run-mips

# Run on QEMU MIPS Magnum with ntprom.raw BIOS:
make run-mips MAGNUM=1

# Automated headless regression test:
make test-mips
```

## 4. Cross-Compilation Toolchain

You do not need to install an external GCC cross-compiler. The standard LLVM/Clang and GNU Binutils tools available on macOS compile both targets natively:

1. **C Compiler (LLVM Clang)**:
   ```bash
   /opt/homebrew/opt/llvm/bin/clang --target=mipsel-unknown-elf -march=mips32r2 -mabi=32 -ffreestanding -nostdlib
   ```
   - Target: `mipsel-unknown-elf` (MIPS 32-bit little-endian).
   - CPU Architecture: `-march=mips32r2` (compatible with both 32-bit and 64-bit MIPS cores, including R5900, 24Kf, 5KEc, and R4000).
   - ABI: `-mabi=32` (standard MIPS o32 ABI).

2. **Linker**:
   ```bash
   mipsel-linux-gnu-ld -T <script.ld> <objects...> -o <target.elf>
   ```
   *(Installed via `brew install mipsel-linux-gnu-binutils`). Fallback is `/opt/homebrew/Cellar/lld/*/bin/ld.lld -EL`.*

3. **Disassembly & Inspection**:
   ```bash
   # Inspect headers:
   llvm-readelf -h btron-ps2.elf
   llvm-readelf -h btron-mips.elf

   # Disassemble startup code:
   llvm-objdump -d btron-mips.elf | head -n 40
   ```

## 5. Developer Quick Reference

| Command | Action | Platform / Output |
|:---|:---|:---|
| `make ps2` | Build PS2 ELF & ISO | `btron-ps2.elf` and `btron-ps2.iso` |
| `make run-ps2` | Launch in PCSX2 | Boots `btron-ps2.iso` in PCSX2 |
| `make mips` | Build MIPS ELF | `btron-mips.elf` |
| `make run-mips` | Launch in QEMU | Interactive console on Malta (`qemu-system-mipsel`) |
| `make run-mips MAGNUM=1` | Launch Magnum | Boots `ntprom.raw` on `qemu-system-mips64el -M magnum` |
| `make test-mips` | Run automated CI test | Headless validation of all 8 kernel boot markers |
| `make test` | Run full test suite | Validates all 12 B-System test suites (100% pass) |
| `make clean` | Clean all outputs | Removes all `.elf`, `.iso`, `.o`, and test binaries |

# Credits

* Namdak Tonpa and Grok 4.5
