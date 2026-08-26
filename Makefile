#
# B-TRON Retro OS Desktop Environment - Multi-Target Makefile
# Cleanroom implementation of BTRON Specification API & Sakamura T-Kernel / POSIX Backends
#
# 3 Target Startup Build Files:
#   1. POSIX Microkernel Simulator:       src/kernel/core_posix.c
#   2. QEMU VirtIO Desktop Environment:  src/kernel/core_virtio.c
#   3. Sakamura T-Kernel Bare-Metal ARM:  src/arch/pi4/startup_arm.c
#

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -Iinclude -Iinclude/drivers -Isrc/kernel

QEMU_ARM     ?= qemu-system-arm
QEMU_AARCH64 ?= qemu-system-aarch64

ARM32_CC     ?= clang --target=arm-none-eabi -mcpu=cortex-a15 -fuse-ld=lld -ffreestanding -nostdlib
ARM64_CC     ?= clang --target=aarch64-none-elf -mcpu=cortex-a72 -mgeneral-regs-only -fuse-ld=lld -ffreestanding -nostdlib
ARM_CFLAGS   ?= -O2 -Wall -Wextra -std=c99 -D_RPI_BCM283x_ -DTYPE_RPI=1 -DBTRON_TARGET=2 -Iinclude -Iinclude/arch/pi4 -Isrc/kernel

# Detect OS & SDL2 flags
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || echo "-I/usr/local/include/SDL2")
    SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null || echo "-lSDL2") -lpthread -framework ApplicationServices -framework Cocoa
    QEMU_DISPLAY := -display cocoa
else ifeq ($(UNAME_S), Linux)
    SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/include/SDL2")
    SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null || pkg-config --libs sdl2 2>/dev/null || echo "-lSDL2") -lm -lpthread
    QEMU_DISPLAY := -display default
else
    # Windows / MINGW
    SDL_CFLAGS := -I/usr/include/SDL2
    SDL_LIBS   := -lSDL2 -lgdi32 -lpthread
    QEMU_DISPLAY := -display default
endif

# Common Desktop Application Sources
COMMON_SRCS = src/graphics/dp_core.c \
              src/graphics/dp_sdl.c \
              src/font/troncode.c \
              src/window/wnd.c \
              src/window/event.c \
              src/vobject/vobj.c \
              src/desktop/desktop.c \
              src/desktop/main.c \
              src/apps/vobj_manager.c \
              src/apps/t_editor.c \
              src/apps/gterm.c

# BUILD 1: POSIX Kernel Sources (Target 0: BTRON_POSIX)
POSIX_STARTUP = src/kernel/core_posix.c
POSIX_SRCS    = $(POSIX_STARTUP) \
                src/drivers/virtio.c \
                src/kernel/core_init.c \
                $(COMMON_SRCS)

# BUILD 2: QEMU VirtIO Desktop Sources (Target 1: BTRON_QEMU)
QEMU_STARTUP = src/kernel/core_virtio.c
QEMU_SRCS    = $(QEMU_STARTUP) \
               src/drivers/virtio.c \
               src/kernel/core_init.c \
               src/kernel/core_boot.c \
               $(COMMON_SRCS)

# BUILD 3: Sakamura Pi4 Hardware Architecture & Bare-Metal Sources (Target 2: BTRON_SAKAMURA)
ARCH_PI4_SRCS = src/arch/pi4/cache.c \
                src/arch/pi4/chkplv.c \
                src/arch/pi4/cntwus.c \
                src/arch/pi4/cpu_calls.c \
                src/arch/pi4/cpu_init.c \
                src/arch/pi4/devinit.c \
                src/arch/pi4/patch.c \
                src/arch/pi4/power.c \
                src/arch/pi4/tkdev_init.c

TKERNEL_SAKAMURA_SRCS = src/kernel/task.c \
                        src/kernel/task_manage.c \
                        src/kernel/task_sync.c \
                        src/kernel/semaphore.c \
                        src/kernel/eventflag.c \
                        src/kernel/mailbox.c \
                        src/kernel/messagebuf.c \
                        src/kernel/rendezvous.c \
                        src/kernel/mutex.c \
                        src/kernel/mempool.c \
                        src/kernel/mempfix.c \
                        src/kernel/subsystem.c \
                        src/kernel/time_calls.c \
                        src/kernel/timer.c \
                        src/kernel/klock.c \
                        src/kernel/wait.c \
                        src/kernel/objname.c \
                        src/kernel/misc_calls.c \
                        src/kernel/version.c \
                        $(ARCH_PI4_SRCS)

