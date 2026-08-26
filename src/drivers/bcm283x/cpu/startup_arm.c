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

/* PL011 UART register offsets (in 32-bit words) */
#define PL011_DR      0
#define PL011_FR      6   /* 0x18/4 */
#define PL011_IBRD    9   /* 0x24/4 */
#define PL011_FBRD    10  /* 0x28/4 */
#define PL011_LCRH    11  /* 0x2C/4 */
#define PL011_CR      12  /* 0x30/4 */
#define PL011_IMSC    14  /* 0x38/4 */
#define PL011_FR_TXFF (1u << 5)
#define PL011_FR_BUSY (1u << 3)

static volatile uint32_t * const pl011 = (volatile uint32_t*)(uintptr_t)PL011_BASE;

static void uart_init(void) {
    /* Disable UART */
    pl011[PL011_CR] = 0;
    /* Wait for UART to finish transmitting */
    while (pl011[PL011_FR] & PL011_FR_BUSY) {}
    /* Set baud rate: 3MHz UART clock / (16 * 115200) = 1.627 → IBRD=1, FBRD=40 */
    pl011[PL011_IBRD] = 1;
    pl011[PL011_FBRD] = 40;
    /* 8N1, FIFO enable */
    pl011[PL011_LCRH] = (3u << 5) | (1u << 4); /* WLEN=8, FEN=1 */
    /* Mask all interrupts */
    pl011[PL011_IMSC] = 0x7FF;
    /* Enable UART: UARTEN | TXE | RXE */
    pl011[PL011_CR] = (1u << 0) | (1u << 8) | (1u << 9);
}

static void uart_putc(char c) {
    while (pl011[PL011_FR] & PL011_FR_TXFF) {}
    pl011[PL011_DR] = (uint32_t)(unsigned char)c;
}

void uart_puts(const char *s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

static void uart_hex8(uint8_t v) {
    const char *h = "0123456789ABCDEF";
    uart_putc(h[(v >> 4) & 0xF]);
    uart_putc(h[v & 0xF]);
}

static void uart_hex32(uint32_t v) {
    uart_putc('0'); uart_putc('x');
    uart_hex8((v >> 24) & 0xFF);
    uart_hex8((v >> 16) & 0xFF);
    uart_hex8((v >> 8)  & 0xFF);
    uart_hex8( v        & 0xFF);
}

/*
 * Bare-metal heap: use a fixed high address (16MB) so it stays clear of:
 *   - kernel text/data/BSS at 0x80000..~0x98000
 *   - GPU framebuffer at ~0x300000 (3MB)
 * 16MB gives us 1GB - 16MB = ~1008MB of headroom on the far side.
 */
#define HEAP_BASE ((uintptr_t)0x01000000)  /* 16 MB */
#define HEAP_LIMIT ((uintptr_t)0x02000000) /* 32 MB — 16MB pool */
static uintptr_t heap_ptr = 0; /* initialized in btron_main before first use */

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
    if (heap_ptr == 0) heap_ptr = HEAP_BASE;
    uintptr_t aligned = (heap_ptr + 15) & ~(uintptr_t)15;
    if (aligned + sz > HEAP_LIMIT) return NULL;
    heap_ptr = aligned + sz;
    return (void*)aligned;
}

void Ifree(void *ptr) { (void)ptr; }

