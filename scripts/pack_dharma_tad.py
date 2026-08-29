#!/usr/bin/env python3
"""
scripts/pack_dharma_tad.py
Packs 4 foundational TRON / B-System books (dharma) into 4 canonical TAD documents:
1. dharma/01_btron3_spec.tad   - BTRON3 Specification 3.20 (Parts 1 & 2 across 8 chapters)
2. dharma/02_tkernel_book.tad  - T-Kernel 2.0 Real-Time OS Specification & Deployment Guide
3. dharma/03_tron_hmi_book.tad - TRON Human-Machine Interface (HMI) Standard & Parts Catalog
4. dharma/04_bfree_os_book.tad - B-Free OS Architectural Manifesto & POSIX/uITRON Specification
"""

import os
import sys
import datetime

DHARMA_DIR = "dharma"

def build_btron3_spec_tad():
    return """\
================================================================================
TAD REAL OBJECT [実身 #101] : BTRON3 Specification Ver 3.20.00
RECORD TYPE : 1 (TAD Main Record) | APP : BTRON Text/TAD Editor (t_editor)
AUTHOR      : Ken Sakamura / TRON Association (Cleanroom Edition)
DATE        : 2026-08-29 | ENCODING : TRON Multilingual Code / TAD Standard
================================================================================

[付箋: DOCUMENT_HEADER | Title="BTRON3 SPEC 3.20" | Font="Cho-Kanji-Mincho" | Size=16]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            BTRON3 仕様書 バージョン 3.20.00 (Sakamura BTRON Architecture)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[仮身] #110 : Part 1: Shared Data Specifications (共通データ構造仕様)
[仮身] #120 : Part 2: Operating System Specification (OS機能仕様)
[仮身] #130 : Static Analysis & Bounded Heap Memory Model (MISRA / NASA JPL)

───────────────────────────────────────────────────────────────────────────────
目次 (TABLE OF CONTENTS)
───────────────────────────────────────────────────────────────────────────────
第1部：共通データ構造仕様 (PART 1: SHARED DATA SPECIFICATIONS)
  [仮身] #111 : 第1章 基本データ型とエラーコード (Data Types & Error Codes)
  [仮身] #112 : 第2章 TRONコード文字体系仕様 (TRON Multilingual Character Code)
  [仮身] #113 : 第3章 TAD (TRON Application Databus) 文書フォーマット仕様
  [仮身] #114 : 第4章 BTRON FS 実身／仮身ファイルシステム構造仕様

第2部：オペレーティングシステム機能仕様 (PART 2: OS SPECIFICATION)
  [仮身] #121 : 第5章 μITRON リアルタイムカーネルとタスク管理 (Kernel & Tasking)
  [仮身] #122 : 第6章 表示プリミティブ DP (Display Primitives Graphics Engine)
  [仮身] #123 : 第7章 GUIシェルとウィンドウマネージャ (Window Manager & Shell)
  [仮身] #124 : 第8章 静的解析と決定論的メモリ制約 (Static Scope & Bounded Memory)

================================================================================
第1部：共通データ構造仕様 (PART 1: SHARED DATA SPECIFICATIONS)
================================================================================

--------------------------------------------------------------------------------
【第1章】 基本データ型とエラーコード (Data Types & Error Codes)
--------------------------------------------------------------------------------
BTRON仕様で規定される標準データ型および最適化抑止型（Volatile）定義：

  typedef char            B;   /* 符号付き 8ビット整数 (Signed 8-bit Integer)   */
  typedef short           H;   /* 符号付き 16ビット整数 (Signed 16-bit Integer)  */
  typedef int             W;   /* 符号付き 32ビット整数 (Signed 32-bit Integer)  */
  typedef unsigned char   UB;  /* 符号なし 8ビット整数 (Unsigned 8-bit Integer) */
  typedef unsigned short  UH;  /* 符号なし 16ビット整数 (Unsigned 16-bit Integer)*/
  typedef unsigned int    UW;  /* 符号なし 32ビット整数 (Unsigned 32-bit Integer)*/
  typedef void           *VP;  /* 汎用データポインタ (Generic Pointer)          */
  typedef void          (*FP)();/* 汎用関数ポインタ (Function Pointer)          */

  /* 最適化抑止型 (Volatile Types - ハードウェアレジスタ／リアルタイム同期用) */
  typedef volatile W     _W;
  typedef volatile H     _H;
  typedef volatile B     _B;
  typedef volatile UW    _UW;
  typedef volatile UH    _UH;
  typedef volatile UB    _UB;

システムエラーコード体系：
  E_OK    (0)  : 正常完了 (Normal Completion)
  E_SYS  (-5)  : システムエラー (Internal System Error)
  E_NOMEM(-10) : メモリ不足 (Insufficient Memory)
  E_LIMIT(-34) : リソース上限超過 (Exceeded Resource Limits)
  E_PAR  (-17) : パラメータ不正 (Invalid Parameter)

--------------------------------------------------------------------------------
【第2章】 TRONコード文字体系仕様 (TRON Multilingual Character Code)
--------------------------------------------------------------------------------
TRON多言語文字体系は、文字集合の統合（Unicode Han Unification）による字体消失を
排除し、150万文字以上を完全に識別・共存させる多面構造（Multi-plane Space）を採用：

  空間構造式： T_Space = Plane × Row × Column
  ・第1面  (P1)  : JIS X 0208 基本漢字・ひらがな・カタカナ・記号
  ・第2面  (P2)  : JIS X 0212 補助漢字・記号
  ・第3〜5面(P3-5): 大漢和辞典 収録漢字 (50,000字以上)
  ・第6〜9面(P6-9): GB 2312 (簡体字), Big5 (繁体字), KS C 5601 (ハングル)
  ・第255面(P255): Unicode外部相互運用マッピング面 (UTF-8 Bridge)

エスケープシーケンス：
  [0x00 - 0x7F]    : 7ビット標準ASCII
  [0xFE, Plane_ID] : 2バイト面切替シフト
  [0xFF, Plane_ID] : 4バイト完全指定ユニバーサルコード

--------------------------------------------------------------------------------
【第3章】 TAD (TRON Application Databus) 文書フォーマット仕様
--------------------------------------------------------------------------------
TADは「データとアプリケーションの完全な融合」を実現するストリーム構造です。
各セグメントは 16ビットの識別タグ(T_tag) と 長さ(L_len) で構成されます：

  TAD Segment = <T_tag, L_len, Data_Payload>

代表的なレコード種別 (Record Types)：
  ・Type 0 : リンクレコード (Link Record)
  ・Type 1 : TADメインレコード (Main Record - 本文・図形・仮身)
  ・Type 2 : 注釈レコード (Annotation Record - ヘルプ・著作権・メタデータ)
  ・Type 5 : 設定付箋レコード (Setting Fusen - 書体・文字サイズ・ルーラー)
  ・Type 9 : 実行可能プログラム (Executable Binary Record)
  ・Type 12: 辞書データ (Mozc / User Dictionary Real Object)

--------------------------------------------------------------------------------
【第4章】 BTRON FS 実身／仮身ファイルシステム構造仕様
--------------------------------------------------------------------------------
階層型ディレクトリツリー（UNIX / DOS型）を廃止し、ネットワーク型ハイパーデータ
グラフを採用：
  ・実身 (Real Object) : データ本体が格納されるキャビネット内の実体ファイル。
  ・仮身 (Virtual Object): 実身へのポインタを含むドキュメント内埋め込みオブジェクト。
  ・1つの実身に対して複数の仮身を異なる文書からリンク可能（多重参照）。
  ・循環参照検出と有向グラフ探索による高速なインデックス管理。

================================================================================
第2部：オペレーティングシステム機能仕様 (PART 2: OS SPECIFICATION)
================================================================================

--------------------------------------------------------------------------------
【第5章】 μITRON リアルタイムカーネルとタスク管理 (Kernel & Tasking)
--------------------------------------------------------------------------------
確定的な最悪実行時間（WCET）を保証する優先度ベースのプリエンプティブ・スケジューラ：
  ・タスク管理システムコール : cre_tsk(), sta_tsk(), ext_tsk(), ter_tsk(), chg_pri()
  ・同期・通信プリミティブ : セマフォ(wai_sem/sig_sem), イベントフラグ(wai_flg),
                            メールボックス(snd_mbx/rcv_mbx), ランデブーポート
  ・時間管理 : dly_tsk(), set_tim(), get_tim()

--------------------------------------------------------------------------------
【第6章】 表示プリミティブ DP (Display Primitives Graphics Engine)
--------------------------------------------------------------------------------
Sakamura 2Dベクトル／ラスタ統合描画エンジン：
  ・GDEV (グラフィックス描画コンテキスト)
  ・PAT (パターン・カラー定義: ソリッド, ハッチング, タイルビットマップ)
  ・RGN (リージョン・矩形クリッピング領域計算)
  ・描画API: gd_rect(), gd_line(), gd_ellipse(), gd_draw_text(), gd_bitblt()

--------------------------------------------------------------------------------
【第7章】 GUIシェルとウィンドウマネージャ (Window Manager & Shell)
--------------------------------------------------------------------------------
二重枠（Double Border）を持つ坂村健デザインのオーセンティック・ウィンドウシステム：
  ・ウィンドウライフサイクル: wnd_create(), wnd_show(), wnd_hide(), wnd_destroy()
  ・Z-orderウィンドウスタック管理とダーティ矩形再描画
  ・メニューバー、システムステータスパネル、キャビネットランチャー
  ・TIP (テキスト入力プリミティブ) によるインライン変換候補ウィンドウのポップアップ

--------------------------------------------------------------------------------
【第8章】 静的解析と決定論的メモリ制約 (Static Scope & Bounded Memory)
--------------------------------------------------------------------------------
MISRA-C:2012 / NASA JPL 航空宇宙セーフティクリティカル C 開発規約への完全適合：
  ・ブート完了後の動的ヒープ確保 (malloc/free) を完全禁止。
  ・配列サイズ・ループ反復回数は全て静的定数で有界化 (MAX_CTRLS = 32 等)。
  ・すべてのAPIで入力ポインタのNULL検証と境界値アサーションを義務付け。
  ・純粋整数演算による確定的イベントディスパッチ。
"""

