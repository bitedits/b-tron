#!/usr/bin/env python3
"""
scripts/populate_doc_screens.py
Automated injection of native C99 transparent screenshot previews into B-System HTML documentation pages.
For Apps: embeds single screenshot with opened menu and active content.
Plain image tags without CSS effects.
"""

import os
import re

SETTINGS_DIR = "b-system/settings"
APPS_DIR = "b-system/apps"

SETTINGS_MAP = {
    "Appearance.html": {
        "screen": "../img/screens/Appearance_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過外観設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "外観設定 (Appearance Settings)"
    },
    "Desktop.html": {
        "screen": "../img/screens/Desktop_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過デスクトップ設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "デスクトップ設定 (Desktop Settings)"
    },
    "Display.html": {
        "screen": "../img/screens/Display_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過画面表示設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "画面表示設定 (Display Settings)"
    },
    "Input.html": {
        "screen": "../img/screens/Input_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過入力環境設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "入力環境設定 (Input Settings)"
    },
    "Language.html": {
        "screen": "../img/screens/Language_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過言語・文字設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "言語設定 (Language Settings)"
    },
    "Media.html": {
        "screen": "../img/screens/Media_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過メディア設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "メディア設定 (Media Settings)"
    },
    "Network.html": {
        "screen": "../img/screens/Network_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過通信網設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "通信網設定 (Network Settings)"
    },
    "Security.html": {
        "screen": "../img/screens/Security_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過保全・権限設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "保全設定 (Security Settings)"
    },
    "Sound.html": {
        "screen": "../img/screens/Sound_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過音響・音声設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "音響設定 (Sound Settings)"
    },
    "System.html": {
        "screen": "../img/screens/System_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過基本情報設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "基本情報設定 (System Settings)"
    },
    "Terminal.html": {
        "screen": "../img/screens/Terminal_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された透過端末環境設定ウィンドウ (Native Headless GDEV Capture)",
        "alt": "端末設定 (Terminal Settings)"
    }
}

APPS_MAP = {
    "Cabinet.html": {
        "screen": "../img/screens/Cabinet_Menu_Opened.png",
        "caption": "実機描画フレームバッファより自動抽出された実身キャビネット・エクスプローラ (ファイルメニュー展開・実身一覧表示)",
        "alt": "実身キャビネット (Cabinet Application - Menu Opened)"
    },
    "TEditor.html": {
        "screen": "../img/screens/TEditor_Menu_Opened.png",
        "caption": "実機描画フレームバッファより自動抽出された基本文章編集 T-Editor (ファイルメニュー展開・本文テキスト表示)",
        "alt": "基本文章編集 (TEditor Application - Menu Opened)"
    },
    "Browser.html": {
        "screen": "../img/screens/Browser_Menu_Opened.png",
        "caption": "実機描画フレームバッファより自動抽出された仕様書閲覧 TAD Browser (ファイルメニュー展開・仕様書TAD文書表示)",
        "alt": "仕様書閲覧 (Browser Application - Menu Opened)"
    },
    "Terminal.html": {
        "screen": "../img/screens/Terminal_Menu_Opened.png",
        "caption": "実機描画フレームバッファより自動抽出されたBTRON端末エミュレータ GTerm (ファイルメニュー展開・シェル対話表示)",
        "alt": "端末 (Terminal Application - Menu Opened)"
    },
    "Cassette.html": {
        "screen": "../img/screens/Cassette_Application.png",
        "caption": "実機描画フレームバッファより自動抽出されたステレオカセットデッキ Cassette (Native Headless GDEV Capture)",
        "alt": "カセット (Cassette Application)"
    },
    "Chat.html": {
        "screen": "../img/screens/Chat_Application.png",
        "caption": "実機描画フレームバッファより自動抽出された対話通信 Blabber Chat ロスター (Native Headless GDEV Capture)",
        "alt": "対話通信 (Chat Application)"
    },
    "Preferences.html": {
        "screen": "../img/screens/Preferences_Settings.png",
        "caption": "実機描画フレームバッファより自動抽出された環境設定コントロールパネルハブ (Native Headless GDEV Capture)",
        "alt": "環境設定 (Preferences Application)"
    }
}

def inject_screen_section(filepath, info):
    if not os.path.exists(filepath):
        return

    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # Remove existing screen preview section if present
    content = re.sub(r'<!-- 画面プレビュー -->\s*<div class="section">\s*<h2>画面プレビュー \(Screen Preview\)</h2>.*?</div>\s*</div>\s*', '', content, flags=re.DOTALL)

    img_block = f"""    <!-- 画面プレビュー -->
    <div class="section">
      <h2>画面プレビュー (Screen Preview)</h2>
      <div class="screen-preview">
        <img src="{info['screen']}" alt="{info['alt']}">
        <p>{info['caption']}</p>
      </div>
    </div>\n\n"""

    # Inject right before <!-- 概要 --> or <h2>概要
    if "<!-- 概要 -->" in content:
        content = content.replace("<!-- 概要 -->", img_block + "    <!-- 概要 -->", 1)
    elif "<h2>概要" in content:
        idx = content.find("<h2>概要")
        sec_idx = content.rfind('<div class="section">', 0, idx)
        if sec_idx != -1:
            content = content[:sec_idx] + img_block + content[sec_idx:]
        else:
            content = content.replace("<h2>概要", img_block + "<h2>概要", 1)
    else:
        idx = content.find('class="tagline-card"')
        if idx != -1:
            end_card = content.find('</div>', idx) + 6
            content = content[:end_card] + "\n\n" + img_block + content[end_card:]

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"Populated single menu+content screenshot in: {filepath}")

def main():
    print("==========================================================")
    print(" Populating Single Opened-Menu+Content Screenshots for Apps...")
    print("==========================================================")
    for filename, info in SETTINGS_MAP.items():
        inject_screen_section(os.path.join(SETTINGS_DIR, filename), info)

    for filename, info in APPS_MAP.items():
        inject_screen_section(os.path.join(APPS_DIR, filename), info)

    print("==========================================================")
    print(" Finished Populating Documentation Screenshots!")
    print("==========================================================")

if __name__ == "__main__":
    main()
