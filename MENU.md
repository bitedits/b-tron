# B-System (BTRON 3.20) Window Menu Architecture & Specification (MENU.md)

This document specifies the architecture, behavioral rules, and implementation details for the BTRON 3.20 Window Menu System and its modern BeOS/Haiku-style Menu Bar integration.

---

## 1. Architectural Foundation & Spec Conformance

### 1.1 BTRON3 SPEC 3.20 Menu Manager Conformance
According to BTRON3 Specification 3.20 (Chapter 7.2 *Menu Manager*), the system defines two complementary menu mechanisms:
1. **Standard Menu (Стандартне меню / Menu Bar & Pull-down Submenus)**:
   - Displayed in a horizontal row at the top of the window client area.
   - Registered and managed through data structures (`MENUITEM`, `MENUDISP`) and system calls (`mcre_men`, `mopn_men`, `mpop_men`, `mact_men`).
   - Items support activation states (`inact` bitmask), selection indicators (`select` bitmask), and keyboard macros (`MC_KEY1`..`MC_KEY15`).
2. **Generic Menu (GMENU / Pop-up Context Menu)**:
   - Free-floating popup menu positioned anywhere relative to cursor coordinates.
   - Supports custom rectangular layout zones (`RECT r[32]`).

### 1.2 Elimination of the Modal "Open Dialog" Window
- **Historical & Philosophical Context**:
  Traditional desktop systems (Macintosh, MS Windows, Unix CUA/X11) are **application-centric**. Launching an application forces the user to navigate a hierarchical directory tree (`/path/to/folder`) through a modal "File Open" dialog box.
  
  In Ken Sakamura's TRON Architecture, the operating system is **document-centric**:
  - The filing system is modeled as a hypermedia network of **Real Objects (実身)** and **Virtual Objects (仮身)**.
  - Documents are opened directly from Cabinets or embedded Virtual Object fusen links without a file picker dialog.
  
- **Cleanroom Implementation Rule**:
  T-Editor and BTRON applications do not use modal file-opening dialogs. Instead:
  - The `ファイル(F)` (File) menu contains an **`開く (Open) ▶` cascading dropdown menu**.
  - The menu dynamically scans available document repositories (e.g., `assets/texts/` filtering `.txt` files).
  - The user accesses any document in **1–2 fluid clicks** with zero modal interruption, zero path typing, and zero file picker friction.

---

## 2. Menu Bar Structure & Layout

