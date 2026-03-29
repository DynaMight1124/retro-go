# Makefile for Celeste ESP32 prototype (SDL2 on Mac)

CC = clang
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = $(shell sdl2-config --cflags --libs)

TARGET = celeste
SRCS = main.c gfx_sdl.c level.c vfx.c player.c
OBJS = $(SRCS:.c=.o)
HDRS = gfx.h level.h vfx.h player.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) $(shell sdl2-config --cflags) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
