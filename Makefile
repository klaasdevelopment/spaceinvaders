CC ?= cc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -std=c99 -Wall -Wextra -O2

TARGET := test
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)
DEPS := $(OBJECTS:.o=.d)

# Use installed raylib metadata, with standard Linux flags as a fallback.
# Override RAYLIB_CFLAGS and RAYLIB_LIBS for a custom installation.
RAYLIB_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags raylib 2>/dev/null)
RAYLIB_LIBS ?= $(shell $(PKG_CONFIG) --libs raylib 2>/dev/null || echo -lraylib -lGL -lm -lpthread -ldl -lrt -lX11)

.PHONY: all run check clean
.DELETE_ON_ERROR:

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(RAYLIB_LIBS) $(LDLIBS) -lm

%.o: %.c Makefile
	$(CC) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

check:
	mkdir -p build
	$(CC) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CFLAGS) $(LDFLAGS) tests/game_test.c -o build/game_test $(RAYLIB_LIBS) $(LDLIBS) -lm
	./build/game_test

clean:
	$(RM) $(TARGET) $(OBJECTS) $(DEPS)
	$(RM) build/game_test

-include $(DEPS)
