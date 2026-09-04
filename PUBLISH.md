# B-System Documentation Publishing & Screenshot Pipeline Specification (PUBLISH.md)

This document specifies the automated, deterministic, and lightweight screenshot extraction and publishing pipeline for **B-System (BTRON 3.20)** documentation.

---

## 1. Design Philosophy & Requirements

Traditional OS documentation pipelines rely on heavyweight virtualization (QEMU, VirtualBox), display servers (X11, Wayland), or simulated mock DOMs. These approaches are slow, fragile, resource-heavy, and prone to visual divergence from the real C codebase.

The B-System documentation pipeline is designed to be **cheap, instant, authentic, and cleanly transparent**:
- **0 Virtual Machines / 0 Emulators**: Executes directly as native C99 test harness binaries.
- **0 External GUI Server Dependencies**: Requires no X11, Wayland, SDL, or browser automation.
- **100% Native C99 Rendering**: Calls actual window creation (`open_*_window()`) and graphics primitives (`redraw_all_windows()`, `dp_core.c`, `wnd.c`, `troncode.c`, `jis_fonts.c`, `tibetan_fonts.c`).
- **Alpha Transparency for Window Surrounds**: The headless framebuffer canvas initializes to `0x00000000` (Alpha = 0). Outer pixels around title tabs, corner chamfers, and bevels are fully transparent in the final PNGs.
- **Clear File Suffix Hierarchy**:
  - Settings applets use the `_Settings.png` suffix (e.g. `Appearance_Settings.png`).
  - Core applications use the `_Application.png` suffix (e.g. `TEditor_Application.png`).
  - System chrome overlays use descriptive single names (e.g. `GlobalMenu.png`).
- **Sub-Second Batch Execution**: Renders all 20+ system windows and encodes compressed PNGs in **<100 milliseconds** total.
- **Lightweight Lossless Storage**: Pure RGBA PNGs (zlib level 9 compression) averaging **6 KB to 28 KB** per window frame.

---

## 2. Pipeline Architecture

```
┌────────────────────────────────────────────────────────┐
│               Actual BTRON C99 Codebase                │
│  src/settings/*.c, src/apps/*.c, src/window/wnd.c      │
└──────────────────────────┬─────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────┐
│        Headless C99 Capturer (capture_screens.c)       │
│  - Clears canvas with transparent Alpha 0x00000000     │
│  - Invokes native window initialization routines       │
│  - Paints widgets, 3D borders, tabs, icons, and text   │
│  - Calculates geometric crop bounding boxes            │
│  - Dumps raw ARGB8888 binary slices to /tmp            │
└──────────────────────────┬─────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────┐
│        Zero-Dependency PNG Encoder (raw_to_png.py)     │
│  - Reads raw ARGB bytes & 8-byte geometry header       │
│  - Converts Little-Endian ARGB to standard RGBA8888    │
│  - Preserves Alpha channel for non-window background   │
│  - Compresses scanlines with Python stdlib zlib        │
│  - Emits valid IHDR, IDAT, IEND PNG binary chunks      │
└──────────────────────────┬─────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────┐
│     Optimized Asset Directory (b-system/img/screens/)  │
│  - <Name>_Settings.png, <Name>_Application.png         │
│  - Pixel-perfect, isolated & transparent background    │
└────────────────────────────────────────────────────────┘
```

---

## 3. Geometric Crop & Framing Formula

When a window is rendered to the headless canvas, the standard client bounds (`wnd->bounds`) do not include the BTRON3 title tab, 3D outer bevels, and drop shadow. The capture harness calculates the bounding box using the following formula:

$$\begin{aligned}
x_0 &= \max(0, \text{wnd}\to\text{bounds.left} - 4) \\
y_0 &= \max(0, \text{wnd}\to\text{bounds.top} - 30) \\
x_1 &= \min(\text{dev}\to\text{width}, \text{wnd}\to\text{bounds.right} + 6) \\
y_1 &= \min(\text{dev}\to\text{height}, \text{wnd}\to\text{bounds.bottom} + 6)
\end{aligned}$$

- **Top Offset ($-30\text{ px}$)**: Captures the BTRON3 sliding title tab, window caption, and close/zoom widgets.
- **Left Offset ($-4\text{ px}$)**: Captures the 3D raised light bevel.
- **Right/Bottom Offset ($+6\text{ px}$)**: Captures the 3D recessed shadow bevel and resize thumb grip.

---

## 4. Pixel Format & Color Fidelity

The headless canvas uses a 32-bit ARGB pixel format (`COLOR = uint32_t`, `0xAARRGGBB`).

