/*
 * test_q800.s - Quick test for QEMU Q800 boot
 */
.global _start
.text
_start:
    lea 0x5000c020, %a0

    /* Init Channel A: WR5 = 0x08 (Tx enable) | 0x02 (RTS) | 0x80 (DTR) = 0x8a */
    /* 5 reversed is 0xa0 */
    move.b #0xa0, 4(%a0)
    /* 0x8a (10001010) reversed is 01010001 = 0x51 */
    move.b #0x51, 4(%a0)

    /* Send 'B' 'T' 'R' 'O' 'N' to Channel A (offset 6) */
    move.b #0x42, 6(%a0)  /* B */
    move.b #0x2a, 6(%a0)  /* T */
    move.b #0x4a, 6(%a0)  /* R */
    move.b #0xf2, 6(%a0)  /* O */
    move.b #0x72, 6(%a0)  /* N */
    move.b #0x50, 6(%a0)  /* \n */

    /* Also draw a colored rectangle to macfb VRAM at 0xf9000000 */
    lea 0xf9000000, %a1
    move.l #(800*600-1), %d0
fill_loop:
    move.b #0x0f, (%a1)+  /* white/bright pixel */
    dbra %d0, fill_loop

halt_loop:
    stop #0x2000
    bra halt_loop