def build_tkernel_book_tad():
    return """\
================================================================================
TAD REAL OBJECT [実身 #102] : T-Kernel 2.0 Specification & Guide
RECORD TYPE : 1 (TAD Main Record) | APP : BTRON Text/TAD Editor (t_editor)
AUTHOR      : Ken Sakamura / TRON Forum (Cleanroom Edition)
DATE        : 2026-08-29 | ENCODING : TRON Multilingual Code / TAD Standard
================================================================================

[付箋: DOCUMENT_HEADER | Title="T-Kernel 2.0 Book" | Font="Cho-Kanji-Gothic" | Size=16]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            T-Kernel 2.0 リアルタイムOS仕様書及び開発ガイド
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[仮身] #210 : 第1章 T-Kernel 2.0 コアアーキテクチャ (Core Architecture)
[仮身] #220 : 第2章 ブート及び初期化シーケンス (Startup Sequence)
[仮身] #230 : 第3章 QEMU仮想環境とボード展開 (QEMU & Board Deployment)
[仮身] #240 : 第4章 T2EX拡張サブシステム (T-Kernel 2.0 Extension Suite)

───────────────────────────────────────────────────────────────────────────────
【第1章】 T-Kernel 2.0 コアアーキテクチャ (Core Architecture)
───────────────────────────────────────────────────────────────────────────────
T-Kernel 2.0は、μITRONのリアルタイム性能を継承しつつ、高度な組み込みシステムや
IoTエッジ機器に向けて標準化されたオープンリアルタイムOS仕様です。

1.1 タスク状態遷移モデル (Task State Model)：
  ・DORMANT   : 休止状態 (タスク生成後未起動、または終了後)
  ・READY     : 実行可能状態 (CPU割り当て待ちキューに待機)
  ・RUNNING   : 実行状態 (CPU上で実行中)
  ・WAITING   : 待ち状態 (セマフォ、タイマー、メッセージ待ち)
  ・SUSPENDED : 強制待ち状態 (他タスクからのsus_tsk要求)
  ・WAIT-SUSP : 二重待ち状態 (WAITING かつ SUSPENDED)

1.2 システムコール一覧 (Core API Category)：
  ・タスク管理      : tk_cre_tsk, tk_sta_tsk, tk_del_tsk, tk_chg_pri, tk_rot_rdq
  ・タスク付属同期  : tk_sus_tsk, tk_rsm_tsk, tk_frsm_tsk, tk_slp_tsk, tk_wup_tsk
  ・同期・通信      : tk_cre_sem, tk_wai_sem, tk_sig_sem, tk_cre_flg, tk_wai_flg, tk_set_flg
  ・メッセージバッファ: tk_cre_mbf, tk_snd_mbf, tk_rcv_mbf
  ・固定長メモリプール: tk_cre_mpf, tk_get_mpf, tk_rel_mpf
  ・可変長メモリプール: tk_cre_mpl, tk_get_mpl, tk_rel_mpl
  ・時間管理        : tk_set_tim, tk_get_tim, tk_dly_tsk

───────────────────────────────────────────────────────────────────────────────
【第2章】 ブート及び初期化シーケンス (Startup Sequence)
───────────────────────────────────────────────────────────────────────────────
T-Kernel 2.0の起動処理は、高信頼性と再現性を確保するため段階的に進行します：

  Step 1 [リセットベクタ / Low-level Init]
    ・CPUアーキテクチャ初期化（MMU/MPU設定、キャッシュ無効化/有効化）
    ・スタックポインタ（SSP/USP）の設定
    ・C言語ランタイム初期化（.data領域コピー、.bss領域ゼロクリア）

  Step 2 [ハードウェア依存部初期化 (sysinit)]
    ・クロックジェネレータ、タイマー割り込みコントローラの初期化
    ・シリアルコンソール (UART) の初期化

  Step 3 [カーネルコア起動 (tk_start_system)]
    ・割り込みベクタテーブルの登録
    ・タスク制御ブロック（TCB）プール及びスケジューラの初期化
    ・初期タスク (Initial Task) の生成とディスパッチ開始

───────────────────────────────────────────────────────────────────────────────
【第3章】 QEMU仮想環境とボード展開 (QEMU & Board Deployment)
───────────────────────────────────────────────────────────────────────────────
3.1 サポート対象アーキテクチャ：
  ・ARM Cortex-A7 / A9 / A53 (Raspberry Pi 2/3, Virt-Board)
  ・RISC-V 32/64 (QEMU virt)
  ・x86 / x86_64 (seL4 Microkit Guest Domain)

3.2 QEMU起動コマンドライン (Standard Virt-Board)：
  $ qemu-system-arm -M virt -cpu cortex-a7 -m 128M -kernel btron-tkernel.elf \
      -serial stdio -display sdl

3.3 ドライバ通信レイヤ (VirtIO 1.4 Transport)：
  ・VirtIO-MMIO によるホスト・ゲスト間ゼロコピー通信
  ・Virtqueue (Split/Packed Ring) による高速フレームバッファ転送
"""