### 2.1 Streamlined Vertical Window Geometry
Following the elimination of the redundant legacy CUA toolbar row and the consolidation of the TIP indicator to the status bar footer, the Menu Bar is purely dedicated to application commands and document status:
```
┌─────────────────────────────────────────────────────────────┐
│ [Title Bar] Window Title                              [X]   │
├─────────────────────────────────────────────────────────────┤ y = 0
│ ファイル(F) 編集(E) 表示(V) 仮身(O) ヘルプ(H)           📄 Doc* │ Menu Bar (y = 0..21)
├─────────────────────────────────────────────────────────────┤ y = 22
│ 1| Document content canvas begins directly here...          │
│ 2|                                                          │ Editor Canvas (y = 24..)
│  |                                                          │
├─────────────────────────────────────────────────────────────┤
│ 📄 BTRON3_Report.txt                UTF-8 | Rows: 42 | [あ] │ Status Bar Footer
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Standard Top-Level Menus & Non-Overfull Margins
All top-level headers are sized to strictly eliminate text overfulls, accounting for 16px wide JIS/Kanji glyphs and 8px wide ASCII glyphs with a guaranteed 8px horizontal padding on each side:

| Menu Title | Text Width | Button Rect | Internal Margins | Items & Shortcuts |
| :--- | :--- | :--- | :--- | :--- |
| **ファイル(F)** | 88px | `[4, 0, 108, 21]` (104px) | 8px left / 8px right | `新規作成 Ctrl+N`, `開く ▶ Ctrl+O`, `---`, `上書き保存 Ctrl+S`, `閉じる Ctrl+W` |
| **編集(E)** | 56px | `[112, 0, 184, 21]` (72px) | 8px left / 8px right | `元に戻す Ctrl+Z`, `---`, `切り取り Ctrl+X`, `コピー Ctrl+C`, `貼り付け Ctrl+V`, `すべて選択 Ctrl+A` |
| **表示(V)** | 56px | `[188, 0, 260, 21]` (72px) | 8px left / 8px right | `拡大 +`, `縮小 -`, `---`, `行番号表示` |
| **仮身(O)** | 56px | `[264, 0, 336, 21]` (72px) | 8px left / 8px right | `仮身を挿入`, `実身キャビネット` |
| **ヘルプ(H)** | 72px | `[340, 0, 428, 21]` (88px) | 8px left / 8px right | `T-Editor について (About)` (Opens 280x135 nano About Box with 5HT attribution) |
| **Document Status** | Dynamic | `[width - title_w - 14, 3]` | 8px left / 14px right | Dynamic width right-aligned filename status |

### 2.3 Explicit Item Division: 3D Etched Distance Shadow / Separator
To maximize CPU rendering efficiency while delivering a distinct visual separation between top-level menu items:
- **Etched Dual-Line Groove**: Each horizontal item boundary is demarcated by a 2-pixel vertical etched groove:
  - Left column (`COLOR_DKGRAY`): 1px shadow line from `y = 3` to `y = 19`.
  - Right column (`COLOR_WHITE`): 1px highlight line directly adjacent (`x + 1`) from `y = 3` to `y = 19`.
- **CPU Efficiency**:
  - Implemented via contiguous 1px vertical pixel writes (`fill_rec`).
  - Exactly 16 dword writes per line (32 writes total per separator).
  - Sub-microsecond execution time with zero overdraw, zero blending, and zero matrix transformations.
- **Placement**:
  - Between `ファイル(F)` and `編集(E)` (`x = 110`)
  - Between `編集(E)` and `表示(V)` (`x = 186`)
  - Between `表示(V)` and `仮身(O)` (`x = 262`)
  - Between `仮身(O)` and `ヘルプ(H)` (`x = 338`)
  - Before the Document Status Title (`x = width - title_w - 22`)

---

## 3. BeOS/Haiku & Modern Menu Bar Behavioral Rules

### 3.1 Hover Selection Architecture
Hover selection is supported across all layers of the window and desktop event hierarchy:

1. **Desktop Event Forwarding (`src/desktop/main.c`)**:
   - The desktop event loop dispatches `EV_MOUSE_MOVE` events directly to the active top window (`top->event_handler(top, &ev)`), allowing in-window controls to track cursor movements continuously without waiting for clicks.

2. **Closed State Header Hover**:
   - When no menu is currently dropped down (`active_menu == -1`), hovering over any top-level header (`ファイル(F)`, `編集(E)`, etc.) highlights the header with a crisp white bevel border and navy text (`COLOR_NAVY`), visually signaling interactivity.
   - Moving the pointer off the Menu Bar cleanly restores standard inactive presentation.

3. **Active Menu Tracking & Hot Header Switching**:
   - Once a menu is clicked open, moving the pointer across to another top-level menu header immediately closes the current menu and opens the target menu without requiring any further clicks.

4. **Dropdown & Cascading Item Hover Selection**:
   - Hovering over valid items highlights them with `COLOR_NAVY` fill and `COLOR_WHITE` text.
   - Hovering over items with child submenus (`開く (Open) ▶`) **automatically expands the cascading submenu on hover**, eliminating redundant clicks.
   - Submenu items similarly highlight on hover and execute on a single click.

5. **Execution & Dismissal**:
   - Clicking an enabled item executes its command and dismisses all menus.
   - Clicking outside any active menu closes the menu system and returns focus to the editor canvas.
   - Pressing `Escape` cancels and dismisses active menus cleanly.

---

## 4. Dynamic Asset Filesystem Discovery (`assets/texts/*.txt`)

### 4.1 Asset Scanner Architecture
The `開く (Open) ▶` cascading menu dynamically scans the filesystem to populate available document items:
- Target Directory: `assets/texts/` (with fallback to `assets/`).
- File Filter: Only files ending in `.txt` (case-insensitive).
- Invariant & Safety:
  - NASA JPL Rule 3 compliant (bounded buffer array `MAX_MENU_FILES 32`, zero post-boot heap allocations).
  - Deterministic fallback list if running on baremetal/unhosted environments without POSIX `dirent`.
- Item Representation:
  - Displays icon and filename: `📄 <filename>`.
  - Selecting an item invokes `teditor_load_file()` directly.

---

## 5. Keyboard Accelerator Specifications

| Action | Accelerator Key | Behavior |
| :--- | :--- | :--- |
| **New Document** | `Ctrl + N` | Clears document buffer, resets to untitled state |
| **Open Menu** | `Ctrl + O` | Directly drops down the `開く (Open) ▶` document menu |
| **Save Document** | `Ctrl + S` | Saves document to current filename |
| **Close Window** | `Ctrl + W` | Closes active editor window |
| **Cut** | `Ctrl + X` | Deletes selected text and copies to system clipboard |
| **Copy** | `Ctrl + C` | Copies selected text to system clipboard |
| **Paste** | `Ctrl + V` | Inserts system clipboard text at cursor position |
| **Select All** | `Ctrl + A` | Selects entire document buffer |
| **Zoom In** | `+` (or `=`) | Increases window size / visible text lines |
| **Zoom Out** | `-` | Decreases window size / visible text lines |
| **Toggle IME** | `F10` | Switches TRON IME mode (ASCII → Hiragana → Katakana → Tibetan) |

---

## 6. Visual Design & Theme Tokens

- **Menu Bar Background**: `COLOR_LTGRAY` (`0xFFD0D0D0`) with bottom separator line `0xFF808080`.
- **Menu Panel Fill**: `COLOR_WHITE` (`0xFFFFFFFF`) or `0xFFF6F6F6`.
- **Bevel Border**: 2px 3D border (`COLOR_WHITE` top/left, `COLOR_DKGRAY` bottom/right) conforming to BTRON / BeOS styling.
- **Hover Selection**: `COLOR_NAVY` (`0xFF003366`) with `COLOR_WHITE` text.
- **Separator Lines**: Inset etched horizontal line (`drw_lin` with `COLOR_GRAY` and `COLOR_WHITE` 1px offset).
- **Submenu Arrow**: `▶` (Unicode `U+25B6` / TRON Code) placed at right margin.
