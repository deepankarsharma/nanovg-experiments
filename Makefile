# Compiler
CC := gcc

# Compiler flags
CFLAGS := -Wall -Wextra -O2
CFLAGS += $(EXTRA_CFLAGS)

# Linker flags
LDFLAGS := -shared
LDFLAGS += $(EXTRA_LDFLAGS)

# Library flags for demo
LIBS := -lm -lGLESv2 -lglfw3
LIBS += $(EXTRA_LIBS)

# Source files
NVG_SRC := nvg.c
DEMO_SRC := demo.c

# Output files
NVG_LIB := libnvg.so
DEMO := demo

SDL_SRC := sdl.c
SDL := sdl
SDL_LIBS := -lm -lGLESv2 -lSDL2
SDL_LIBS += $(EXTRA_LIBS)

# Default target
all: $(NVG_LIB) $(DEMO) $(SDL)

# Rule to build the shared library
$(NVG_LIB): $(NVG_SRC)
	$(CC) $(CFLAGS) -fPIC $(LDFLAGS) -o $@ $< -fuse-ld=mold

# Rule to build the demo executable
$(DEMO): $(DEMO_SRC) $(NVG_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -L/usr/local/lib -lnvg $(LIBS) -Wl,-rpath,'$$ORIGIN' -fuse-ld=mold

$(SDL): $(SDL_SRC) $(NVG_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -L/usr/local/lib -lnvg $(SDL_LIBS) -Wl,-rpath,'$$ORIGIN' -fuse-ld=mold

# Clean target
clean:
	rm -f $(NVG_LIB) $(DEMO)

# Phony targets
.PHONY: all clean