def build_tron_hmi_book_tad():
    return """\
================================================================================
TAD REAL OBJECT [実身 #103] : TRON Human-Machine Interface (HMI) Specification
RECORD TYPE : 1 (TAD Main Record) | APP : BTRON Text/TAD Editor (t_editor)
AUTHOR      : Ken Sakamura / PMC TRON Human-Machine Interface Committee
DATE        : 2026-08-29 | ENCODING : TRON Multilingual Code / TAD Standard
================================================================================

[付箋: DOCUMENT_HEADER | Title="TRON HMI Book" | Font="Cho-Kanji-Mincho" | Size=16]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            TRON 人間・機械インタフェース (HMI) 設計仕様書及び標準カタログ
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[仮身] #310 : 第1部 ユーザ向け基本設計原則 (SUI & Ergonomics)
[仮身] #320 : 第2部 エンジニア向け仕様と実装規約 (Cleanroom Architecture)
[仮身] #330 : 第3部 標準部品カタログ (HMI Parts Book Catalog)
[仮身] #340 : 第4部 カノニカル実装例：SONY TC-K777ES ステレオデッキ

───────────────────────────────────────────────────────────────────────────────
【第1部】 ユーザ向け基本設計原則 (SUI & Ergonomics)
───────────────────────────────────────────────────────────────────────────────
TRON HMIは、家庭用家電、産業機械、自動車からワークステーションまで、あらゆる機器で
「操作の一貫性」と「直感性」を保証するために坂村健教授によって策定されました。

1.1 標準動作三原則 (The Standard Action Triad)：
  (1) 操作対象の指定 (Target Selection)
  (2) 操作内容の指定 (Operation Specification)
  (3) 実行の確認／確定 (Execution Confirmation)

1.2 モードレス並列状態遷移 (Modeless Parallel State Model)：
  ・隠れたモードによる誤操作（Mode Confusion）を徹底排除。
  ・現在の状態が視覚的・物理的に常に確認可能。

1.3 タッチ＆リリース・エッジメカニクス (Touch/Release Edge Mechanics)：
  ・指が触れた瞬間（Touch）にフォーカスと視覚フィードバックを表示。
  ・指を離した瞬間（Release）に処理を確定。
  ・押下したまま領域外にスライド（Cancel Gesture）することで安全にキャンセル可能。

───────────────────────────────────────────────────────────────────────────────
【第2部】 エンジニア向け仕様と実装規約 (Cleanroom Architecture)
───────────────────────────────────────────────────────────────────────────────
航空宇宙・組み込みセーフティクリティカル基準（MISRA-C:2012 / NASA JPL 10 Rules）：

2.1 確定的メモリ制約 (Zero Post-Boot Dynamic Allocation)：
  ・全HMIパネル構造体（HMI_PANEL）は最大32コントロール（HMI_PANEL_MAX_CTRLS=32）
    の平坦な固定長静的配列として定義。
  ・実行時のメモリ断片化（Heap Fragmentation）を完全にゼロ化。

2.2 O(N) 確定フォーカス代数 (Bounded Focus Algebra)：
  ・7キー・ユニバーサルコントローラ（UP, DOWN, LEFT, RIGHT, ENTER, MENU, CANCEL）
    によるフォーカス移動アルゴリズムの最悪実行時間を厳格に保証。

───────────────────────────────────────────────────────────────────────────────
【第3部】 標準部品カタログ (HMI Parts Book Catalog)
───────────────────────────────────────────────────────────────────────────────
HMI部品カタログ一覧：
  [1] プッシュスイッチ (Push Switch) : モメンタリ型 / オルタネート型
  [2] アップダウンスステッパ (Up/Down Stepper) : 段階値のインクリメント／デクリメント
  [3] ラジオボタンマトリクス (Radio Matrix) : 排他的単一選択グループ
  [4] アナログボリューム / ロータリーノブ (Rotary Dial) : 270度連続角度調整
  [5] スライダーバー (Linear Slider) : 水平／垂直リニアボリューム
  [6] セグメントVUメーター (Bar VU Meter) : ピークホールド付きオーディオレベル表示
  [7] ユニバーサルコントローラ (Universal Controller) : 7キー統一リモート入力パネル

───────────────────────────────────────────────────────────────────────────────
【第4部】 カノニカル実装例：SONY TC-K777ES ステレオデッキ
───────────────────────────────────────────────────────────────────────────────
BTRON HMIの実証アプリケーション（src/apps/audio_player.c）として、往年の名機
SONY 3ヘッドカセットデッキ「TC-K777ES」を完全シミュレーション：
  ・左右デュアル回転テープリール（走行時間に応じたリアルタイムテープ厚移動）
  ・12セグメント蛍光表示VUバーメーター（減衰率 -2dB/frame のピークホールド）
  ・270度ロータリーマスターボリューム、Bass, Treble, Balance ポテンショメータ
  ・Universal Controller によるリモート操作オーバーレイ
"""

