/*
 * B-System (BTRON 3.20) Clarity Publishing System (src/apps/clarity.c)
 * QuarkXPress / Word-Class DTP Compositor & Cabinet TAD Link Aggregator
 * First-Class Cultural Preservation: Horizontal Tibetan Pecha (དཔེ་ཆ་) & Native Japanese Legacy Paper Formats (和装本・和式伝統判型)
 */

#include <btron/wnd.h>
#include <btron/dp.h>
#include <btron/tad_browser.h>
#include <btron/vobj.h>
#include <btron/event.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

typedef enum {
    CLARITY_MODE_DTP = 0,
    CLARITY_MODE_TEX = 1,
    CLARITY_MODE_PECHA = 2,    /* Horizontal Tibetan Pecha (དཔེ་ཆ་) Mode */
    CLARITY_MODE_WASOBON = 3,  /* Native Japanese Legacy Vertical Wasōbon (和装本・縦書き) Mode */
    CLARITY_MODE_PREVIEW = 4
} ClarityMode;

typedef enum {
    FLOW_HORIZONTAL_LTR = 0,
    FLOW_HORIZONTAL_PECHA = 1, /* Tibetan Horizontal lines with margin folio markers */
    FLOW_VERTICAL_RTL = 2      /* Japanese Traditional Vertical Right-to-Left (縦書き) */
} ClarityTextFlowDirection;

/* Native Japanese Legacy Paper Formats (和式伝統判型・日本出版文化保全) */
typedef enum {
    JP_PAPER_MINO_BAN = 0,      /* 美濃判 (Mino-ban): 273 x 394 mm (Edo standard official & woodblock) */
    JP_PAPER_HANSHI,            /* 半紙 (Hanshi): 242 x 333 mm (Classic calligraphy & washi manuscript) */
    JP_PAPER_SHIROKU_BAN,       /* 四六判 (Shiroku-ban): 127 x 188 mm (Standard literary novel publishing) */
    JP_PAPER_KIKU_BAN,          /* 菊判 (Kiku-ban): 150 x 218 mm (Academic journals, essays, art books) */
    JP_PAPER_SHINSHO_BAN,       /* 新書判 (Shinsho-ban): 105 x 173 mm (Classic pocket non-fiction) */
    JP_PAPER_BUNKO_BAN,         /* 文庫判 (Bunko-ban): 105 x 148 mm (Pocket literature editions) */
    JP_PAPER_HOSHO_BAN,         /* 大奉書 (Hōsho-ban): 394 x 530 mm (Sacred Imperial decrees & Ukiyo-e) */
    JP_PAPER_DAIFUKUCHO,        /* 大福帳 (Daifukuchō-ban): 160 x 240 mm (Merchant ledger binding) */
    JP_PAPER_KAISHI,            /* 懐紙 (Kaishi-ban): 145 x 175 mm (Waka poetry & tea ceremony washi) */
    JP_PAPER_TANZAKU,           /* 短冊 (Tanzaku-ban): 60 x 363 mm (Narrow vertical Haiku/Waka strip) */
    JP_PAPER_SHIKISHI,          /* 色紙 (Shikishi-ban): 242 x 272 mm (Square formal calligraphy board) */
    JP_PAPER_ORIHON,            /* 折本 (Orihon): Accordion-folded sutra format (e.g. 80 x 260 mm) */
    JP_PAPER_KANSUBON,          /* 巻子本 (Kansubon): Continuous scroll format with vertical columns */
    JP_PAPER_WASOBON_FUKUROTOJI /* 和装本・袋綴じ: Four-hole stitched pouch book */
} JapaneseLegacyPaperFormat;

