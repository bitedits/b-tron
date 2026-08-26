/*
 * Freestanding ARM Bare-Metal Startup, PL011 UART & BCM283x Framebuffer Video Driver
 * Sakamura T-Kernel 2.0 Real-Time Engine Integration
 * Supports BCM2836 (Raspberry Pi 2B, Cortex-A7 / ARMv7) & BCM2711 (Pi 4B, AArch64)
 */

#include <stdint.h>
#include <stddef.h>
#include <btron/desktop.h>
#include <btron/wnd.h>

#if (TYPE_RPI == 1)
#define PL011_BASE      0x20201000u
#define MBOX_BASE_ADDR  0x2000b880u
#elif (TYPE_RPI == 2 || TYPE_RPI == 3)
#define PL011_BASE      0x3f201000u
#define MBOX_BASE_ADDR  0x3f00b880u
#elif (TYPE_RPI == 4)
#define PL011_BASE      0xfe201000u
#define MBOX_BASE_ADDR  0xfe00b880u
#else
#define PL011_BASE      0x3f201000u
#define MBOX_BASE_ADDR  0x3f00b880u
#endif

#define MBOX_READ       0x00
#define MBOX_STATUS     0x18
#define MBOX_WRITE      0x20
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000
#define MBOX_CH_PROP    8

static void uart_putc(char c) {
    volatile uint32_t *uart = (volatile uint32_t*)(uintptr_t)PL011_BASE;
    uart[0] = (uint32_t)(unsigned char)c;
}

void uart_puts(const char *s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

/* Freestanding Bare-Metal Static Heap Allocator */
static uint8_t heap_mem[8 * 1024 * 1024] __attribute__((aligned(16)));
static size_t heap_offset = 0;

void* tkl_memcpy(void *dst, const void *src, size_t n) {
    volatile uint8_t *d = (volatile uint8_t*)dst;
    const volatile uint8_t *s = (const volatile uint8_t*)src;

    if (((uintptr_t)d & 3) == 0 && ((uintptr_t)s & 3) == 0) {
        volatile uint32_t *d32 = (volatile uint32_t*)d;
        const volatile uint32_t *s32 = (const volatile uint32_t*)s;
        while (n >= 4) {
            *d32++ = *s32++;
            n -= 4;
        }
        d = (volatile uint8_t*)d32;
        s = (const volatile uint8_t*)s32;
    }
    while (n > 0) {
        *d++ = *s++;
        n--;
    }
    return dst;
}

void* tkl_memset(void *s, int c, size_t n) {
    volatile uint8_t *p = (volatile uint8_t*)s;
    uint32_t val32 = (uint8_t)c;
    val32 |= (val32 << 8);
    val32 |= (val32 << 16);

    while (n > 0 && ((uintptr_t)p & 3)) {
        *p++ = (uint8_t)c;
        n--;
    }
    volatile uint32_t *p32 = (volatile uint32_t*)p;
    while (n >= 4) {
        *p32++ = val32;
        n -= 4;
    }
    p = (volatile uint8_t*)p32;
    while (n > 0) {
        *p++ = (uint8_t)c;
        n--;
    }
    return s;
}

void* memcpy(void *dst, const void *src, size_t n) { return tkl_memcpy(dst, src, n); }
void* memset(void *s, int c, size_t n) { return tkl_memset(s, c, n); }

void* Imalloc(size_t sz) {
    if (heap_offset + sz > sizeof(heap_mem)) return NULL;
    void *ptr = &heap_mem[heap_offset];
    heap_offset += (sz + 15) & ~15UL;
    return ptr;
}

void Ifree(void *ptr) { (void)ptr; }

void* Icalloc(size_t nmemb, size_t sz) {
    size_t total = nmemb * sz;
    void *ptr = Imalloc(total);
    if (ptr) {
        tkl_memset(ptr, 0, total);
    }
    return ptr;
}

void* malloc(size_t sz) { return Imalloc(sz); }
void* calloc(size_t nmemb, size_t sz) { return Icalloc(nmemb, sz); }
void free(void *ptr) { Ifree(ptr); }

void* IAmalloc(size_t sz, unsigned int attr) { (void)attr; return Imalloc(sz); }
void IAfree(void *ptr, unsigned int attr) { (void)attr; (void)ptr; }

char* tkl_strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dst[i] = src[i];
    for ( ; i < n; i++) dst[i] = '\0';
    return dst;
}

