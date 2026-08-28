#
# B-TRON Retro OS — Multi-Target Makefile
# Cleanroom Sakamura T-Kernel 2.0 / POSIX / QEMU / BCM283x Bare-Metal
#
# Targets:
#   posix         POSIX Microkernel Desktop (SDL2 host)
#   qemu          QEMU VirtIO Desktop (SDL2 host)
#   t-kernel      T-Kernel SDL2 host build (debug / development)
#   arm-elf       Freestanding ARM32 ELF  — BCM283x Pi 2B (Cortex-A7, ARMv7)
#   arm64-elf     Freestanding AArch64 ELF — Pi 4B only (Cortex-A72)
#   run-tkernel   Boot Pi 2B ELF in QEMU (raspi2b, with display)
#   test-tkernel  Boot Pi 2B ELF in QEMU (raspi2b, serial-only, headless)
#   debug-gdb     QEMU + GDB stub on Pi 2B
#
# NOTE: qemu-system-aarch64 supports ALL Pi models (raspi0/1ap/2b/3b/4b).
#       No separate qemu-system-arm binary needed.
#

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -Iinclude -Iinclude/drivers -Isrc/kernel

QEMU_ARM     ?= qemu-system-arm
QEMU_AARCH64 ?= qemu-system-aarch64

# ARM32: Cortex-A7 for Pi 2B (BCM2836)
ARM32_CC ?= clang --target=arm-none-eabi -mcpu=cortex-a7 -marm -fuse-ld=lld -ffreestanding -nostdlib
# AArch64: Cortex-A72 for Pi 4B (BCM2711) — kept for Pi4-only development
ARM64_CC ?= clang --target=aarch64-none-elf -mcpu=cortex-a72 -fuse-ld=lld -ffreestanding -nostdlib

# BCM283x bare-metal flags (TYPE_RPI=2 → BCM2836, Pi 2B, Cortex-A7)
BCM_INC      = -Iinclude -Iinclude/arch/bcm283x -Isrc/kernel
ARM_CFLAGS   = -O2 -Wall -Wextra -std=c99 \
               -D_RPI_BCM283x_ -DTYPE_RPI=2 -DBTRON_TARGET=2 -mfpu=vfpv4 -mfloat-abi=softfp \
               $(BCM_INC)
ARM64_CFLAGS = -O2 -Wall -Wextra -std=c99 \
               -D_RPI_BCM283x_ -DTYPE_RPI=3 -DBTRON_TARGET=2 \
               -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-unused-parameter \
               $(BCM_INC)

# Host OS / SDL2 detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    SDL_CFLAGS   := $(shell sdl2-config --cflags 2>/dev/null || echo "-I/usr/local/include/SDL2")
    SDL_LIBS     := $(shell sdl2-config --libs 2>/dev/null || echo "-lSDL2") \
                    -lpthread -framework ApplicationServices -framework Cocoa
    QEMU_DISPLAY := -display cocoa,show-cursor=on