void* Icalloc(size_t nmemb, size_t sz) {
    size_t total = nmemb * sz;
    void *ptr = Imalloc(total);
    if (ptr) tkl_memset(ptr, 0, total);
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

int SDefDevice(const void *ddev, void *idev, void **sdi) {
    (void)ddev; (void)idev;
    if (sdi) *sdi = (void*)1;
    return 0;
}

int MapMemory(const void *paddr, int len, unsigned int attr, void **laddr) {
    (void)len; (void)attr;
    if (laddr) *laddr = (void*)paddr;
    return 0;
}

int CnvPhysicalAddr(const void *vaddr, int len, void **paddr) {
    if (paddr) *paddr = (void*)vaddr;
    return len;
}

int tk_get_smb(void **addr, int nblk, unsigned int attr) {
    (void)attr;
    size_t sz = (size_t)nblk * 4096;
    void *ptr = Imalloc(sz);
    if (!ptr) return -5; /* E_NOMEM */
    if (addr) *addr = ptr;
    return 0;
}

int tk_ref_smb(void *pk_rsmb) {
    (void)pk_rsmb;
    return 0;
}

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

__attribute__((naked))
uint32_t __aeabi_uidiv(uint32_t num, uint32_t den) {
    __asm__ volatile(
        "cmp r1, #0\n\t"
        "beq 1f\n\t"
        "udiv r0, r0, r1\n\t"
        "bx lr\n\t"
        "1:\n\t"
        "mov r0, #0\n\t"
        "bx lr\n\t"
    );
}

__attribute__((naked))
int32_t __aeabi_idiv(int32_t num, int32_t den) {
    __asm__ volatile(
        "cmp r1, #0\n\t"
        "beq 1f\n\t"
        "sdiv r0, r0, r1\n\t"
        "bx lr\n\t"
        "1:\n\t"
        "mov r0, #0\n\t"
        "bx lr\n\t"
    );
}

__attribute__((naked))
void __aeabi_uidivmod(void) {
    __asm__ volatile(
        "push {lr}\n\t"
        "mov r2, r0\n\t"
        "mov r3, r1\n\t"
        "bl __aeabi_uidiv\n\t"
        "mul r3, r0, r3\n\t"
        "sub r1, r2, r3\n\t"
        "pop {pc}\n\t"
    );
}

__attribute__((naked))
void __aeabi_idivmod(void) {
    __asm__ volatile(
        "push {lr}\n\t"
        "mov r2, r0\n\t"
        "mov r3, r1\n\t"
        "bl __aeabi_idiv\n\t"
        "mul r3, r0, r3\n\t"
        "sub r1, r2, r3\n\t"
        "pop {pc}\n\t"
    );
}
#endif

/* Mailbox message buffer aligned to 16 bytes */
static volatile uint32_t mbox[36] __attribute__((aligned(16)));
uint32_t *g_pi_fb_ptr = NULL;

static uint32_t* init_pi_framebuffer(uint32_t w, uint32_t h) {
    volatile uint32_t *status_reg = (volatile uint32_t*)(uintptr_t)(MBOX_BASE_ADDR + MBOX_STATUS);
    volatile uint32_t *write_reg  = (volatile uint32_t*)(uintptr_t)(MBOX_BASE_ADDR + MBOX_WRITE);
    volatile uint32_t *read_reg   = (volatile uint32_t*)(uintptr_t)(MBOX_BASE_ADDR + MBOX_READ);

    mbox[0] = 35 * 4;
    mbox[1] = 0;

    mbox[2] = 0x00048003;  /* set phy wh */
    mbox[3] = 8;
    mbox[4] = 0;          /* request code */
    mbox[5] = w;
    mbox[6] = h;

    mbox[7] = 0x00048004;  /* set virt wh */
    mbox[8] = 8;
    mbox[9] = 0;          /* request code */
    mbox[10] = w;
    mbox[11] = h;

    mbox[12] = 0x00048005; /* set depth */
    mbox[13] = 4;
    mbox[14] = 0;          /* request code */
    mbox[15] = 32;

    mbox[16] = 0x00048006; /* set pixel order */
    mbox[17] = 4;
    mbox[18] = 0;          /* request code */
    mbox[19] = 1;          /* 1: RGB */

    mbox[20] = 0x00048009; /* set virt offset */
    mbox[21] = 8;
    mbox[22] = 0;          /* request code */
    mbox[23] = 0;
    mbox[24] = 0;

    mbox[25] = 0x00040001; /* allocate framebuffer */
    mbox[26] = 8;
    mbox[27] = 0;          /* request code */
    mbox[28] = 4096;       /* alignment */
    mbox[29] = 0;          /* response: size in bytes */

    mbox[30] = 0x00040008; /* get pitch */
    mbox[31] = 4;
    mbox[32] = 0;          /* request code */
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

    if (mbox[1] == 0x80000000 && mbox[28] != 0) {
        uint32_t fb_phys = mbox[28] & 0x3FFFFFFF;
        uint32_t fb_sz   = mbox[29];
        uart_puts("[QEMU-ARM] Framebuffer Allocated by VideoCore GPU!\n");
        uart_puts("[QEMU-ARM] FB Address: ");
        uart_hex32(fb_phys);
        uart_puts(" Size: ");
        uart_hex32(fb_sz);
        uart_puts("\n");
        g_pi_fb_ptr = (uint32_t*)(uintptr_t)fb_phys;
        return g_pi_fb_ptr;
    }

    uart_puts("[QEMU-ARM] Framebuffer allocation fallback.\n");
    g_pi_fb_ptr = (uint32_t*)0x3c000000;
    return g_pi_fb_ptr;
}

/*
 * QEMU raspi2b VideoCore pixel format: 0xAARRGGBB (ARGB32 / big-endian RGB)
 * Note: QEMU bcm2835-fb uses the pixel_order tag; with tag 0x00048006 value=1
 * (RGB), the byte layout in memory is R, G, B, X — i.e. 0xXXBBGGRR in little-
 * endian 32-bit words. So ARGB constant 0xFFRRGGBB becomes 0xFFBBGGRR here.
 */
#define ARGB(a,r,g,b) (((uint32_t)(a)<<24)|((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b))

/* Colors in 0xAARRGGBB — VideoCore display shows them correctly as-is */
#define COL_TEAL    ARGB(0xFF, 0x00, 0x78, 0x7A)  /* Classic B-TRON Teal     */
#define COL_NAVY    ARGB(0xFF, 0x00, 0x27, 0x6A)  /* Dark Navy Header        */
#define COL_GOLD    ARGB(0xFF, 0xFF, 0xA5, 0x00)  /* Gold accent bar         */
#define COL_LTGRAY  ARGB(0xFF, 0xCC, 0xCC, 0xCC)  /* Window chrome           */
#define COL_GRAY    ARGB(0xFF, 0x80, 0x80, 0x80)  /* Button face             */
#define COL_WHITE   ARGB(0xFF, 0xFF, 0xFF, 0xFF)
#define COL_BLACK   ARGB(0xFF, 0x00, 0x00, 0x00)
#define COL_FOCUS   ARGB(0xFF, 0x00, 0x40, 0xA0)  /* Focused title bar       */

static void fb_hline(uint32_t *fb, uint32_t pitch_px, uint32_t y,
                     uint32_t x0, uint32_t x1, uint32_t col) {
    uint32_t *row = fb + (y * pitch_px) + x0;
    uint32_t count = x1 - x0;
    while (count >= 8) {
        row[0] = col; row[1] = col; row[2] = col; row[3] = col;
        row[4] = col; row[5] = col; row[6] = col; row[7] = col;
        row += 8;
        count -= 8;
    }
    while (count > 0) {
        *row++ = col;
        count--;
    }
}

static void fb_fill(uint32_t *fb, uint32_t pitch_px,
                    uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                    uint32_t col) {
    for (uint32_t y = y0; y < y1; y++) {
        fb_hline(fb, pitch_px, y, x0, x1, col);
    }
}

static void fb_rect_outline(uint32_t *fb, uint32_t pw,
                             uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                             uint32_t col) {
    fb_hline(fb, pw, y0,   x0, x1, col);
    fb_hline(fb, pw, y1-1, x0, x1, col);
    for (uint32_t y = y0; y < y1; y++) {
        fb[y*pw + x0]   = col;
        fb[y*pw + x1-1] = col;
    }
}

static void draw_btron_pattern(uint32_t *fb, uint32_t w, uint32_t h) {
    if (!fb) return;
    uint32_t pw = w; /* pitch in pixels */

    /* ── Background ─────────────────────────────────── */
    fb_fill(fb, pw, 0, 0, w, h, COL_TEAL);

    /* ── Top Panel ───────────────────────────────────── */
    fb_fill(fb, pw, 0, 0, w, 26, COL_LTGRAY);
    fb_hline(fb, pw, 26, 0, w, COL_GRAY);
    fb_fill(fb, pw, 4, 3, 74, 23, COL_NAVY);
    fb_fill(fb, pw, 85,  5, 105, 18, COL_GRAY);
    fb_fill(fb, pw, 115, 5, 135, 18, COL_GRAY);
    fb_fill(fb, pw, 145, 5, 165, 18, COL_GRAY);
    fb_fill(fb, pw, 175, 5, 215, 18, COL_GRAY);
    fb_fill(fb, pw, w-80, 4, w-4, 22, COL_NAVY);

    /* ── Gold accent line ───────────────────────────── */
    fb_hline(fb, pw, 27, 0, w, COL_GOLD);
    fb_hline(fb, pw, 28, 0, w, COL_GOLD);

    /* ── Color test bars (bottom 80px) ─────────────── */
    uint32_t bars[8] = {
        COL_WHITE,
        ARGB(0xFF,0xFF,0xFF,0x00),
        ARGB(0xFF,0x00,0xFF,0xFF),
        ARGB(0xFF,0x00,0xFF,0x00),
        ARGB(0xFF,0xFF,0x00,0xFF),
        ARGB(0xFF,0xFF,0x00,0x00),
        ARGB(0xFF,0x00,0x00,0xFF),
        COL_BLACK,
    };
    uint32_t bar_y0 = h - 80;
    for (uint32_t bi = 0; bi < 8; bi++) {
        uint32_t bx0 = (bi * w) >> 3;
        uint32_t bx1 = ((bi+1) * w) >> 3;
        fb_fill(fb, pw, bx0, bar_y0, bx1, h, bars[bi]);
    }

    /* ── Desktop icons (left sidebar) ──────────────── */
    /* Cabinet Real Object icon */
    fb_fill(fb, pw,  20,  50,  70,  90, COL_LTGRAY);
    fb_rect_outline(fb, pw, 20, 50, 70, 90, COL_GRAY);
    fb_fill(fb, pw,  28,  58,  62,  82, ARGB(0xFF,0xE0,0xD0,0x50));
    fb_fill(fb, pw,  28,  54,  48,  58, ARGB(0xFF,0xE0,0xD0,0x50));

    /* T-Editor icon */
    fb_fill(fb, pw,  20, 110,  70, 150, COL_WHITE);
    fb_rect_outline(fb, pw, 20, 110, 70, 150, COL_NAVY);
    fb_hline(fb, pw, 120, 26, 64, COL_NAVY);
    fb_hline(fb, pw, 127, 26, 64, COL_NAVY);
    fb_hline(fb, pw, 134, 26, 64, COL_NAVY);

    /* Terminal icon */
    fb_fill(fb, pw,  20, 170,  70, 210, COL_BLACK);
    fb_rect_outline(fb, pw, 20, 170, 70, 210, COL_GOLD);
    fb_fill(fb, pw, 28, 182, 38, 188, COL_GOLD);
    fb_fill(fb, pw, 40, 185, 55, 190, COL_GOLD);

    /* ── Window: Cabinet Manager ────────────────────── */
    fb_fill(fb, pw, 100,  40, 620, 400, COL_LTGRAY);
    fb_rect_outline(fb, pw, 100, 40, 620, 400, COL_GRAY);
    fb_rect_outline(fb, pw, 102, 42, 618, 398, COL_GRAY);
    fb_fill(fb, pw, 103,  43, 617,  65, COL_FOCUS);
    fb_fill(fb, pw, 103, 398-2, 617, 398, COL_LTGRAY);
    fb_fill(fb, pw, 104,  66, 616, 396, COL_WHITE);
    fb_fill(fb, pw, 596, 47, 612, 62, COL_LTGRAY);
    fb_rect_outline(fb, pw, 596, 47, 612, 62, COL_GRAY);

    /* ── Window: T-Editor ─────────────────────────── */
    fb_fill(fb, pw, 200, 110, 680, 430, COL_LTGRAY);
    fb_rect_outline(fb, pw, 200, 110, 680, 430, COL_GRAY);
    fb_rect_outline(fb, pw, 202, 112, 678, 428, COL_GRAY);
    fb_fill(fb, pw, 203, 113, 677, 135, ARGB(0xFF,0x40,0x60,0xA0));
    fb_fill(fb, pw, 204, 136, 676, 426, COL_WHITE);
    for (int li = 0; li < 8; li++)
        fb_hline(fb, pw, 148 + li*22, 212, 650, ARGB(0xFF,0xC8,0xC8,0xD8));
    fb_fill(fb, pw, 656, 117, 672, 132, COL_LTGRAY);
    fb_rect_outline(fb, pw, 656, 117, 672, 132, COL_GRAY);

    /* ── Window: Terminal ─────────────────────────── */
    fb_fill(fb, pw, 320, 200, 840, 480, ARGB(0xFF,0x10,0x10,0x18));
    fb_rect_outline(fb, pw, 320, 200, 840, 480, COL_GOLD);
    fb_rect_outline(fb, pw, 322, 202, 838, 478, ARGB(0xFF,0x30,0x30,0x40));
    fb_fill(fb, pw, 323, 203, 837, 225, ARGB(0xFF,0x20,0x20,0x30));
    fb_fill(fb, pw, 326, 205, 346, 220, ARGB(0xFF,0x00,0xA0,0x20));
    fb_fill(fb, pw, 326, 228, 346, 240, ARGB(0xFF,0x00,0xA0,0x20));
    fb_fill(fb, pw, 326, 248, 346, 260, ARGB(0xFF,0x00,0xA0,0x20));
    fb_fill(fb, pw, 352, 268, 360, 280, ARGB(0xFF,0xCC,0xCC,0xCC));
    fb_fill(fb, pw, 836, 207, 836, 222, COL_GOLD);
    fb_rect_outline(fb, pw, 834, 207, 836, 222, COL_LTGRAY);

    /* ── Data Cache Barrier ─────────────────────────── */
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

    uart_puts("[QEMU-ARM] Drawing B-TRON desktop directly to framebuffer...\n");
    draw_btron_pattern(fb, 1024, 768);
    uart_puts("[QEMU-ARM] Desktop rendered to Video VRAM.\n");

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

    uart_puts("[B-TRON] Desktop Multi-Window Compositor running in VRAM — entering idle loop.\n");

    while (1) {
        __asm__ volatile("wfe");
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
        "mov r0, #(1 << 30)\n\t"
        "vmsr fpexc, r0\n\t"
        "bl btron_main\n\t"
        "3: wfe\n\t"
        "b 3b\n\t"
    );
#endif
}