/* Tibetan Pecha Canonical Formats (དཔེ་ཆ་) */
typedef enum {
    PECHA_SIZE_RINCHEN_TERDZO = 0, /* Large Canonical Pecha: 650 x 140 mm */
    PECHA_SIZE_KANGYUR,            /* Standard Kangyur / Tengyur: 560 x 110 mm */
    PECHA_SIZE_DERGE,              /* Derge Woodblock Edition: 700 x 180 mm */
    PECHA_SIZE_POCKET_DHARMA       /* Monastic Recitation Pocket Pecha: 320 x 85 mm */
} TibetanPechaSize;

typedef struct {
    TibetanPechaSize pecha_size;
    int width_mm;
    int height_mm;
    BOOL double_border_kheng_khe;  /* Red/Black ceremonial border frame (lcags-ri) */
    BOOL enable_interlinear_mchan; /* Small commentary verses (mchan-'grel) */
    char folio_left_label[32];     /* Title / Section marker (sna-chen) */
    int folio_number;              /* Tibetan numeral pagination (tshig-grangs) */
    BOOL is_recto_verso;           /* G.yon (Left) vs G.yas (Right) leaf */
} PechaLayoutConfig;

typedef struct {
    JapaneseLegacyPaperFormat format;
    int width_mm;
    int height_mm;
    BOOL is_tategaki;              /* Vertical Right-to-Left flow (縦書き) */
    BOOL enable_ruby;              /* Furigana ruby annotations on right side of glyphs */
    BOOL enable_warichu;           /* Inline double-column note splits (割注) */
    BOOL enable_kinsoku;           /* Japanese punctuation & bracket line-breaking rules */
    BOOL enable_tate_chu_yoko;     /* Tate-chu-yoko for numbers/alphanumerics in vertical lines */
    int gyo_dori_lines;            /* Multi-line heading span (行取り) */
} WasobonLayoutConfig;

typedef struct {
    int frame_id;
    RECT bounds;
    int columns;
    ClarityTextFlowDirection flow;
    UW linked_robj_id;            /* Dynamic link to Real Body in Cabinet */
    int next_frame_id;            /* Linked text flow chain */
} ClarityTextFrame;

typedef struct {
    WND *wnd;
    ClarityMode mode;
    int page_count;
    int current_page;
    ClarityTextFrame frames[32];
    int frame_count;
    BOOL live_cabinet_sync;

    /* Cultural Typography Subsystems */
    PechaLayoutConfig pecha_cfg;
    WasobonLayoutConfig wasobon_cfg;
} ClarityDoc;

extern int clarity_tex_compile_mode(ClarityDoc *doc, const char *tex_source);
extern int clarity_tex_render_formula(const char *latex_math, void *dp_surface, int x, int y);

void clarity_init(ClarityDoc *doc) {
    if (!doc) return;
    memset(doc, 0, sizeof(ClarityDoc));
    doc->mode = CLARITY_MODE_DTP;
    doc->live_cabinet_sync = TRUE;

    /* Default Tibetan Pecha config */
    doc->pecha_cfg.pecha_size = PECHA_SIZE_KANGYUR;
    doc->pecha_cfg.width_mm = 560;
    doc->pecha_cfg.height_mm = 110;
    doc->pecha_cfg.double_border_kheng_khe = TRUE;
    doc->pecha_cfg.enable_interlinear_mchan = TRUE;

    /* Default Japanese Legacy format: Shiroku-ban vertical */
    doc->wasobon_cfg.format = JP_PAPER_SHIROKU_BAN;
    doc->wasobon_cfg.width_mm = 127;
    doc->wasobon_cfg.height_mm = 188;
    doc->wasobon_cfg.is_tategaki = TRUE;
    doc->wasobon_cfg.enable_ruby = TRUE;
    doc->wasobon_cfg.enable_warichu = TRUE;
    doc->wasobon_cfg.enable_kinsoku = TRUE;
    doc->wasobon_cfg.enable_tate_chu_yoko = TRUE;
}