TKERNEL_SRCS = src/kernel/core_tkernel.c \
               src/drivers/virtio.c \
               src/kernel/core_init.c \
               src/kernel/core_boot.c \
               $(TKERNEL_SAKAMURA_SRCS) \
               $(COMMON_SRCS)

COMMON_NO_SDL_SRCS = src/graphics/dp_core.c \
                     src/font/troncode.c \
                     src/window/wnd.c \
                     src/window/event.c \
                     src/vobject/vobj.c \
                     src/desktop/desktop.c

BAREMETAL_STARTUP  = src/arch/pi4/startup_arm.c
ARM_BAREMETAL_SRCS = $(TKERNEL_SAKAMURA_SRCS) $(BAREMETAL_STARTUP) $(COMMON_NO_SDL_SRCS)

POSIX_OBJS   = $(POSIX_SRCS:.c=.posix.o)
QEMU_OBJS    = $(QEMU_SRCS:.c=.qemu.o)
TKERNEL_OBJS = $(TKERNEL_SRCS:.c=.tkernel.o)
ARM32_OBJS   = $(ARM_BAREMETAL_SRCS:.c=.arm32.o)
ARM64_OBJS   = $(ARM_BAREMETAL_SRCS:.c=.arm64.o)

POSIX_TARGET     = btron-posix
QEMU_TARGET      = btron-qemu.elf
TKERNEL_TARGET   = btron-tkernel.elf
ARM32_TARGET     = btron-arm-baremetal.elf
ARM64_TARGET     = btron-aarch64-baremetal.elf
DEFAULT_TARGET   = btron

TKERNEL_INC = -D_RPI_BCM283x_ -DTYPE_RPI=1 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast \
              -Iinclude \
              -Iinclude/arch/pi4 \
              -Isrc/kernel

.PHONY: all posix qemu t-kernel clean run-posix run-qemu run-t-kernel test-qemu test-t-kernel arm-elf arm64-elf debug-virtio debug-gdb

all: posix

posix: $(POSIX_TARGET)
	@ln -sf $(POSIX_TARGET) $(DEFAULT_TARGET)
	@echo "=========================================================="
	@echo " B-TRON POSIX Kernel & Desktop successfully built!"
	@echo " Startup File: $(POSIX_STARTUP)"
	@echo " Run './btron' or 'make run-posix' to start."
	@echo "=========================================================="

%.posix.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

%.qemu.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

src/kernel/%.tkernel.o: src/kernel/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -c $< -o $@

src/arch/pi4/%.tkernel.o: src/arch/pi4/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -c $< -o $@

%.tkernel.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

%.arm32.o: %.c
	$(ARM32_CC) $(ARM_CFLAGS) -c $< -o $@

%.arm64.o: %.c
	$(ARM64_CC) $(ARM_CFLAGS) -c $< -o $@

$(POSIX_OBJS): CFLAGS += -DBTRON_TARGET=0 -UBTRON_QEMU_TARGET -UBTRON_SAKAMURA_TARGET
$(QEMU_OBJS): CFLAGS += -DBTRON_TARGET=1 -DBTRON_QEMU_TARGET -UBTRON_SAKAMURA_TARGET
$(TKERNEL_OBJS): CFLAGS += -DBTRON_TARGET=2 -DBTRON_SAKAMURA_TARGET -UBTRON_QEMU_TARGET

