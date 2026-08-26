# B-TRON Retro OS for Synrc VE OS.1

A cleanroom recreation of the **B-TRON** (Business TRON / TRON OS Architecture for personal computers)
specification and retro desktop environment. Designed using standard C99, rendering through SDL2,
and organized into a authentic Japanese BTRON tree structure.

## Specification

* [B-TRON SPEC 3.20](https://bitedits.github.io/btron/doc/)
* [B-FREE OS 1994](https://bitedits.github.io/btron/b-free/)
* [B-SYSTEM OS 2026](https://bitedits.github.io/btron/b-system/)

## Raspberry Pi 2/3 T-Kernel 2.0 `Yokobayashi`

```
$ make tkernel
$ make run-tkernel
```

<img width="1070" height="844" alt="Screenshot 2026-08-22 at 23 22 22" src="https://bitedits.github.io/btron/doc/img/yokobayashi.png" />

## QEMU VirtIO T-Kernel 2.0 `Sakamura`

```
$ make sakamura
$ ./btron-sakamura.elf
```

<img width="1070" height="844" alt="Screenshot 2026-08-22 at 23 22 22" src="https://bitedits.github.io/btron/b-system/img/sakamura.png" />

## QEMU VirtIO `Synrc`

```
$ make qemu
$ make run-qemu
```

<img width="1070" height="844" alt="Screenshot 2026-08-22 at 23 22 22" src="https://bitedits.github.io/btron/b-system/img/qemu.png" />

## POSIX `Light`

```
$ make posix
$ make run-posix
```

<img width="1070" height="844" alt="Screenshot 2026-08-22 at 23 22 22" src="https://bitedits.github.io/btron/b-system/img/posix.png" />

## Japanese BTRON Lineage & Subsystem Layout

```
btron/
├── Makefile                    # Plain BSD/BeOS/TRON style Makefile
├── README.md                   # Cleanroom architecture & BTRON spec documentation
├── include/                    # Authentic Japanese BTRON Header Tree
│   └── btron/
│       ├── btron.h             # Master include header
│       ├── types.h             # Fundamental TRON types (W, H, B, UW, ID, ER, etc.)
│       ├── error.h             # TRON error codes (E_OK, E_SYS, E_NOMEM, E_PAR, etc.)
│       ├── itron.h             # μITRON Real-Time Kernel Primitives
│       ├── dp.h                # Display Primitives (Graphics engine)
│       ├── wnd.h               # Sakamura Window Manager APIs
│       ├── event.h             # BTRON System Event Queue
│       ├── vobj.h              # Real Object / Virtual Object Hyper-Data Model Engine
│       ├── troncode.h          # TRON Multilingual Character Code & Font Engine
│       └── desktop.h           # Desktop Shell & Panel Manager APIs
├── src/                        # Core Subsystems Implementation
│   ├── kernel/                 # μITRON Task & Synchronization Subsystem
│   ├── graphics/               # Display Primitives (DP) Vector & Raster Graphics
│   ├── window/                 # Sakamura BTRON Window Manager & Event Dispatcher
│   ├── vobject/                # Real Object / Virtual Object Hyper-Data Engine
│   ├── font/                   # TRON Character Code & Pixel Font Renderer
│   └── desktop/                # Cho-Kanji / BTRON Desktop Compositor & Main Launcher
└── apps/                       # Authentic BTRON Accessories
    ├── vobj_manager.c          # Real Object Cabinet & Virtual Object Explorer Window
    ├── t_editor.c              # TRON Text Editor (T-Editor) Window
    └── gterm.c                 # BTRON Terminal Shell (gterm) Window
```

## Features

1. **True Japanese BTRON API Header Hierarchy**:
   - Includes `<btron/btron.h>`, `<btron/types.h>`, `<btron/itron.h>`, `<btron/dp.h>`, `<btron/vobj.h>`, `<btron/wnd.h>`, `<btron/event.h>`, and `<btron/troncode.h>`.
   - Native TRON type definitions: `W`, `H`, `B`, `UW`, `UH`, `UB`, `VW`, `ID`, `ER`, `RECT`, `PNT`, `PAT`, `COLOR`.
   - Real-time kernel primitives: `cre_tsk`, `sta_tsk`, `slp_tsk`, `wup_tsk`, `cre_sem`, `wai_sem`, `sig_sem`.

2. **Real Object / Virtual Object Hyper-Data Model Engine (`vobj`)**:
   - Implements Sakamura's legendary hyper-data object engine where documents contain live embedded pointers (Virtual Objects) to Real Objects stored in disk cabinets.

3. **Sakamura Cho-Kanji Retro Desktop & Window Manager**:
   - Classic Sakamura teal desktop workspace with top status panel, clock, desktop cabinet launcher, and window z-ordering.
   - Double-bordered retro window frames with titlebars, close buttons, window dragging, and focus.

4. **BTRON Accessories**:
   - **`gterm`**: BTRON Terminal Console window.
   - **`t_editor`**: TRON Text Editor with virtual object icon embedding.
   - **`vobj_manager`**: Real Object Cabinet & Virtual Object Link Explorer.

## Building & Running

In the spirit of NetBSD, BeOS, and TRON, compilation requires only a plain **BSD Makefile**:

```bash
make
./btron
```

### Clean Build

```bash
make clean
```
