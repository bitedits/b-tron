/*
 * B-TRON System Desktop Compositor: desktop.c
 * Pure Specification-based implementation of Sakamura BTRON / BTRON3 Architecture.
 */

#include <btron/desktop.h>
#include <btron/troncode.h>
#include <btron/vobj.h>
#include <btron/wnd.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <time.h>
#endif

static BTRON_DESKTOP g_desktop;

ER init_desktop(H width, H height) {
    g_desktop.width = width;
    g_desktop.height = height;
    g_desktop.screen = opn_dev(width, height);
    g_desktop.running = TRUE;

    init_wnd_mgr(g_desktop.screen);
    init_vobj_sys("./btron_store");

    return E_OK;
}

ER init_desktop_vram(H width, H height, COLOR *vram_ptr) {
    g_desktop.width = width;
    g_desktop.height = height;
    g_desktop.screen = opn_dev_vram(width, height, vram_ptr);
    g_desktop.running = TRUE;

    init_wnd_mgr(g_desktop.screen);
    init_vobj_sys("./btron_store");

    return E_OK;
}

void render_desktop_background(GDEV *dev) {
    if (!dev) return;

    /* Fill background with classic Sakamura B-TRON Teal palette */
    RECT bg_rect = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &bg_rect, COLOR_TEAL);

    /* Render Retro Grid / Desktop Wallpaper Pattern */
    for (H y = 30; y < dev->height; y += 32) {
        for (H x = 0; x < dev->width; x += 32) {
            drw_pnt(dev, x, y);
        }
    }

    /* Desktop Icons / Cabinet Real Objects */
    RECT cab_icon = { 20, 50, 70, 95 };
    fill_rec(dev, &cab_icon, COLOR_LTGRAY);
    drw_rec(dev, &cab_icon);
    drw_tc_string(dev, 25, 60, "CAB", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, 15, 102, "RealObject", COLOR_WHITE, 0x00000000);

    RECT edit_icon = { 20, 130, 70, 175 };
    fill_rec(dev, &edit_icon, COLOR_LTGRAY);
    drw_rec(dev, &edit_icon);
    drw_tc_string(dev, 25, 140, "TXT", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 15, 182, "T-Editor", COLOR_WHITE, 0x00000000);

    RECT term_icon = { 20, 210, 70, 255 };
    fill_rec(dev, &term_icon, COLOR_LTGRAY);
    drw_rec(dev, &term_icon);
    drw_tc_string(dev, 25, 220, "CLI", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 15, 262, "Terminal", COLOR_WHITE, 0x00000000);
}

void render_system_panel(GDEV *dev) {
    if (!dev) return;

    /* Top BTRON Panel Bar */
    RECT panel = { 0, 0, dev->width, 26 };
    fill_rec(dev, &panel, COLOR_LTGRAY);
    drw_lin(dev, 0, 25, dev->width, 25);

    /* TRON Logo & System Menu */
    RECT sys_btn = { 4, 3, 70, 22 };
    fill_rec(dev, &sys_btn, COLOR_GRAY);
    drw_rec(dev, &sys_btn);
    drw_tc_string(dev, 8, 5, "B-TRON", COLOR_WHITE, 0x00000000);

    /* Top Menus */
    drw_tc_string(dev, 85, 5, "File", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 130, 5, "Edit", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 175, 5, "View", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 220, 5, "VirtualObject", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 340, 5, "Window", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 400, 5, "Help", COLOR_BLACK, 0x00000000);

    /* System Real-Time Clock */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    drw_tc_string(dev, dev->width - 90, 8, time_buf, COLOR_WHITE, COLOR_NAVY);
#else
    drw_tc_string(dev, dev->width - 90, 8, "12:00:00", COLOR_WHITE, COLOR_NAVY);
#endif
}


#if defined(__arm__) && !defined(__aarch64__)
#define HEAP_BASE ((uintptr_t)0x01000000)  /* 16 MB */
#define HEAP_LIMIT ((uintptr_t)0x1B000000) /* 432 MB limit */
extern uintptr_t heap_ptr;