| Component | Offset in Memory (LE) | Conversion to PNG Scanline |
| :--- | :--- | :--- |
| **Blue (B)** | Byte 0 | Byte 2 |
| **Green (G)** | Byte 1 | Byte 1 |
| **Red (R)** | Byte 2 | Byte 0 |
| **Alpha (A)** | Byte 3 | Preserved from Framebuffer (`0x00` outside window, `0xFF` window surface) |

The pipeline ensures authentic reproduction of:
- **SONY Dark Charcoal Palette** (`#2E3436`, `#1E2224`)
- **BTRON Classic Desktop Teal** (`#008080`)
- **3D Surface Bevels** (Highlight `#FFFFFF`, Shadow `#404040`)
- **Multi-Script Fonts** (TRONCode Kanji, Kana, Ascii, Tibetan dBu-can)
- **Transparent Desktop Cutouts** (Clean overlay on both dark and light documentation themes)

---

## 5. Screen Asset Inventory

Screenshots are generated into `b-system/img/screens/`:

### 5.1 Settings Applets (`_Settings.png`)
| Applet Name | Japanese Title | Generated Screenshot | Typical Size |
| :--- | :--- | :--- | :--- |
| **Appearance** | 外観 (Appearance) | `b-system/img/screens/Appearance_Settings.png` | ~10.6 KB |
| **Desktop** | デスクトップ (Desktop) | `b-system/img/screens/Desktop_Settings.png` | ~7.6 KB |
| **Display** | 画面表示 (Display) | `b-system/img/screens/Display_Settings.png` | ~7.3 KB |
| **Input** | 入力環境 (Input) | `b-system/img/screens/Input_Settings.png` | ~7.4 KB |
| **Language** | 言語・文字 (Language) | `b-system/img/screens/Language_Settings.png` | ~11.1 KB |
| **Media** | メディア (Media) | `b-system/img/screens/Media_Settings.png` | ~6.2 KB |
| **Network** | 通信網 (Network) | `b-system/img/screens/Network_Settings.png` | ~7.4 KB |
| **Preferences** | 環境設定 (Preferences) | `b-system/img/screens/Preferences_Settings.png` | ~28.8 KB |
| **Security** | 保全・権限 (Security) | `b-system/img/screens/Security_Settings.png` | ~6.3 KB |
| **Sound** | 音響・音声 (Sound) | `b-system/img/screens/Sound_Settings.png` | ~7.2 KB |
| **System** | 基本情報 (System) | `b-system/img/screens/System_Settings.png` | ~6.4 KB |
| **Terminal** | 端末・通信設定 (Terminal) | `b-system/img/screens/Terminal_Settings.png` | ~10.7 KB |

### 5.2 Core Applications (`_Application.png`)
| Application Name | Japanese Title | Generated Screenshot | Typical Size |
| :--- | :--- | :--- | :--- |
| **Cabinet** | 実身キャビネット (Cabinet) | `b-system/img/screens/Cabinet_Application.png` | ~8.6 KB |
| **TEditor** | 基本文章編集 (TEditor) | `b-system/img/screens/TEditor_Application.png` | ~17.8 KB |
| **Browser** | 仕様書閲覧 (Browser) | `b-system/img/screens/Browser_Application.png` | ~5.7 KB |
| **Cassette** | カセット (Cassette) | `b-system/img/screens/Cassette_Application.png` | ~10.1 KB |
| **Chat** | 対話通信 (Chat) | `b-system/img/screens/Chat_Application.png` | ~4.1 KB |
| **About** | システム情報 (About) | `b-system/img/screens/About_Application.png` | ~11.6 KB |

### 5.3 System Overlays
| Component | Japanese Title | Generated Screenshot | Typical Size |
| :--- | :--- | :--- | :--- |
| **GlobalMenu** | システムメニューバー | `b-system/img/screens/GlobalMenu.png` | ~1.9 KB |

---

## 6. HTML & TAD Embedding Standard

All documentation pages link to their corresponding isolated window screenshot using standard semantic markup:

```html
<div class="section">
    <h2>画面プレビュー (Screen Preview)</h2>
    <div class="screen-preview">
        <img src="../img/screens/Appearance_Settings.png" alt="外観設定 ウィンドウプレビュー (Appearance Settings Window)" width="650" />
        <p class="caption">実機描画フレームバッファより自動抽出された透過外観設定ウィンドウ (Native Headless Transparent GDEV Capture)</p>
    </div>
</div>
```

---

## 7. Makefile Commands

```bash
# 1. Generate all window screenshots with alpha-transparent backgrounds and suffixes
make screenshots

# 2. Recompile all HTML documentation pages to TAD binary format
elixir scripts/html2tad.exs

# 3. Run entire test suite (including GUI, kernel, and app tests)
make test
```