$(POSIX_TARGET): $(POSIX_OBJS)
	$(CC) $(POSIX_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

qemu: $(QEMU_TARGET)
	@echo "=========================================================="
	@echo " B-TRON T-Kernel QEMU Image successfully built!"
	@echo " Startup File: $(QEMU_STARTUP)"
	@echo " Output: $(QEMU_TARGET)"
	@echo " Run 'make run-qemu' to launch under QEMU."
	@echo "=========================================================="

$(QEMU_TARGET): $(QEMU_OBJS)
	$(CC) $(QEMU_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

t-kernel: $(TKERNEL_TARGET)
	@echo "=========================================================="
	@echo " Sakamura T-Kernel 2.0 Engine Image successfully built!"
	@echo " Output: $(TKERNEL_TARGET)"
	@echo " Run 'make run-t-kernel' to launch."
	@echo "=========================================================="

$(TKERNEL_TARGET): $(TKERNEL_OBJS)
	$(CC) $(TKERNEL_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

arm-elf: $(ARM32_TARGET)

$(ARM32_TARGET): $(ARM32_OBJS) src/arch/pi4/link.ld
	@echo "=========================================================="
	@echo " Building Freestanding ARM32 Bare-Metal ELF Image for QEMU"
	@echo " Startup File: $(BAREMETAL_STARTUP)"
	@echo " Target Architecture: ARM EABI5 (32-bit Bare-Metal)"
	@echo "=========================================================="
	$(ARM32_CC) $(ARM_CFLAGS) -Wl,-T,src/arch/pi4/link.ld $(ARM32_OBJS) -o $@
	@echo "[ARM-ELF] Built freestanding 32-bit ARM ELF image: $@"
	@file $@

arm64-elf: $(ARM64_TARGET)

$(ARM64_TARGET): $(ARM64_OBJS) src/arch/pi4/link.ld
	@echo "=========================================================="
	@echo " Building Freestanding AArch64 64-Bit Bare-Metal ELF Image for QEMU"
	@echo " Startup File: $(BAREMETAL_STARTUP)"
	@echo " Target Architecture: AArch64 (64-bit Bare-Metal)"
	@echo "=========================================================="
	$(ARM64_CC) $(ARM_CFLAGS) -Wl,-T,src/arch/pi4/link.ld $(ARM64_OBJS) -o $@
	@echo "[ARM64-ELF] Built freestanding 64-bit AArch64 ELF image: $@"
	@file $@

clean:
	rm -f $(POSIX_OBJS) $(QEMU_OBJS) $(TKERNEL_OBJS) $(POSIX_TARGET) $(QEMU_TARGET) $(TKERNEL_TARGET) $(ARM32_TARGET) $(ARM64_TARGET) $(DEFAULT_TARGET)
	find src -type f \( -name "*.o" -o -name "*.posix.o" -o -name "*.qemu.o" -o -name "*.tkernel.o" -o -name "*.arm32.o" -o -name "*.arm64.o" \) -delete 2>/dev/null || true

run-posix: posix
	./$(POSIX_TARGET)

run-qemu: qemu
	@echo "Running B-TRON T-Kernel QEMU VirtIO Environment..."
	@./$(QEMU_TARGET)

run-t-kernel: t-kernel
	@echo "Running Sakamura T-Kernel 2.0 Engine Environment..."
	@./$(TKERNEL_TARGET)

test-qemu: qemu
	@echo "=========================================================="
	@echo " Testing B-TRON VirtIO Mode Desktop Environment"
	@echo " Startup File: $(QEMU_STARTUP)"
	@echo "=========================================================="
	@./$(QEMU_TARGET)

test-t-kernel: arm64-elf
	@echo "=========================================================="
	@echo " Testing Freestanding Bare-Metal Sakamura T-Kernel ELF with QEMU"
	@echo " Startup File: $(BAREMETAL_STARTUP)"
	@echo " Machine Target: Raspberry Pi 4B Board (-M raspi4b) Video & Console"
	@echo "=========================================================="
	@if command -v $(QEMU_AARCH64) >/dev/null 2>&1; then \
		echo "[TEST-T-KERNEL] Running $(QEMU_AARCH64) -M raspi4b $(QEMU_DISPLAY)..."; \
		$(QEMU_AARCH64) -M raspi4b -m 2G $(QEMU_DISPLAY) -kernel $(ARM64_TARGET) -serial stdio || true; \
	elif command -v $(QEMU_ARM) >/dev/null 2>&1; then \
		echo "[TEST-T-KERNEL] Running $(QEMU_ARM) -M raspi2b $(QEMU_DISPLAY)..."; \
		$(QEMU_ARM) -M raspi2b -m 1G $(QEMU_DISPLAY) -kernel $(ARM32_TARGET) -serial stdio || true; \
	fi

debug-virtio: arm64-elf
	@echo "=========================================================="
	@echo " Launching QEMU VirtIO Event Tracing Engine on Raspberry Pi 4B (-M raspi4b)"
	@echo "=========================================================="
	$(QEMU_AARCH64) -M raspi4b -m 2G -trace "virtio_*" -kernel $(ARM64_TARGET) -serial stdio

debug-gdb: arm64-elf
	@echo "=========================================================="
	@echo " Launching QEMU GDB Remote Debug Server (localhost:1234)"
	@echo " Run 'gdb-multiarch $(ARM64_TARGET)' and 'target remote localhost:1234'"
	@echo "=========================================================="
	$(QEMU_AARCH64) -M raspi4b -m 2G $(QEMU_DISPLAY) -kernel $(ARM64_TARGET) -serial stdio -s -S