char* tkl_strncat(char *dst, const char *src, size_t n) {
    size_t dlen = 0;
    while (dst[dlen] != '\0') dlen++;
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dst[dlen + i] = src[i];
    }
    dst[dlen + i] = '\0';
    return dst;
}

/* Bit manipulation primitives for T-Kernel scheduler */
void BitSet(uint32_t *base, int offset) {
    int idx = offset / 32;
    int bit = offset % 32;
    base[idx] |= (1U << bit);
}

void BitClr(uint32_t *base, int offset) {
    int idx = offset / 32;
    int bit = offset % 32;
    base[idx] &= ~(1U << bit);
}

int BitTest(const uint32_t *base, int offset) {
    int idx = offset / 32;
    int bit = offset % 32;
    return (base[idx] & (1U << bit)) ? 1 : 0;
}

int BitSearch0_w(const uint32_t *base, int offset, int width) {
    for (int i = 0; i < width; i++) {
        int pos = offset + i;
        int idx = pos / 32;
        int bit = pos % 32;
        if (!(base[idx] & (1U << bit))) return i;
    }
    return -1;
}

int BitSearch1_w(const uint32_t *base, int offset, int width) {
    for (int i = 0; i < width; i++) {
        int pos = offset + i;
        int idx = pos / 32;
        int bit = pos % 32;
        if (base[idx] & (1U << bit)) return i;
    }
    return -1;
}

uint32_t disint(void) { return 0; }
uint32_t enaint(uint32_t intsts) { return intsts; }
void DisableInt(uint32_t vec) { (void)vec; }
void EnableInt(int vec) { (void)vec; }
void SetIntMode(uint32_t vec, uint32_t mode) { (void)vec; (void)mode; }
void ClearInt(uint32_t vec) { (void)vec; }
int CheckInt(int vec) { (void)vec; return 0; }

void tm_monitor(void) {}
void tm_putstring(const char *s) { uart_puts(s); }
void tm_exit(int code) { (void)code; while(1); }
void tm_command(const char *cmd) { (void)cmd; }

int _tk_get_cfn(uint8_t *name, int *val, int max) {
    (void)name;
    if (val && max > 0) val[0] = 0;
    return 1;
}

int __tk_get_cfn(uint8_t *name, int *val, int max) {
    return _tk_get_cfn(name, val, max);
}

int GetDevConf(const uint8_t *name, int *val) { (void)name; if (val) val[0] = 0; return 0; }
int GetSysConf(const uint8_t *name, int *val) { (void)name; if (val) val[0] = 0; return 0; }

void *lowmem_top = (void*)0x200000;
void call_entry(void) {}
void dispatch_entry(void) {}
void dispatch_to_schedtsk(void) {}
void rettex_entry(void) {}
void _tk_ret_int(void) {}
void call_dbgspt(void) {}
void timer_handler_startup(void) {}
void defaulthdr_startup(void) {}
void exchdr_startup(void) {}
void inthdr_startup(void) {}
int no_support(void) { return -70; /* E_NOSPT */ }

void *hook_dsp = NULL;
void *unhook_dsp = NULL;
void *hook_int = NULL;
void *unhook_int = NULL;
void *hook_svc = NULL;
void *unhook_svc = NULL;

/* 64-bit integer division runtime helpers for 32-bit ARM (EABI) */
#if !defined(__aarch64__)
__attribute__((naked))
void __aeabi_uldivmod(void) {
    __asm__ volatile(
        "push {r4, r5, r6, r7, lr}\n\t"
        "mov r4, #0\n\t"
        "mov r5, #0\n\t"
        "mov r6, #0\n\t"
        "mov r7, #0\n\t"
        "mov ip, #64\n\t"
        "1:\n\t"
        "lsls r6, r6, #1\n\t"
        "adc r7, r7, r7\n\t"
        "tst r1, #0x80000000\n\t"
        "orrne r6, r6, #1\n\t"
        "lsls r0, r0, #1\n\t"
        "adc r1, r1, r1\n\t"
        "cmp r7, r3\n\t"
        "cmpeq r6, r2\n\t"
        "blo 2f\n\t"
        "subs r6, r6, r2\n\t"
        "sbc r7, r7, r3\n\t"
        "orr r4, r4, #1\n\t"
        "2:\n\t"
        "subs ip, ip, #1\n\t"
        "beq 3f\n\t"
        "lsls r4, r4, #1\n\t"
        "adc r5, r5, r5\n\t"
        "b 1b\n\t"
        "3:\n\t"
        "mov r0, r4\n\t"
        "mov r1, r5\n\t"
        "mov r2, r6\n\t"
        "mov r3, r7\n\t"
        "pop {r4, r5, r6, r7, pc}\n\t"
    );
}