else ifeq ($(UNAME_S), Linux)
    SDL_CFLAGS   := $(shell sdl2-config --cflags 2>/dev/null || pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/include/SDL2")
    SDL_LIBS     := $(shell sdl2-config --libs 2>/dev/null || pkg-config --libs sdl2 2>/dev/null || echo "-lSDL2") \
                    -lm -lpthread
    QEMU_DISPLAY := -display default,show-cursor=on
else
    SDL_CFLAGS   := -I/usr/include/SDL2
    SDL_LIBS     := -lSDL2 -lgdi32 -lpthread
    QEMU_DISPLAY := -display default,show-cursor=on
endif

# ── Text Input Primitives (TIP) / Mozc IME sources ────────────────
IME_SRCS    = src/ime/mozc_kkc.c       \
              src/ime/tip_ife.c        \
              src/ime/tip_task.c       \
              src/ime/tip_vobj.c

# ── Common SDL2-hosted app sources ────────────────────────────────
COMMON_SRCS = src/graphics/dp_core.c   \
              src/graphics/dp_sdl.c    \
              src/font/troncode.c      \
              src/font/jis_fonts.c     \
              src/window/wnd.c         \
              src/window/event.c       \
              src/vobject/vobj.c       \
              src/desktop/desktop.c    \
              src/desktop/main.c       \
              src/apps/vobj_manager.c  \
              src/apps/t_editor.c      \
              src/apps/gterm.c         \
              $(IME_SRCS)

# ── POSIX build (Target 0) ────────────────────────────────────────
POSIX_STARTUP = src/kernel/core_posix.c
POSIX_SRCS    = $(POSIX_STARTUP)        \
                src/drivers/virtio/virtio.c \
                src/kernel/core_init.c  \
                $(COMMON_SRCS)

# ── QEMU VirtIO build (Target 1) ─────────────────────────────────
QEMU_STARTUP = src/kernel/core_virtio.c
QEMU_SRCS    = $(QEMU_STARTUP)          \
               src/drivers/virtio/virtio.c \
               src/kernel/core_init.c   \
               src/kernel/core_boot.c   \
               $(COMMON_SRCS)

# ── BCM283x (Pi 2B) bare-metal arch sources ───────────────────────
ARCH_BCM_SRCS = src/drivers/bcm283x/cpu/cache.c      \
                src/drivers/bcm283x/cpu/chkplv.c     \
                src/drivers/bcm283x/cpu/cntwus.c     \
                src/drivers/bcm283x/cpu/cpu_calls.c  \
                src/drivers/bcm283x/cpu/cpu_init.c   \
                src/drivers/bcm283x/cpu/devinit.c    \
                src/drivers/bcm283x/cpu/patch.c      \
                src/drivers/bcm283x/cpu/power.c      \
                src/drivers/bcm283x/cpu/tkdev_init.c \
                src/drivers/bcm283x/screen/em1d512.c \
                src/drivers/bcm283x/screen/conf.c    \
                src/drivers/bcm283x/screen/common.c  \
                src/drivers/bcm283x/screen/main.c    \
                src/drivers/bcm283x/usb/dwc2.c

TKERNEL_SAKAMURA_SRCS = \
    src/kernel/task.c         \
    src/kernel/task_manage.c  \
    src/kernel/task_sync.c    \
    src/kernel/semaphore.c    \
    src/kernel/eventflag.c    \
    src/kernel/mailbox.c      \
    src/kernel/messagebuf.c   \
    src/kernel/rendezvous.c   \
    src/kernel/mutex.c        \
    src/kernel/mempool.c      \
    src/kernel/mempfix.c      \
    src/kernel/subsystem.c    \
    src/kernel/time_calls.c   \
    src/kernel/timer.c        \
    src/kernel/klock.c        \
    src/kernel/wait.c         \
    src/kernel/objname.c      \
    src/kernel/misc_calls.c   \
    src/kernel/version.c

TKERNEL_SRCS = src/kernel/core_tkernel.c \
               src/kernel/core_yoko.c \
               src/drivers/virtio/virtio.c \
               src/kernel/core_init.c     \
               src/kernel/core_boot.c     \
               $(TKERNEL_SAKAMURA_SRCS)   \
               $(COMMON_SRCS)

# Bare-metal: SDL-free subset only
COMMON_NO_SDL_SRCS = \
    src/graphics/dp_core.c \
    src/font/troncode.c    \
    src/font/jis_fonts.c   \
    src/window/wnd.c       \
    src/window/event.c     \
    src/vobject/vobj.c     \
    src/desktop/desktop.c  \
    src/apps/vobj_manager.c \
    src/apps/t_editor.c    \
    src/apps/gterm.c       \
    $(IME_SRCS)

BAREMETAL_STARTUP  = src/drivers/bcm283x/cpu/startup_arm.c
BAREMETAL_LD       = src/drivers/bcm283x/cpu/link.ld
ARM_BAREMETAL_SRCS = src/kernel/core_yoko.c $(TKERNEL_SAKAMURA_SRCS) $(ARCH_BCM_SRCS) $(BAREMETAL_STARTUP) $(COMMON_NO_SDL_SRCS)

# ── Object lists ─────────────────────────────────────────────────
POSIX_OBJS   = $(POSIX_SRCS:.c=.posix.o)
QEMU_OBJS    = $(QEMU_SRCS:.c=.qemu.o)
TKERNEL_OBJS = $(TKERNEL_SRCS:.c=.tkernel.o)
ARM32_OBJS   = $(ARM_BAREMETAL_SRCS:.c=.arm32.o)
ARM64_OBJS   = $(ARM_BAREMETAL_SRCS:.c=.arm64.o)
SAKAMURA_OBJS  = $(TKERNEL_SRCS:.c=.sakamura.o)

# ── Output names ──────────────────────────────────────────────────
POSIX_TARGET   = btron-posix
QEMU_TARGET    = btron-qemu.elf
TKERNEL_TARGET = btron-tkernel.elf
SAKAMURA_TARGET = btron-sakamura.elf
ARM32_TARGET   = btron-arm-baremetal.elf     # Pi 2B — BCM2836, Cortex-A7, ARMv7
ARM64_TARGET   = btron-aarch64-baremetal.elf # Pi 4B — BCM2711, Cortex-A72, AArch64
DEFAULT_TARGET = btron

TKERNEL_INC = -D_RPI_BCM283x_ -DTYPE_RPI=2 \
              -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast \
              -Iinclude -Iinclude/arch/bcm283x -Isrc/kernel

.PHONY: all posix qemu tkernel sakamura arm-elf arm64-elf \
        run-posix run-qemu run-tkernel test-tkernel \
        debug-virtio debug-gdb clean

all: posix qemu tkernel sakamura

# ═══════════════════════════════════════════════════════════════════
# POSIX Desktop
# ═══════════════════════════════════════════════════════════════════
posix: $(POSIX_TARGET)
	@ln -sf $(POSIX_TARGET) $(DEFAULT_TARGET)
	@echo "=========================================================="
	@echo " B-TRON POSIX Kernel & Desktop successfully built!"
	@echo " Startup File: $(POSIX_STARTUP)"
	@echo " Run './btron' or 'make run-posix' to start."
	@echo "=========================================================="

%.posix.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -DBTRON_TARGET=0 -c $< -o $@

$(POSIX_TARGET): $(POSIX_OBJS)
	$(CC) $(POSIX_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

run-posix: posix
	./$(POSIX_TARGET)

# ═══════════════════════════════════════════════════════════════════
# QEMU VirtIO Desktop
# ═══════════════════════════════════════════════════════════════════
qemu: $(QEMU_TARGET)
	@echo "=========================================================="
	@echo " B-TRON QEMU VirtIO Desktop built!"
	@echo " Run 'make run-qemu' to launch."
	@echo "=========================================================="

%.qemu.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -DBTRON_TARGET=1 -DBTRON_QEMU_TARGET -c $< -o $@

$(QEMU_TARGET): $(QEMU_OBJS)
	$(CC) $(QEMU_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

run-qemu: qemu
	@./$(QEMU_TARGET)

test-qemu: qemu
	@./$(QEMU_TARGET)

# ═══════════════════════════════════════════════════════════════════
# T-Kernel SDL2 host build (development / debug on host)
# ═══════════════════════════════════════════════════════════════════
tkernel: $(TKERNEL_TARGET)
	@echo "=========================================================="
	@echo " Sakamura T-Kernel 2.0 Engine built: $(TKERNEL_TARGET)"
	@echo "=========================================================="

src/kernel/%.tkernel.o: src/kernel/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -DBTRON_TARGET=2 -c $< -o $@

src/drivers/bcm283x/cpu/%.tkernel.o: src/drivers/bcm283x/cpu/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -DBTRON_TARGET=2 -c $< -o $@

src/drivers/bcm283x/screen/%.tkernel.o: src/drivers/bcm283x/screen/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -DBTRON_TARGET=2 -c $< -o $@

%.tkernel.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -DBTRON_TARGET=2 -c $< -o $@

$(TKERNEL_TARGET): $(TKERNEL_OBJS)
	$(CC) $(TKERNEL_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

# ===================================================================
sakamura: $(SAKAMURA_TARGET)
	@echo "=========================================================="
	@echo " Sakamura T-Kernel 2.0 Engine (UART/VirtIO Mode) built: $(SAKAMURA_TARGET)"
	@echo "=========================================================="

src/kernel/%.sakamura.o: src/kernel/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -DBTRON_TARGET=3 -c $< -o $@

src/drivers/bcm283x/cpu/%.sakamura.o: src/drivers/bcm283x/cpu/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -DBTRON_TARGET=3 -c $< -o $@

src/drivers/bcm283x/screen/%.sakamura.o: src/drivers/bcm283x/screen/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -DBTRON_TARGET=3 -c $< -o $@

%.sakamura.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -DBTRON_TARGET=3 -c $< -o $@

$(SAKAMURA_TARGET): $(SAKAMURA_OBJS)
	$(CC) $(SAKAMURA_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

# ═══════════════════════════════════════════════════════════════════
# Bare-Metal ARM32 ELF — BCM283x Pi 2B (Cortex-A7 / ARMv7 / BCM2836)
# ═══════════════════════════════════════════════════════════════════
arm-elf: $(ARM32_TARGET)

%.arm32.o: %.c
	$(ARM32_CC) $(ARM_CFLAGS) -c $< -o $@

$(ARM32_TARGET): $(ARM32_OBJS) $(BAREMETAL_LD)
	@echo "=========================================================="
	@echo " Building ARM32 ELF — BCM283x Pi 2B (Cortex-A7, ARMv7)"
	@echo " Startup: $(BAREMETAL_STARTUP)"
	@echo "=========================================================="
	$(ARM32_CC) $(ARM_CFLAGS) -Wl,-T,$(BAREMETAL_LD) $(ARM32_OBJS) -o $@
	@echo "[ARM-ELF] Built: $@"
	@file $@

# ═══════════════════════════════════════════════════════════════════
# Bare-Metal AArch64 ELF — Pi 4B (Cortex-A72 / BCM2711)
# ═══════════════════════════════════════════════════════════════════
arm64-elf: $(ARM64_TARGET)

%.arm64.o: %.c
	$(ARM64_CC) $(ARM64_CFLAGS) -c $< -o $@

$(ARM64_TARGET): $(ARM64_OBJS) $(BAREMETAL_LD)
	@echo "=========================================================="
	@echo " Building AArch64 ELF — Pi 4B (Cortex-A72, BCM2711)"
	@echo " Startup: $(BAREMETAL_STARTUP)"
	@echo "=========================================================="
	$(ARM64_CC) $(ARM64_CFLAGS) -Wl,-T,$(BAREMETAL_LD) $(ARM64_OBJS) -o $@
	@echo "[ARM64-ELF] Built: $@"
	@file $@

# ═══════════════════════════════════════════════════════════════════
# QEMU Pi 2B — run-tkernel / test-tkernel
#
# Runs B-TRON on Raspberry Pi 2B (BCM2836, Cortex-A7, ARMv7 32-bit).
# Supports both qemu-system-arm and qemu-system-aarch64.
# ═══════════════════════════════════════════════════════════════════
run-tkernel: arm-elf
	@echo "=========================================================="
	@echo " Launching T-Kernel on QEMU Raspberry Pi 2B (BCM2836)"
	@echo " Machine : raspi2b  |  CPU: Cortex-A7  |  RAM: 1G"
	@echo " ELF     : $(ARM32_TARGET)"
	@echo " Devices : USB Keyboard, USB Mouse & VideoCore GPU Display"
	@echo "=========================================================="
	@if command -v $(QEMU_ARM) >/dev/null 2>&1; then \
	    $(QEMU_ARM) -M raspi2b -m 1G $(QEMU_DISPLAY) \
	        -usb -device usb-kbd -device usb-mouse \
	        -kernel $(ARM32_TARGET) -serial stdio; \
	elif command -v $(QEMU_AARCH64) >/dev/null 2>&1; then \
	    $(QEMU_AARCH64) -M raspi2b -m 1G $(QEMU_DISPLAY) \
	        -usb -device usb-kbd -device usb-mouse \
	        -kernel $(ARM32_TARGET) -serial stdio; \
	else \
	    echo "[ERROR] QEMU not found — install qemu-system-arm or qemu-system-aarch64"; \
	    exit 1; \
	fi

test-tkernel: arm-elf
	@echo "=========================================================="
	@echo " Testing T-Kernel on QEMU Raspberry Pi 2B (BCM2836)"
	@echo " Machine : raspi2b  |  CPU: Cortex-A7  |  RAM: 1G"
	@echo " Mode    : headless (-display none), serial output only"
	@echo "=========================================================="
	@if command -v $(QEMU_ARM) >/dev/null 2>&1; then \
	    $(QEMU_ARM) -M raspi2b -m 1G -display none \
	        -kernel $(ARM32_TARGET) -serial stdio; \
	elif command -v $(QEMU_AARCH64) >/dev/null 2>&1; then \
	    $(QEMU_AARCH64) -M raspi2b -m 1G -display none \
	        -kernel $(ARM32_TARGET) -serial stdio; \
	else \
	    echo "[ERROR] QEMU not found — install qemu-system-arm or qemu-system-aarch64"; \
	    exit 1; \
	fi

# ═══════════════════════════════════════════════════════════════════
# Debug
# ═══════════════════════════════════════════════════════════════════
debug-virtio: arm64-elf
	$(QEMU_AARCH64) -M raspi4b -m 2G -trace "virtio_*" \
	    -kernel $(ARM64_TARGET) -serial stdio

# ═══════════════════════════════════════════════════════════════════
# Mozc Kana-Kanji Conversion & TIP Unit Test Suite
# ═══════════════════════════════════════════════════════════════════
TEST_MOZC_SRCS = src/ime/test_mozc.c src/ime/mozc_kkc.c src/ime/tip_ife.c \
                 src/font/troncode.c src/font/jis_fonts.c src/ime/tip_vobj.c src/window/wnd.c \
                 src/graphics/dp_core.c
TEST_MOZC_OBJS = $(TEST_MOZC_SRCS:.c=.test.o)
TEST_MOZC_BIN  = test_mozc

%.test.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test-mozc: $(TEST_MOZC_BIN)
	@echo "=========================================================="
	@echo " Running Mozc Kana-Kanji Conversion & TIP Unit Tests..."
	@echo "=========================================================="
	@./$(TEST_MOZC_BIN)

$(TEST_MOZC_BIN): $(TEST_MOZC_OBJS)
	$(CC) $(TEST_MOZC_OBJS) -o $@ $(LDFLAGS)

# ═══════════════════════════════════════════════════════════════════
# T-Editor UI & Internal Functions Test Suite
# ═══════════════════════════════════════════════════════════════════
TEST_EDITOR_SRCS = src/apps/test_editor_ui.c src/ime/mozc_kkc.c src/ime/tip_ife.c \
                   src/font/troncode.c src/font/jis_fonts.c src/ime/tip_vobj.c src/window/wnd.c \
                   src/graphics/dp_core.c
TEST_EDITOR_OBJS = $(TEST_EDITOR_SRCS:.c=.test.o)
TEST_EDITOR_BIN  = test_editor

test-editor: $(TEST_EDITOR_BIN)
	@echo "=========================================================="
	@echo " Running B-TRON T-Editor UI & Internal Functions Tests..."
	@echo "=========================================================="
	@./$(TEST_EDITOR_BIN)

$(TEST_EDITOR_BIN): $(TEST_EDITOR_OBJS)
	$(CC) $(TEST_EDITOR_OBJS) -o $@ $(LDFLAGS)

# ═══════════════════════════════════════════════════════════════════
# Clean
# ═══════════════════════════════════════════════════════════════════
clean:
	rm -f *.toc
	rm -f *.aux
	rm -f *.log
	rm -f *.out
	rm -f $(POSIX_TARGET) $(QEMU_TARGET) $(TKERNEL_TARGET) $(SAKAMURA_TARGET) \
	      $(ARM32_TARGET) $(ARM64_TARGET) $(DEFAULT_TARGET) $(TEST_MOZC_BIN) $(TEST_EDITOR_BIN)
	find src -type f \( -name "*.posix.o" -o -name "*.qemu.o" \
	    -o -name "*.tkernel.o" -o -name "*.sakamura.o" -o -name "*.arm32.o" \
	    -o -name "*.arm64.o" -o -name "*.test.o" -o -name "*.o" \) -delete 2>/dev/null || true