def build_bfree_os_book_tad():
    return """\
================================================================================
TAD REAL OBJECT [実身 #104] : B-Free Operating System Architecture (GPL)
RECORD TYPE : 1 (TAD Main Record) | APP : BTRON Text/TAD Editor (t_editor)
AUTHOR      : B-Free Project / Ken Sakamura Free Software Working Group
DATE        : 2026-08-29 | ENCODING : TRON Multilingual Code / TAD Standard
================================================================================

[付箋: DOCUMENT_HEADER | Title="B-Free OS Book" | Font="Cho-Kanji-Gothic" | Size=16]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            B-Free 自由なBTRON3オペレーティングシステム技術解説書
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[仮身] #410 : 第1章 B-Free マニフェストと自由ソフトウェアの理念 (Manifesto)
[仮身] #420 : 第2章 μITRON 3.0 マイクロカーネルアーキテクチャ (Kernel)
[仮身] #430 : 第3章 POSIXエミュレーション層とシステムコール (POSIX Layer)
[仮身] #440 : 第4章 B-Free OS 統合デスクトップ環境 (BTRON Desktop)
[仮身] #450 : 第5章 ブート機構とソースツリー構造 (Boot & Source Tree)

───────────────────────────────────────────────────────────────────────────────
【第1章】 B-Free マニフェストと自由ソフトウェアの理念 (Manifesto)
───────────────────────────────────────────────────────────────────────────────
B-Free プロジェクトは、Ken Sakamura教授が提唱した「万人に開かれた標準アーキテクチャ」
というTRONの精神に基づき、GPL（GNU General Public License）の下で開発された、
世界初の自由なBTRON3互換オペレーティングシステムです。

1.1 設立趣旨：
  ・ベンダーロックインを排除した、誰でも自由に学習・改変・再配布可能なBTRON環境の実現。
  ・商用B-right/V等のプロプライエタリ実装に依存しないオープンソース標準の樹立。
  ・μITRON 3.0仕様に準拠したマイクロカーネルによる確定的リアルタイム性の提供。

───────────────────────────────────────────────────────────────────────────────
【第2章】 μITRON 3.0 マイクロカーネルアーキテクチャ (Kernel)
───────────────────────────────────────────────────────────────────────────────
B-Freeの中核を担うマイクロカーネル設計：
  ・カーネルスペースとユーザスペースの明確な分離。
  ・タスクコンテキストと非タスクコンテキスト（割り込みハンドラ）の厳密な区別。
  ・メッセージパッシングによるプロセス間通信（IPC）とフォールトアイソレーション。

───────────────────────────────────────────────────────────────────────────────
【第3章】 POSIXエミュレーション層とシステムコール (POSIX Layer)
───────────────────────────────────────────────────────────────────────────────
BTRONアプリケーションとオープンソースUNIXソフトウェア資産の融合：
  ・libposix による標準CライブラリおよびPOSIX.1システムコールのエミュレーション。
  ・fork(), execve(), waitpid(), pipe(), open(), read(), write(), close() の実装。
  ・VFS（仮想ファイルシステム）によるBTRON FS実身／仮身構造とPOSIX階層パスの透過的相互変換。

───────────────────────────────────────────────────────────────────────────────
【第4章】 B-Free OS 統合デスクトップ環境 (BTRON Desktop)
───────────────────────────────────────────────────────────────────────────────
・BTRON3仕様に完全準拠したウィンドウマネージャとティール色（#008080）のワークスペース。
・TAD文書ビューア、テキストエディタ、オブジェクトマネージャ（キャビネット操作）。
・TRON多言語文字コードによる日本語・英語・多言語の自然な混在表示。

───────────────────────────────────────────────────────────────────────────────
【第5章】 ブート機構とソースツリー構造 (Boot & Source Tree)
───────────────────────────────────────────────────────────────────────────────
5.1 ブートシーケンス：
  (1) GRUB / マルチブートローダーによるカーネルバイナリのロード
  (2) ページテーブル及びセグメントディスクリプタの初期化
  (3) μITRONタスクマネージャの起動
  (4) ルートサーバ (POSIX Server & Window Server) の起動
  (5) ユーザシェル (Cabinet Desktop) の表示

5.2 ソースツリー概要：
  /b-free/
    ├── kernel/     : μITRON 3.0 マイクロカーネルソースコード
    ├── posix/      : POSIX エミュレーションサーバおよび libposix
    ├── btron/      : BTRON3 ウィンドウマネージャおよび DP グラフィックス
    ├── apps/       : 標準アプリケーション（T-Editor, Console, Cabinet）
    └── include/    : TRON標準ヘッダファイル群 (itron.h, btron.h, tad.h)
"""

def main():
    os.makedirs(DHARMA_DIR, exist_ok=True)

    books = [
        ("01_btron3_spec.tad", "BTRON3 3.20 Specification Book", build_btron3_spec_tad()),
        ("02_tkernel_book.tad", "T-Kernel 2.0 Real-Time OS Book", build_tkernel_book_tad()),
        ("03_tron_hmi_book.tad", "TRON Human-Machine Interface Book", build_tron_hmi_book_tad()),
        ("04_bfree_os_book.tad", "B-Free Operating System Book", build_bfree_os_book_tad()),
    ]

    print("======================================================================")
    print(" B-System TAD Dharma Packing Tool (4 Books in One Shot)")
    print("======================================================================")

    for filename, title, content in books:
        filepath = os.path.join(DHARMA_DIR, filename)
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(content)
        size_bytes = os.path.getsize(filepath)
        line_count = len(content.splitlines())
        print(f"  [PACKED] {filepath:<28} : {title:<38} ({line_count} lines, {size_bytes} bytes)")

    print("======================================================================")
    print(f" Successfully generated all 4 TAD book files in ./{DHARMA_DIR}/")
    print("======================================================================")

if __name__ == "__main__":
    main()