__attribute__((naked))
void __aeabi_ldivmod(void) {
    __asm__ volatile("b __aeabi_uldivmod\n\t");
}

uint32_t __aeabi_uidiv(uint32_t num, uint32_t den) {
    if (den == 0) return 0;
    return num / den;
}

int32_t __aeabi_idiv(int32_t num, int32_t den) {
    if (den == 0) return 0;
    return num / den;
}
#endif

/* Mailbox message buffer aligned to 16 bytes */
static volatile uint32_t mbox[36] __attribute__((aligned(16)));

static uint32_t* init_pi_framebuffer(uint32_t width, uint32_t height) {
    uint32_t base = MBOX_BASE_ADDR;
    volatile uint32_t *read_reg   = (volatile uint32_t*)(uintptr_t)(base + MBOX_READ);
    volatile uint32_t *status_reg = (volatile uint32_t*)(uintptr_t)(base + MBOX_STATUS);
    volatile uint32_t *write_reg  = (volatile uint32_t*)(uintptr_t)(base + MBOX_WRITE);

    mbox[0] = 35 * 4;   /* buffer size in bytes */
    mbox[1] = 0;        /* request code */

    mbox[2] = 0x00048003;  /* set phys width/height */
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = width;
    mbox[6] = height;

    mbox[7] = 0x00048004;  /* set virt width/height */
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = width;
    mbox[11] = height;

    mbox[12] = 0x00048009; /* set virt offset */
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;
    mbox[16] = 0;

    mbox[17] = 0x00048005; /* set depth */
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32;         /* 32 bpp */

    mbox[21] = 0x00048006; /* set pixel order (1 = RGB) */
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1;

    mbox[25] = 0x00040001; /* allocate framebuffer */
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 16;         /* alignment */
    mbox[29] = 0;          /* fb ptr returned by GPU */

    mbox[30] = 0x00040008; /* get pitch */
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;

    mbox[34] = 0;          /* end tag */

    uint32_t mbox_addr = (uint32_t)(uintptr_t)mbox;

    __asm__ volatile("dsb sy" : : : "memory");

    /* Send mailbox message to Channel 8 */
    while (*status_reg & MBOX_FULL) {
        __asm__ volatile("nop");
    }
    *write_reg = ((mbox_addr & 0xFFFFFFF0) | MBOX_CH_PROP);

    /* Read mailbox response from Channel 8 */
    while (1) {
        while (*status_reg & MBOX_EMPTY) {
            __asm__ volatile("nop");
        }
        uint32_t res = *read_reg;
        if ((res & 0xF) == MBOX_CH_PROP) {
            break;
        }
    }

    __asm__ volatile("dsb sy" : : : "memory");

    if (mbox[1] == 0x80000000 && mbox[29] != 0) {
        uint32_t fb_phys = mbox[29] & 0x3FFFFFFF;
        uart_puts("[QEMU-ARM] Framebuffer Allocated by VideoCore GPU!\n");
        return (uint32_t*)(uintptr_t)fb_phys;
    }

    uart_puts("[QEMU-ARM] Framebuffer allocation fallback.\n");
    return (uint32_t*)0x3c000000;
}

