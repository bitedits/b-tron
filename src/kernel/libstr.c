#include <stdint.h>
/*
 * T-Kernel 2.0 Standard String & Memory Library: libstr.c
 */

#include <basic.h>
#include <libstr.h>
#include <stdarg.h>

__attribute__((weak)) void* tkl_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

__attribute__((weak)) void* tkl_memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

__attribute__((weak)) void* tkl_memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int tkl_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

size_t tkl_strlen(const char *s) {
    size_t len = 0;
    if (!s) return 0;
    while (s[len]) len++;
    return len;
}

char* tkl_strcpy(char *dst, const char *src) {
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

char* tkl_strncpy(char *dst, const char *src, size_t n) {
    char *ret = dst;
    while (n && (*dst++ = *src++)) n--;
    while (n--) *dst++ = '\0';
    return ret;
}

int tkl_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int tkl_strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* tkl_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == 0) ? (char *)s : NULL;
}

char* tkl_strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char *h = haystack, *n = needle;
            while (*h && *n && *h == *n) { h++; n++; }
            if (!*n) return (char *)haystack;
        }
    }
    return NULL;
}

/* Heap Memory Allocator (Icalloc / Ifree / Imalloc) */
static uint8_t s_heap_pool[1024 * 1024 * 8] __attribute__((aligned(16)));
static size_t  s_heap_offset = 0;

__attribute__((weak)) void* Imalloc(size_t size) {
    size = (size + 15) & ~15; /* 16-byte alignment */
    if (s_heap_offset + size > sizeof(s_heap_pool)) return NULL;
    void *ptr = &s_heap_pool[s_heap_offset];
    s_heap_offset += size;
    return ptr;
}

__attribute__((weak)) void* Icalloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *p = Imalloc(total);
    if (p) tkl_memset(p, 0, total);
    return p;
}

__attribute__((weak)) void Ifree(void *ptr) {
    (void)ptr;
}

/* Standard aliases */
void* memset(void *s, int c, size_t n) __attribute__((weak, alias("tkl_memset")));
void* memcpy(void *dst, const void *src, size_t n) __attribute__((weak, alias("tkl_memcpy")));
void* memmove(void *dst, const void *src, size_t n) __attribute__((weak, alias("tkl_memmove")));
int   memcmp(const void *s1, const void *s2, size_t n) __attribute__((alias("tkl_memcmp")));
size_t strlen(const char *s) __attribute__((alias("tkl_strlen")));
char*  strcpy(char *dst, const char *src) __attribute__((alias("tkl_strcpy")));
char*  strncpy(char *dst, const char *src, size_t n) __attribute__((alias("tkl_strncpy")));
int    strcmp(const char *s1, const char *s2) __attribute__((alias("tkl_strcmp")));
int    strncmp(const char *s1, const char *s2, size_t n) __attribute__((alias("tkl_strncmp")));

char* tkl_strcat(char *dst, const char *src) {
    char *ret = dst;
    while (*dst) dst++;
    while ((*dst++ = *src++));
    return ret;
}

char* tkl_strncat(char *dst, const char *src, size_t n) {
    char *ret = dst;
    while (*dst) dst++;
    while (n-- && *src) *dst++ = *src++;
    *dst = '\0';
    return ret;
}

char* strcat(char *dst, const char *src) __attribute__((alias("tkl_strcat")));
char* strncat(char *dst, const char *src, size_t n) __attribute__((alias("tkl_strncat")));

char*  strchr(const char *s, int c) __attribute__((alias("tkl_strchr")));
char*  strstr(const char *haystack, const char *needle) __attribute__((alias("tkl_strstr")));

/* Minimal standalone vsnprintf / snprintf */
__attribute__((weak)) int tkl_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    if (!str || size == 0) return 0;
    char *out = str;
    char *end = str + size - 1;

    while (*format && out < end) {
        if (*format != '%') {
            *out++ = *format++;
            continue;
        }
        format++;
        if (*format == '\0') break;

        /* Skip width / formatting flags like %-18s, %02d, %d, %s, %x, %p */
        int width = 0;
        int left_align = 0;
        if (*format == '-') { left_align = 1; format++; }
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }

        if (*format == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int slen = (int)tkl_strlen(s);
            while (*s && out < end) *out++ = *s++;
            if (left_align && width > slen) {
                for (int pad = 0; pad < width - slen && out < end; pad++) *out++ = ' ';
            }
        } else if (*format == 'd' || *format == 'i' || *format == 'u') {
            int val = va_arg(ap, int);
            char tmp[32];
            int p = 0;
            if (val == 0) tmp[p++] = '0';
            else {
                unsigned int uval = (unsigned int)val;
                if (*format != 'u' && val < 0) {
                    if (out < end) *out++ = '-';
                    uval = (unsigned int)(-val);
                }
                while (uval > 0) {
                    tmp[p++] = (char)('0' + (uval % 10));
                    uval /= 10;
                }
            }
            while (p > 0 && out < end) *out++ = tmp[--p];
        } else if (*format == 'x' || *format == 'X' || *format == 'p') {
            unsigned int val = va_arg(ap, unsigned int);
            char tmp[32];
            int p = 0;
            if (val == 0) tmp[p++] = '0';
            else {
                while (val > 0) {
                    int digit = val & 0xF;
                    tmp[p++] = (char)((digit < 10) ? ('0' + digit) : ('a' + digit - 10));
                    val >>= 4;
                }
            }
            while (p > 0 && out < end) *out++ = tmp[--p];
        } else if (*format == 'c') {
            char c = (char)va_arg(ap, int);
            *out++ = c;
        } else if (*format == '%') {
            *out++ = '%';
        }
        format++;
    }

    *out = '\0';
    return (int)(out - str);
}

__attribute__((weak)) int tkl_snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = tkl_vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *str, size_t size, const char *format, ...) __attribute__((weak, alias("tkl_snprintf")));
