/*
 * ps2_sio.h — Sony PlayStation 2 Cleanroom Debug Serial / Console Output
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#ifndef PS2_SIO_H
#define PS2_SIO_H

#include <stddef.h>
#include <stdint.h>

#include <stdint.h>
#include <stddef.h>

void ps2_sio_init(void);
void ps2_sio_putc(char c);
void ps2_sio_puts(const char *str);
int  ps2_sio_getc(void);
int  ps2_sio_has_char(void);

#endif /* PS2_SIO_H */
