#
# B-TRON Retro OS Desktop Environment - Plain BSD / TRON Makefile
# Cleanroom implementation of BTRON Specification API & SDL2 Desktop Shell
#

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -Iinclude

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

CFLAGS += $(SDL_CFLAGS)
LDFLAGS += $(SDL_LIBS)

TARGET = btron

SRCS = src/kernel/task.c \
       src/graphics/dp_core.c \
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

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "=========================================================="
	@echo " B-TRON Retro OS Desktop successfully built!"
	@echo " Run './btron' to start the interactive BTRON desktop shell."
	@echo "=========================================================="

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
