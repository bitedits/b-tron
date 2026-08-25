#
# B-TRON Retro OS Desktop Environment - Multi-Target Makefile
# Cleanroom implementation of BTRON Specification API & T-Kernel / POSIX Backends
#

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -Iinclude -Isrc/kernel

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
              apps/vobj_manager.c \
              apps/t_editor.c \
              apps/gterm.c

# POSIX Kernel Sources
POSIX_SRCS = src/kernel/task.c \
             src/kernel/posix_kernel.c \
             src/kernel/virtio.c \
             src/kernel/kernel_init.c \
             $(COMMON_SRCS)

# QEMU / T-Kernel Sources
QEMU_SRCS = src/kernel/tkernel_core.c \
            src/kernel/virtio.c \
            src/kernel/kernel_init.c \
            src/kernel/multiboot.c \
            $(COMMON_SRCS)

POSIX_OBJS = $(POSIX_SRCS:.c=.posix.o)
QEMU_OBJS  = $(QEMU_SRCS:.c=.qemu.o)

# Additional .o targets for clean pattern matching
GEN_OBJS = $(POSIX_SRCS:.c=.o) $(QEMU_SRCS:.c=.o)

POSIX_TARGET = btron-posix
QEMU_TARGET  = btron-qemu.elf
DEFAULT_TARGET = btron

.PHONY: all posix qemu clean run-posix run-qemu

all: posix

posix: $(POSIX_TARGET)
	@ln -sf $(POSIX_TARGET) $(DEFAULT_TARGET)
	@echo "=========================================================="
	@echo " B-TRON POSIX Kernel & Desktop successfully built!"
	@echo " Run './btron' or 'make run-posix' to start the shell."
	@echo "=========================================================="

%.posix.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

%.qemu.o: %.c
	$(CC) $(CFLAGS) -DBTRON_QEMU_TARGET $(SDL_CFLAGS) -c $< -o $@

$(POSIX_OBJS): CFLAGS += -UBTRON_QEMU_TARGET
$(QEMU_OBJS): CFLAGS += -DBTRON_QEMU_TARGET

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

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(POSIX_OBJS) $(QEMU_OBJS) $(GEN_OBJS) $(POSIX_TARGET) $(QEMU_TARGET) $(DEFAULT_TARGET)
	rm -f src/desktop/*.o src/font/*.o src/graphics/*.o src/kernel/*.o src/vobject/*.o src/window/*.o apps/*.o

run-posix: posix
	./$(POSIX_TARGET)

run-qemu: qemu
	@echo "Running B-TRON T-Kernel QEMU VirtIO Environment..."
	@./$(QEMU_TARGET)
