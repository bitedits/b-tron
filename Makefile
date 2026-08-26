#
# B-TRON Retro OS Desktop Environment - Multi-Target Makefile
# Cleanroom implementation of BTRON Specification API & Sakamura T-Kernel / POSIX Backends
#

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -Iinclude -Iinclude/drivers -Isrc/kernel

# Detect OS & SDL2 flags
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || echo "-I/usr/local/include/SDL2")
    SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null || echo "-lSDL2") -lpthread -framework ApplicationServices -framework Cocoa
else ifeq ($(UNAME_S), Linux)
    SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/include/SDL2")
    SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null || pkg-config --libs sdl2 2>/dev/null || echo "-lSDL2") -lm -lpthread
else
    # Windows / MINGW
    SDL_CFLAGS := -I/usr/include/SDL2
    SDL_LIBS   := -lSDL2 -lgdi32 -lpthread
endif

# Common Sources
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

# POSIX Kernel Sources (Target 0: BTRON_POSIX)
POSIX_SRCS = src/kernel/core_posix.c \
             src/drivers/virtio.c \
             src/kernel/core_init.c \
             $(COMMON_SRCS)

# QEMU / VirtIO Sources (Target 1: BTRON_QEMU)
QEMU_SRCS = src/kernel/core_virtio.c \
            src/drivers/virtio.c \
            src/kernel/core_init.c \
            src/kernel/core_boot.c \
            $(COMMON_SRCS)

# Unmodified Sakamura T-Kernel 2.0 Engine Source Files in src/kernel (Target 2: BTRON_SAKAMURA)
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
                        src/kernel/version.c

TKERNEL_SRCS = src/kernel/core_tkernel.c \
               src/drivers/virtio.c \
               src/kernel/core_init.c \
               src/kernel/core_boot.c \
               $(TKERNEL_SAKAMURA_SRCS) \
               $(COMMON_SRCS)

POSIX_OBJS   = $(POSIX_SRCS:.c=.posix.o)
QEMU_OBJS    = $(QEMU_SRCS:.c=.qemu.o)
TKERNEL_OBJS = $(TKERNEL_SRCS:.c=.tkernel.o)

POSIX_TARGET   = btron-posix
QEMU_TARGET    = btron-qemu.elf
TKERNEL_TARGET = btron-tkernel.elf
DEFAULT_TARGET = btron

TKERNEL_INC = -D_RPI_BCM283x_ -DTYPE_RPI=1 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast \
              -Iinclude \
              -Iinclude/arch/pi4 \
              -Isrc/kernel

.PHONY: all posix qemu t-kernel clean run-posix run-qemu run-t-kernel

all: posix qemu t-kernel

posix: $(POSIX_TARGET)
	@ln -sf $(POSIX_TARGET) $(DEFAULT_TARGET)
	@echo "=========================================================="
	@echo " B-TRON POSIX Kernel & Desktop successfully built!"
	@echo " Run './btron' or 'make run-posix' to start the shell."
	@echo "=========================================================="

%.posix.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

%.qemu.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

# Rules for Sakamura T-Kernel core files in src/kernel
src/kernel/%.tkernel.o: src/kernel/%.c
	$(CC) $(CFLAGS) $(TKERNEL_INC) $(SDL_CFLAGS) -c $< -o $@

# Rule for general BTRON application sources under Sakamura target mode
%.tkernel.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(POSIX_OBJS): CFLAGS += -UBTRON_QEMU_TARGET -UBTRON_SAKAMURA_TARGET
$(QEMU_OBJS): CFLAGS += -DBTRON_QEMU_TARGET -UBTRON_SAKAMURA_TARGET
$(TKERNEL_OBJS): CFLAGS += -DBTRON_SAKAMURA_TARGET -UBTRON_QEMU_TARGET

$(POSIX_TARGET): $(POSIX_OBJS)
	$(CC) $(POSIX_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

qemu: $(QEMU_TARGET)
	@echo "=========================================================="
	@echo " B-TRON T-Kernel QEMU Image successfully built!"
	@echo " Output: $(QEMU_TARGET)"
	@echo " Run 'make run-qemu' to launch under QEMU."
	@echo "=========================================================="

$(QEMU_TARGET): $(QEMU_OBJS)
	$(CC) $(QEMU_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

t-kernel: $(TKERNEL_TARGET)
	@echo "=========================================================="
	@echo " Sakamura T-Kernel 2.0 Engine Image successfully built!"
	@echo " Output: $(TKERNEL_TARGET)"
	@echo " Run 'make run-t-kernel' to launch under QEMU."
	@echo "=========================================================="

$(TKERNEL_TARGET): $(TKERNEL_OBJS)
	$(CC) $(TKERNEL_OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS)

clean:
	rm -f $(POSIX_OBJS) $(QEMU_OBJS) $(TKERNEL_OBJS) $(POSIX_TARGET) $(QEMU_TARGET) $(TKERNEL_TARGET) $(DEFAULT_TARGET)
	find src -type f \( -name "*.o" -o -name "*.posix.o" -o -name "*.qemu.o" -o -name "*.tkernel.o" \) -delete 2>/dev/null || true

run-posix: posix
	./$(POSIX_TARGET)

run-qemu: qemu
	@echo "Running B-TRON T-Kernel QEMU VirtIO Environment..."
	@./$(QEMU_TARGET)

run-t-kernel: t-kernel
	@echo "Running Sakamura T-Kernel 2.0 Engine Environment..."
	@./$(TKERNEL_TARGET)
