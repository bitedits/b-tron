# TRON Application Databus (TAD) in B-System

This document provides a comprehensive technical overview of **TAD (TRON Application Databus)** in B-System, explaining the `./dharma/` and `./tad_bin/` directories, their generation pipeline, binary segment format conforming to **BTRON3 SPEC 3.20**, compatibility with **B-right/V Cho-Kanji (超漢字)**, and their runtime usage across the **Cabinet Explorer** and **Native TAD Document Browser**.

---

## 1. Overview: What is TAD?

In the TRON architecture designed by Prof. Ken Sakamura, **TAD (TRON Application Databus)** is the foundational, multi-modal data exchange format. Unlike standard linear text files (ASCII/UTF-8) or rigid relational databases, TAD is a **hyper-data format** composed of a continuous stream of text and embedded **Fusen (付箋 - Sticky Tabs/Tags)**.

In B-System, TAD serves two critical roles:
1. **Hyper-Object Document Storage:** Organizes system documentation, books, and user documents into first-class **Real Objects (実身 - Jitsushin)** with embedded **Virtual Object (仮身 - Kashin)** links.
2. **Deterministic Bounded Layout:** A cleanroom linear layout engine replaces legacy complex DOM trees with a single-pass stream engine adhering strictly to **NASA Safety-Critical C Rules (JPL Rule 3: Zero post-boot dynamic heap allocations)**.

---

## 2. Directory Structure & Contents

```
btron/
├── dharma/                       # 4 Foundational Dharma Books (Binary TAD + Annotated Text)
│   ├── 01_btron3_spec.tad        # BTRON3 3.20 Specification Book (Binary TAD)
│   ├── 01_btron3_spec.tad.txt    # Human-readable symbolic TAD representation
│   ├── 02_tkernel_book.tad       # T-Kernel 2.0 Real-Time OS Book (Binary TAD)
│   ├── 02_tkernel_book.tad.txt   # Human-readable symbolic TAD representation
│   ├── 03_tron_hmi_book.tad      # TRON Human-Machine Interface Book (Binary TAD)
│   ├── 03_tron_hmi_book.tad.txt  # Human-readable symbolic TAD representation
│   ├── 04_bfree_os_book.tad      # B-Free Operating System Book (Binary TAD)
│   └── 04_bfree_os_book.tad.txt  # Human-readable symbolic TAD representation
│
├── tad_bin/                      # 41 Ukrainian BTRON3 Specification Documents (Compiled from ./doc/)
│   ├── index.tad                 # Main Documentation Portal
│   ├── shared_data/              # Part 1: Shared Data Specifications (10 documents)
│   │   ├── data_type.tad         # Basic data types, error codes, and C99 bounds
│   │   ├── tron_code.tad         # TRON multilingual character planes & encoding
│   │   ├── tad1.tad              # TAD document format & segment specification
│   │   ├── tad2.tad              # Text Fusen segments specification
│   │   ├── tad3.tad              # Figure & graphics Fusen segments
│   │   └── fd_format.tad         # BTRON FS Real/Virtual Object filesystem layout
│   ├── os_spec/                  # Part 2: Operating System Specifications (31 documents)
│   │   ├── kernel/               # μITRON Kernel, Tasks, Memory, I/O, IPC, Clock, Device
│   │   ├── dp/                   # Display Primitives (DP) 2D vector & text engine
│   │   ├── shell/                # GUI Shell, Window Manager, Menus, Panels, Parts, IME
│   │   └── indexfig.tad          # NASA JPL Rule 3 & Static Heap Analysis
│   └── *.tad.txt                 # Companion human-readable symbolic representations
```

---

## 3. Binary TAD Segment Structure (BTRON3 SPEC 3.20)