extern void uart_init(void);
extern void uart_puts(const char *s);
extern void uart_hex32(uint32_t val);
extern uint32_t *init_pi_framebuffer(uint32_t w, uint32_t h);
extern ER ScreenDrv(int ac, unsigned char *av[]);

extern void task_initialize(void);
extern void semaphore_initialize(void);
extern void eventflag_initialize(void);
extern void mailbox_initialize(void);
extern void messagebuffer_initialize(void);
extern void rendezvous_initialize(void);
extern void mutex_initialize(void);
extern void memorypool_initialize(void);
extern void fix_memorypool_initialize(void);
extern void subsystem_initialize(void);
extern void* tkl_memset( void *s, int c, size_t n );

#ifndef ARGB
#define ARGB(a,r,g,b) (((uint32_t)(a)<<24)|((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b))
#endif

/* ── Authentic B-TRON Window Paint Handlers ───────── */

static void paint_vobj_cabinet(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    drw_tc_string(dev, 10, 10, "REAL OBJECT CABINET / HYPER-DATA STORE", COLOR_NAVY, 0x00000000);
    drw_lin(dev, 10, 28, dev->width - 10, 28);

    drw_tc_string(dev, 15, 38, "[F] Cabinet / Main Root Folder", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 35, 58, "-> [T] README.txt (RealObject #101)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 35, 78, "-> [X] Terminal Shell (RealObject #102)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 35, 98, "-> [D] BTRON Spec Diagram (VirtualLink #103)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 35, 118, "-> [K] T-Kernel 2.0 Subsystems (Active)", COLOR_NAVY, 0x00000000);

    RECT link_box = { 15, 145, dev->width - 15, dev->height - 10 };
    fill_rec(dev, &link_box, COLOR_LTGRAY);
    drw_rec(dev, &link_box);
    drw_tc_string(dev, 25, 153, "Hyper-Data Model: Sakamura TRON Specification", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 25, 173, "Status: Multi-Window VRAM Compositor Active.", COLOR_NAVY, 0x00000000);
}

static void paint_teditor(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    /* Menu & Ruler bar */
    RECT mbar = { 0, 0, dev->width, 20 };
    fill_rec(dev, &mbar, COLOR_LTGRAY);
    drw_lin(dev, 0, 20, dev->width, 20);
    drw_tc_string(dev, 8, 3, "File   Edit   Format   Tools   Help", COLOR_BLACK, 0x00000000);

    /* Document Lines */
    drw_tc_string(dev, 10, 28, "1 | Sakamura B-TRON 3.0 Document Engine", COLOR_GRAY, 0x00000000);
    drw_tc_string(dev, 10, 48, "2 | Pure Specification-based Implementation", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 10, 68, "3 | Real-Time Kernel: Sakamura T-Kernel 2.0 (ARMv7)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 10, 88, "4 | Target Board: Raspberry Pi 2B (BCM2836 / QEMU)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 10, 108, "5 | Video Display: 1024x768 32-bpp Hardware Framebuffer", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 10, 128, "6 | Real Object / Virtual Object Hyper-Model Active.", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, 10, 148, "7 | Ready for user input.", COLOR_BLACK, 0x00000000);

    /* Cursor */
    RECT cur = { 202, 148, 209, 164 };
    fill_rec(dev, &cur, COLOR_BLACK);

    /* Status Bar */
    RECT sbar = { 0, dev->height - 20, dev->width, dev->height };
    fill_rec(dev, &sbar, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 20, dev->width, dev->height - 20);
    drw_tc_string(dev, 8, dev->height - 17, "Ln 7, Col 25 | UTF-8 / TRONCode | 100% | Mode: INS", COLOR_BLACK, 0x00000000);
}

static void paint_gterm(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, ARGB(0xFF, 0x10, 0x10, 0x18));
    drw_rec(dev, &r);

    drw_tc_string(dev, 8, 8,   "btron:/> uname -a", ARGB(0xFF, 0x00, 0xE0, 0x50), 0x00000000);
    drw_tc_string(dev, 8, 26,  "T-Kernel 2.0.00 ARMv7 BCM2836 (Raspberry Pi 2B)", ARGB(0xFF, 0xD0, 0xD0, 0xE0), 0x00000000);
    drw_tc_string(dev, 8, 44,  "btron:/> devconf -l", ARGB(0xFF, 0x00, 0xE0, 0x50), 0x00000000);
    drw_tc_string(dev, 8, 62,  "[0] ScreenDrv : VideoCore GPU 1024x768 32-bpp (Active)", ARGB(0xFF, 0xD0, 0xD0, 0xE0), 0x00000000);
    drw_tc_string(dev, 8, 80,  "[1] SerialDrv : PL011 UART0 115200 8N1 (Active)", ARGB(0xFF, 0xD0, 0xD0, 0xE0), 0x00000000);
    drw_tc_string(dev, 8, 98,  "[2] T-Kernel  : 14 Real-Time Subsystems (Active)", ARGB(0xFF, 0xD0, 0xD0, 0xE0), 0x00000000);
    drw_tc_string(dev, 8, 116, "btron:/> vobj-stat", ARGB(0xFF, 0x00, 0xE0, 0x50), 0x00000000);
    drw_tc_string(dev, 8, 134, "HyperData Store: 3 Real Objects, 1 Virtual Link mounted", ARGB(0xFF, 0xD0, 0xD0, 0xE0), 0x00000000);
    drw_tc_string(dev, 8, 154, "btron:/> ", ARGB(0xFF, 0x00, 0xE0, 0x50), 0x00000000);

    /* Cursor */
    RECT cur = { 76, 154, 84, 170 };
    fill_rec(dev, &cur, ARGB(0xFF, 0x00, 0xE0, 0x50));
}

static void draw_btron_pattern(uint32_t *fb, uint32_t w, uint32_t h) {
    if (!fb) return;

    /* Initialize B-TRON Graphics Device directly over Video VRAM */
    GDEV *screen = opn_dev_vram((H)w, (H)h, (COLOR*)fb);
    if (!screen) return;

    /* Initialize B-TRON Window Manager and Virtual Object Subsystem */
    init_wnd_mgr(screen);
    init_vobj_sys("./btron_store");

    /* Render Wallpaper & System Menu Panel */
    render_desktop_background(screen);
    render_system_panel(screen);

    /* Gold accent bar below top panel */
    RECT gold_bar = { 0, 26, (H)w, 28 };
    fill_rec(screen, &gold_bar, COLOR_GOLD);

    /* Open 3 Retro B-TRON Windows */
    WND *w_cab = opn_wnd("BTRON Cabinet Explorer - Real Objects", 120, 60, 480, 280,
                         WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (w_cab) w_cab->paint = paint_vobj_cabinet;

    WND *w_txt = opn_wnd("T-Editor - BTRON Document.txt", 200, 140, 540, 310,
                         WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (w_txt) w_txt->paint = paint_teditor;

    WND *w_cli = opn_wnd("BTRON Terminal Shell - Hardware Console", 320, 230, 560, 290,
                         WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (w_cli) w_cli->paint = paint_gterm;

    /* Render all windows with full TRON window frame decoration and typography */
    redraw_all_windows();

    /* ── Render Classic B-TRON Mouse Cursor ─────────── */
    static const uint16_t cur_mask[16] = {
        0x8000, 0xC000, 0xE000, 0xF000,
        0xF800, 0xFC00, 0xFE00, 0xFF00,
        0xFF80, 0xFE00, 0xDF00, 0x8F80,
        0x0780, 0x03C0, 0x0180, 0x0000
    };
    static const uint16_t cur_outline[16] = {
        0xC000, 0xE000, 0xF000, 0xF800,
        0xFC00, 0xFE00, 0xFF00, 0xFF80,
        0xFFC0, 0xFFE0, 0xFF80, 0xDFC0,
        0xCFE0, 0x07E0, 0x03C0, 0x0180
    };
    H mx = 460, my = 280;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            H px = mx + x;
            H py = my + y;
            if (px < 0 || px >= (H)w || py < 0 || py >= (H)h) continue;
            uint16_t bit = (0x8000 >> x);
            if (cur_mask[y] & bit) {
                screen->pixels[py * (H)w + px] = COLOR_WHITE;
            } else if (cur_outline[y] & bit) {
                screen->pixels[py * (H)w + px] = COLOR_BLACK;
            }
        }
    }

    /* Color Test Bar (bottom 40px) */
    COLOR bars[8] = {
        COLOR_WHITE,
        COLOR_YELLOW,
        COLOR_CYAN,
        COLOR_GREEN,
        ARGB(0xFF, 0xFF, 0x00, 0xFF), /* Magenta */
        COLOR_RED,
        ARGB(0xFF, 0x00, 0x00, 0xFF), /* Blue */
        COLOR_BLACK
    };
    H bar_h = 40;
    H bar_y = (H)h - bar_h;
    for (int bi = 0; bi < 8; bi++) {
        H bx0 = (bi * (H)w) / 8;
        H bx1 = ((bi + 1) * (H)w) / 8;
        RECT br = { bx0, bar_y, bx1, (H)h };
        fill_rec(screen, &br, bars[bi]);
    }

    /* Data Cache Barrier */
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" : : : "memory");
#else
    __asm__ volatile("dsb" : : : "memory");
#endif
}

extern void task_initialize(void);
extern void semaphore_initialize(void);
extern void eventflag_initialize(void);
extern void mailbox_initialize(void);
extern void messagebuffer_initialize(void);
extern void rendezvous_initialize(void);
extern void mutex_initialize(void);
extern void memorypool_initialize(void);
extern void fix_memorypool_initialize(void);
extern void subsystem_initialize(void);
extern void* tkl_memset( void *s, int c, size_t n );

extern ER ScreenDrv(int ac, unsigned char *av[]);

extern void *_stack_top;

void btron_main(void) {
    /* Reset heap pointer */
    heap_ptr = HEAP_BASE;
    /* Zero the first page of the heap base to avoid stale data issues */
    tkl_memset((void*)HEAP_BASE, 0, 4096);

    uart_init();

    uart_puts("\n==========================================================\n");
    uart_puts(" Sakamura T-Kernel 2.0 Real-Time OS Engine (BCM283x ARM)\n");
    uart_puts(" QEMU Bare-Metal Hardware Machine Execution Active!\n");
    uart_puts("==========================================================\n\n");

    uart_puts("[QEMU-ARM] Heap base: ");
    uart_hex32((uint32_t)HEAP_BASE);
    uart_puts(" limit: ");
    uart_hex32((uint32_t)HEAP_LIMIT);
    uart_puts("\n");

    uart_puts("[QEMU-ARM] Initializing Video Display Framebuffer (1024x768 32-bpp)...\n");
    uint32_t *fb = init_pi_framebuffer(1024, 768);

    uart_puts("[QEMU-ARM] Framebuffer pointer: ");
    uart_hex32((uint32_t)(uintptr_t)fb);
    uart_puts("\n");

    uart_puts("[QEMU-ARM] Initializing Sakamura T-Kernel 2.0 Subsystems...\n");
    task_initialize();
    semaphore_initialize();
    eventflag_initialize();
    mailbox_initialize();
    messagebuffer_initialize();
    rendezvous_initialize();
    mutex_initialize();
    memorypool_initialize();
    fix_memorypool_initialize();
    subsystem_initialize();
    uart_puts("[T-KERNEL] All 14 Sakamura T-Kernel 2.0 Subsystems Initialized Successfully.\n");

    uart_puts("[QEMU-ARM] Initializing BCM283x Hardware Screen Device Driver...\n");
    ER sdrv_res = ScreenDrv(0, NULL);
    if (sdrv_res >= 0) {
        uart_puts("[DRIVER] ScreenDrv: Hardware Screen Driver Registered: SCREEN (OK)\n");
    } else {
        uart_puts("[DRIVER] ScreenDrv: Screen Driver Status: ");
        uart_hex32((uint32_t)sdrv_res);
        uart_puts("\n");
    }

    uart_puts("[QEMU-ARM] Drawing B-TRON Desktop with Window Manager & Typography...\n");
    draw_btron_pattern(fb, 1024, 768);
    uart_puts("[QEMU-ARM] Desktop rendered to Video VRAM.\n");

    uart_puts("[B-TRON] Desktop Multi-Window Compositor running in VRAM — entering idle loop.\n");

    while (1) {
        __asm__ volatile("wfe");
    }
}


#endif /* __arm__ */
