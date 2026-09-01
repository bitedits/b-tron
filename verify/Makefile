#
# BTRON 3.20 Conformance Verifier Makefile
#

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -I../include -I../include/drivers -I../src/kernel -D_GNU_SOURCE
LDFLAGS ?= -lpthread -lm

VERIFY_SRCS = btron_verify.c \
              report/results.c \
              suite/v_types.c \
              suite/v_errors.c \
              suite/v_itron.c \
              suite/v_memory.c \
              suite/v_message.c \
              suite/v_vobj.c \
              suite/v_dp.c \
              suite/v_wnd.c \
              suite/v_hmi.c \
              ../src/kernel/core_posix.c \
              ../src/graphics/dp_core.c \
              ../src/window/wnd.c \
              ../src/vobject/vobj.c \
              ../src/hmi/hmi_core.c \
              ../src/hmi/hmi_switch.c \
              ../src/hmi/hmi_selector.c \
              ../src/hmi/hmi_volume.c \
              ../src/hmi/hmi_meter.c \
              ../src/hmi/hmi_controller.c \
              ../src/hmi/hmi_panel.c \
              ../src/font/troncode.c \
              ../src/font/jis_fonts.c

VERIFY_OBJS = $(VERIFY_SRCS:.c=.o)
VERIFY_BIN = btron_verify

.PHONY: all clean run

all: $(VERIFY_BIN)

$(VERIFY_BIN): $(VERIFY_OBJS)
	$(CC) $(VERIFY_OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(VERIFY_BIN)
	./$(VERIFY_BIN)

clean:
	rm -f $(VERIFY_BIN) $(VERIFY_OBJS)