Every `.tad` file in `./dharma/` and `./tad_bin/` begins with the 16-bit **TAD Main Record Header** followed by structured Fusen segments:

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│  Record Header: Type = 0x0001 (Main Record) | 32-bit Payload Length (N bytes)   │
├──────────────────────────────────────────────────────────────────────────────────┤
│  TS_TPAGE   (0xFFA0) : Page Geometry (Width, Height, Top/Left/Right Margins)     │
├──────────────────────────────────────────────────────────────────────────────────┤
│  TS_TRULER  (0xFFA1) : Text Ruler (Line pitch: 18..24px, Left indent, Tabs)      │
├──────────────────────────────────────────────────────────────────────────────────┤
│  TS_TFONT   (0xFFA2) : Font Fusen (Font ID: Mincho/Gothic/Mono, TRON Plane: 0/1) │
├──────────────────────────────────────────────────────────────────────────────────┤
│  TS_TCHAR   (0xFFA3) : Character Style (Point Size: 10..22pt, Weight, RGB Color) │
├──────────────────────────────────────────────────────────────────────────────────┤
│  TS_VOBJ    (0xFFA8) : Embedded Virtual Object Link [仮身] (Target Real Object)  │
├──────────────────────────────────────────────────────────────────────────────────┤
│  TS_FPRIM   (0xFFB0) : Figure Primitive (Vector separator lines, Box containers) │
├──────────────────────────────────────────────────────────────────────────────────┤
│  Raw Text Payload    : Multi-byte TRON-Code / UTF-8 character byte stream        │
└──────────────────────────────────────────────────────────────────────────────────┘
```

### Segment Opcode Reference

| Segment Opcode | Hex Tag | Description | Payload Attributes |
|:---|:---:|:---|:---|
| `TS_TPAGE` | `0xFFA0` | Text Page Fusen | Page width, page height, margin top/bottom/left/right |
| `TS_TRULER` | `0xFFA1` | Text Ruler Fusen | Line pitch (px), paragraph indent, tab stops |
| `TS_TFONT` | `0xFFA2` | Text Font Fusen | Font typeface (0=System, 1=Mincho, 2=Gothic, 3=Mono), TRON Plane |
| `TS_TCHAR` | `0xFFA3` | Character Fusen | Font size (10..24pt), bold weight (400/700), RGB color |
| `TS_VOBJ` | `0xFFA8` | Virtual Object Link | Target Real Object ID, Link Name, Target Filepath |
| `TS_FPRIM` | `0xFFB0` | Figure Primitive | Primitive type (0=Horizontal Rule, 1=Rect), stroke, fill |

---

## 4. Generation Pipeline: Elixir Compiler (`scripts/html2tad.exs`)

All TAD files are compiled deterministically using the Elixir batch compiler tool:

```
                      ┌──────────────────────────────────────┐
                      │    Input HTML Sources (./doc/)       │
                      │    Dharma Book Definitions (AST)     │
                      └──────────────────┬───────────────────┘
                                         │
                                         ▼
                      ┌──────────────────────────────────────┐
                      │   Stage 1: Noise Filtering           │
                      │   - Strip navigation banners         │
                      │   - Remove breadcrumbs & scripts     │
                      │   - Decode HTML entities (&copy;...) │
                      └──────────────────┬───────────────────┘
                                         │
                                         ▼
                      ┌──────────────────────────────────────┐
                      │   Stage 2: Semantic DOM Tokenizer    │
                      │   - Headings (H1..H6) -> TS_TCHAR    │
                      │   - Code blocks (<pre>) -> TS_TFONT  │
                      │   - Anchors (<a href>) -> TS_VOBJ    │
                      │   - Rules (<hr>) -> TS_FPRIM         │
                      └──────────────────┬───────────────────┘
                                         │
                                         ▼
                      ┌──────────────────────────────────────┐
                      │   Stage 3: Binary & Text Generation  │
                      ├──────────────────┬───────────────────┤
                      ▼                                      ▼
        ┌─────────────────────────┐            ┌─────────────────────────┐
        │  Binary TAD (*.tad)     │            │  Symbolic TAD (*.tad.txt│
        │  SPEC 3.20 Byte Stream  │            │  Annotated Human-Readable
        └─────────────────────────┘            └─────────────────────────┘
```

### Build Commands

```bash
# Compile the 4 Dharma Books into ./dharma/*.tad:
make dharma

# Compile all 41 HTML specification documents into ./tad_bin/*.tad and run Elixir tests:
make html2tad

# Run the complete C-level unit test harness:
make test-tad
```

---

## 5. Runtime Usage in B-System

### 1. Cabinet Explorer (実身キャビネット - `src/apps/vobj_manager.c`)
- **Central Storage:** Manages the registry of all Real Objects (`dharma/` and `tad_bin/`).
- **Interactive Navigation:**
  - **Single Click / Key Arrow:** Selects an object, highlighting it in BTRON Navy Blue (`COLOR_NAVY`) and displaying metadata (Real Object ID, category, size in bytes) in the status bar.
  - **Double-Click / [開く (Open)] / [閲覧 (View)]:** Instantly opens the selected Real Object in the **TAD Document Browser**.
  - **Smooth Scrolling:** Scrollbar thumb and keyboard scrolling (<kbd>Page Up</kbd>, <kbd>Page Down</kbd>, <kbd>j</kbd>, <kbd>k</kbd>) for navigating large document libraries.

### 2. Native TAD Document Browser (`src/apps/tad_browser.c`)
- **Linear Stream Layout Engine:** Parses binary TAD segments into sequential layout spans (`TAD_SPAN`) with precomputed bounding boxes, font heights, and colors.
- **Hyper-Data Link Dispatch:** Embedded Virtual Objects (`[仮身]`) are rendered with interactive blue borders and hover indicators. Clicking a Virtual Object opens the referenced target document.
- **NASA JPL Safety-Critical Compliance:** Computes all layout coordinates in static bounded scope with zero runtime heap allocation.

---

## 6. Compatibility Matrix

| Feature | Legacy B-right/V Cho-Kanji (超漢字) | B-System Cleanroom OS | Status |
|:---|:---:|:---:|:---:|
| **Record Header (Type 1)** | `0x0001` (16-bit) | `0x0001` (16-bit) | **100% Binary Compatible** |
| **Virtual Object Fusen** | `TS_VOBJ` (`0xFFA8`) | `TS_VOBJ` (`0xFFA8`) | **100% Binary Compatible** |
| **Font & Character Fusen** | `TS_TFONT` / `TS_TCHAR` | `TS_TFONT` / `TS_TCHAR` | **100% Binary Compatible** |
| **Memory Allocation Model** | Unmanaged dynamic heap | Static bounded scope (NASA JPL Rule 3) | **Superior Safety & Predictability** |
| **Multilingual Character Code**| TRON Code Planes 0..3 | TRON Code + UTF-8 Dual Engine | **Modern Unicode Interop** |
| **Platform Target** | Bare-metal PC-AT (x86) | POSIX, QEMU ARM/AArch64, Pi 2B/3B/4B | **Universal Deployment** |
