B-System IME
============

Requirement Specification: Character Encoding and Japanese Input Method for B-TRON
Document purpose: This specification defines the mandatory requirements for character
encoding and Japanese text input in a cleanroom B-TRON implementation. It ensures
historical fidelity to the TRON Code model while achieving practical interoperability
with modern systems.

TOC:

* [1]. Character Encoding Model
* [2]. Japanese Input Method (IME)
* [3]. Application Integration
* [4]. Compatibility and Extensibility
* [5]. Summary of Design Intent
* [6]. Wireframe Mockups – B-TRON Japanese IME Windows

## 1. Character Encoding Model

### REQ-1.1 Hybrid Representation

The system shall use a hybrid character model:

* Internal native representation: TRON Code (multi-plane), extended with a Unicode mapping plane.
* External interchange format: UTF-8.

### REQ-1.2 Conversion Functions

The system shall provide bidirectional, lossless conversion for the common Japanese character repertoire:

* UTF-8 → TRON Code
* TRON Code → UTF-8

Conversion shall correctly handle ASCII, Hiragana, Katakana, JIS X 0208, and frequently used Unicode CJK characters.
Unsupported or rare characters shall map to a defined fallback or private plane without data loss where possible.

### REQ-1.3 Storage Rules

* TAD documents and internal text objects shall prefer TRON Code.
* Plain-text files, clipboard data, and host-OS interchange shall use UTF-8 by default.
* Full-width / half-width distinction shall be handled via TAD formatting attributes (fusen), not by distinct code points.

### REQ-1.4 Glyph Rendering

* The font engine shall support:Classic bitmap glyphs (8×16 / 16×16) for retro compatibility.
* Modern scalable fonts (via FreeType or equivalent) for Unicode coverage.
* Optional loading of extended Japanese administrative character sets.

## 2. Japanese Input Method (IME)

### REQ-2.1 Architecture

The system shall implement a separated IME architecture consistent with original B-TRON design:

* Input front-end (composition UI and candidate display)
* Conversion engine (Romaji/Kana → Kanji)

### REQ-2.2 Supported Input Modes

The IME shall support at minimum:

* Romaji input (primary mode for modern users)
* Direct Kana input

### REQ-2.3 Conversion Engine

The conversion engine shall provide accurate kana-kanji conversion.
Integration of Mozc (or an equivalent high-quality open engine) is the preferred implementation path.
A minimal dictionary-based engine is acceptable as an interim solution.

### REQ-2.4 Composition Handling

The system shall:

* Maintain an active composition state.
* Display composition feedback (underline or equivalent B-TRON visual style).
* Present conversion candidates in a dedicated window or panel.
* Support standard confirmation, cancellation, and candidate selection actions.
* Integrate cleanly with the existing B-TRON event queue.

### REQ-2.5 User Dictionary

The system shall support a persistent user dictionary stored as a Real Object.

## 3. Application Integration

### REQ-3.1 Text Components

The text editor (t_editor) and terminal (gterm) shall accept both UTF-8 and TRON Code input streams
and render them correctly.

### REQ-3.2 Event Flow

Keyboard events shall be routed through the IME front-end when Japanese input mode is active.
Confirmed text shall be inserted into the target application as TRON Code or UTF-8 according to
the storage rules in REQ-1.3.

## 4. Compatibility and Extensibility

### REQ-4.1 Round-trip Safety

Common Japanese text shall survive UTF-8 <-> TRON Code conversion without corruption.

### REQ-4.2 Extensibility

The design shall allow future addition of:

* Ideographic Variation Sequences (IVS)
* Additional TRON Code planes for rare/historical characters
* TRON keyboard layout support

### REQ-4.3 Host Integration

When running under a host operating system, the implementation may initially
leverage the host IME and convert the resulting UTF-8 text into the internal
representation. A native B-TRON IME remains the target architecture.

## 5. Summary of Design Intent

The requirements preserve the original B-TRON character-set philosophy (multi-plane, non-unified encoding)
while mandating UTF-8 as the practical external standard and a modern, usable Japanese IME.
This enables both authentic retro behaviour and contemporary usability for Japanese text processing.

## 6. Wireframe Mockups – B-TRON Japanese IME Windows

Here are clean wireframe concepts for the IME UI, designed in the classic B-TRON / Sakamura style (double-bordered windows,
simple title bars, minimal chrome, clear hierarchy).

### 6.1. Composition Window (Inline / Floating)

```
┌══════════════════════════════════════════════════════┐
║  Composition                                      [×]║
╠══════════════════════════════════════════════════════╣
║                                                      ║
║   わたしのなまえは  中野です                         ║
║   ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾                          ║
║   (reading / converted clauses with underline)       ║
║                                                      ║
║   Mode: あ  Romaji → Kana → Kanji                    ║
╚══════════════════════════════════════════════════════╝
```

Notes:

* Double outer border (classic B-TRON).
* Underline (or dotted line) under the active composition string.
* Optional small mode indicator (あ / A / ア).
* Can appear as a floating window or as an overlay directly above the caret in the text editor.

### 6.2. Candidate Window (Selection List)

```
┌══════════════════════════════┐
║  Candidates               [×]║
╠══════════════════════════════╣
║ 1  中野                      ║
║ 2  仲野                      ║
║ 3  なかの                    ║
║ 4  ナカノ                    ║
║ 5  中埜                      ║
║──────────────────────────────║
║ ↑↓ Select   Enter Confirm    ║
║ Space Next page              ║
╚══════════════════════════════╝
```

Variant with annotations (recommended):

```
┌══════════════════════════════════┐
║  Candidates                   [×]║
╠══════════════════════════════════╣
║ 1  中野     (surname)            ║
║ 2  仲野     (surname)            ║
║ 3  なかの   (hiragana)           ║
║ 4  ナカノ   (katakana)           ║
║ 5  中埜     (rare)               ║
║──────────────────────────────────║
║ 1-9 Select   Esc Cancel          ║
╚══════════════════════════════════╝
```

### 6.3. Combined View (Typical Usage)

Text Editor Window:

```
┌══════════════════════════════════════════════════════┐
║  t_editor – Untitled                              [×]║
╠══════════════════════════════════════════════════════╣
║                                                      ║
║  今日は  [わたしのなまえは  中野です]  です。        ║
║             ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾                ║
║                                                      ║
└══════════════════════════════════════════════════════┘
                          │
                          ▼
              ┌══════════════════════════════┐
              ║  Candidates               [×]║
              ╠══════════════════════════════╣
              ║ 1  中野                      ║
              ║ 2  仲野                      ║
              ║ 3  なかの                    ║
              ║ 4  ナカノ                    ║
              ║ 5  中埜                      ║
              ╚══════════════════════════════╝
```

7. Design Rules Applied

* Strict double-line outer borders
* Single-line inner separators
* Minimal title bar with close button only
* Clear numeric selection (1–9) + keyboard hints at the bottom
* Compact vertical candidate list (classic Japanese IME style)
* Composition string always shows reading + current conversion state with underline

These wireframes can be implemented directly with the existing B-TRON window manager (wnd.h) primitives.
We need a more detailed pixel-level version or additional states (e.g. prediction list, error state, or vertical candidate layout).