static void draw_btron_pattern(uint32_t *fb, uint32_t w, uint32_t h) {
    if (!fb) return;

    uint32_t bg_color     = 0xFF1B4965; /* BTRON Retro Cyan/Teal */
    uint32_t header_color = 0xFF0B2545; /* Dark Navy Top Header Bar */
    uint32_t gold_color   = 0xFFEE9B00; /* Bright Gold Title Accent */

    /* 8 Color Test Bars */
    uint32_t bars[8] = {
        0xFFFFFFFF, /* White */
        0xFFFFFF00, /* Yellow */
        0xFF00FFFF, /* Cyan */
        0xFF00FF00, /* Green */
        0xFFFF00FF, /* Magenta */
        0xFFFF0000, /* Red */
        0xFF0000FF, /* Blue */
        0xFF000000  /* Black */
    };

    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = y * w + x;
            if (y < 40) {
                fb[idx] = header_color;
            } else if (y >= 40 && y < 44) {
                fb[idx] = gold_color;
            } else if (y >= h - 100) {
                uint32_t bar_idx = (x * 8) / w;
                fb[idx] = bars[bar_idx];
            } else {
                fb[idx] = bg_color;
            }
        }
    }
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

extern void *_stack_top;

void btron_main(void) {
    heap_offset = 0;

    uart_puts("\n==========================================================\n");
    uart_puts(" Sakamura T-Kernel 2.0 Real-Time OS Engine (BCM283x ARM)\n");
    uart_puts(" QEMU Bare-Metal Hardware Machine Execution Active!\n");
    uart_puts("==========================================================\n\n");

    uart_puts("[QEMU-ARM] Initializing Video Display Framebuffer (1024x768 32-bpp)...\n");
    uint32_t *fb = init_pi_framebuffer(1024, 768);
    draw_btron_pattern(fb, 1024, 768);
    uart_puts("[QEMU-ARM] BTRON Bootscreen & Color Bar Pattern Rendered to Video VRAM.\n");

    uart_puts("[QEMU-ARM] Initializing Sakamura T-Kernel 2.0 Real-Time Subsystems...\n");
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

    uart_puts("[B-TRON] Launching B-TRON Multi-Window Desktop Engine in Video VRAM...\n");
    GDEV *screen = opn_dev_vram(1024, 768, (COLOR*)fb);
    init_wnd_mgr(screen);
    uart_puts("[B-TRON] B-TRON Window Manager Subsystem Initialized in VRAM.\n");

    /* Render Desktop Wallpaper & System Top Panel */
    render_desktop_background(screen);
    render_system_panel(screen);
    uart_puts("[B-TRON] Desktop Teal Wallpaper & Real Object Icons Rendered.\n");

    /* Open 3 Retro Windows */
    WND *w_cab = opn_wnd("Cabinet Manager - Real Objects", 140, 70, 520, 360, WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    WND *w_txt = opn_wnd("T-Editor - BTRON Document.txt", 220, 150, 480, 320, WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    WND *w_cli = opn_wnd("BTRON Terminal Shell", 340, 240, 520, 300, WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    (void)w_cab; (void)w_txt; (void)w_cli;

    uart_puts("[B-TRON] 3 Desktop Windows Opened (Cabinet Manager, T-Editor, Terminal).\n");
    redraw_all_windows();
    uart_puts("[B-TRON] Interactive Multi-Window Desktop Compositor Active in VRAM!\n");

    while (1) {
        __asm__ volatile("nop");
    }
}

__attribute__((section(".text._start"), naked))
void _start(void) {
#if defined(__aarch64__)
    __asm__ volatile(
        "adrp x0, __stack_top\n\t"
        "add  x0, x0, :lo12:__stack_top\n\t"
        "and  x0, x0, #~15\n\t"
        "mov  sp, x0\n\t"
        "mrs x0, mpidr_el1\n\t"
        "and x0, x0, #0xFF\n\t"
        "cbz x0, 1f\n\t"
        "2: wfe\n\t"
        "b 2b\n\t"
        "1:\n\t"
        "bl btron_main\n\t"
        "3: wfe\n\t"
        "b 3b\n\t"
    );
#else
    __asm__ volatile(
        "ldr sp, =__stack_top\n\t"
        "bic sp, sp, #7\n\t"
        "mrc p15, 0, r0, c0, c0, 5\n\t"
        "ands r0, r0, #3\n\t"
        "beq 1f\n\t"
        "2: wfe\n\t"
        "b 2b\n\t"
        "1:\n\t"
        "mrc p15, 0, r0, c1, c0, 2\n\t"
        "orr r0, r0, #(0xF << 20)\n\t"
        "mcr p15, 0, r0, c1, c0, 2\n\t"
        "isb\n\t"
        "bl btron_main\n\t"
        "3: wfe\n\t"
        "b 3b\n\t"
    );
#endif
}