void clarity_set_pecha_mode(ClarityDoc *doc, TibetanPechaSize size) {
    if (!doc) return;
    doc->mode = CLARITY_MODE_PECHA;
    doc->pecha_cfg.pecha_size = size;
    switch (size) {
        case PECHA_SIZE_RINCHEN_TERDZO:
            doc->pecha_cfg.width_mm = 650; doc->pecha_cfg.height_mm = 140; break;
        case PECHA_SIZE_KANGYUR:
            doc->pecha_cfg.width_mm = 560; doc->pecha_cfg.height_mm = 110; break;
        case PECHA_SIZE_DERGE:
            doc->pecha_cfg.width_mm = 700; doc->pecha_cfg.height_mm = 180; break;
        case PECHA_SIZE_POCKET_DHARMA:
            doc->pecha_cfg.width_mm = 320; doc->pecha_cfg.height_mm = 85; break;
    }
}

void clarity_set_japanese_legacy_format(ClarityDoc *doc, JapaneseLegacyPaperFormat fmt, BOOL vertical) {
    if (!doc) return;
    doc->mode = CLARITY_MODE_WASOBON;
    doc->wasobon_cfg.format = fmt;
    doc->wasobon_cfg.is_tategaki = vertical;
    switch (fmt) {
        case JP_PAPER_MINO_BAN: doc->wasobon_cfg.width_mm = 273; doc->wasobon_cfg.height_mm = 394; break;
        case JP_PAPER_HANSHI: doc->wasobon_cfg.width_mm = 242; doc->wasobon_cfg.height_mm = 333; break;
        case JP_PAPER_SHIROKU_BAN: doc->wasobon_cfg.width_mm = 127; doc->wasobon_cfg.height_mm = 188; break;
        case JP_PAPER_KIKU_BAN: doc->wasobon_cfg.width_mm = 150; doc->wasobon_cfg.height_mm = 218; break;
        case JP_PAPER_SHINSHO_BAN: doc->wasobon_cfg.width_mm = 105; doc->wasobon_cfg.height_mm = 173; break;
        case JP_PAPER_BUNKO_BAN: doc->wasobon_cfg.width_mm = 105; doc->wasobon_cfg.height_mm = 148; break;
        case JP_PAPER_HOSHO_BAN: doc->wasobon_cfg.width_mm = 394; doc->wasobon_cfg.height_mm = 530; break;
        case JP_PAPER_DAIFUKUCHO: doc->wasobon_cfg.width_mm = 160; doc->wasobon_cfg.height_mm = 240; break;
        case JP_PAPER_KAISHI: doc->wasobon_cfg.width_mm = 145; doc->wasobon_cfg.height_mm = 175; break;
        case JP_PAPER_TANZAKU: doc->wasobon_cfg.width_mm = 60; doc->wasobon_cfg.height_mm = 363; break;
        case JP_PAPER_SHIKISHI: doc->wasobon_cfg.width_mm = 242; doc->wasobon_cfg.height_mm = 272; break;
        case JP_PAPER_ORIHON: doc->wasobon_cfg.width_mm = 80; doc->wasobon_cfg.height_mm = 260; break;
        case JP_PAPER_KANSUBON: doc->wasobon_cfg.width_mm = 280; doc->wasobon_cfg.height_mm = 1200; break;
        case JP_PAPER_WASOBON_FUKUROTOJI: doc->wasobon_cfg.width_mm = 180; doc->wasobon_cfg.height_mm = 250; break;
    }
}

int clarity_link_cabinet_source(ClarityDoc *doc, int frame_idx, UW robj_id) {
    if (!doc || frame_idx < 0 || frame_idx >= 32) return -1;
    doc->frames[frame_idx].linked_robj_id = robj_id;
    return 0;
}

void clarity_render_page(ClarityDoc *doc, int page_num) {
    (void)page_num;
    if (!doc || !doc->wnd) return;
    /* Render multi-column master page, baseline grid, Pecha decorative borders, or Wasōbon vertical frames */
}
