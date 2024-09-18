# Compiler
CC := gcc

# Compiler flags
CFLAGS := -Wall -Wextra -O2
CFLAGS += $(EXTRA_CFLAGS)

# Linker flags
LDFLAGS := -shared
LDFLAGS += $(EXTRA_LDFLAGS)

# Library flags for demo
LIBS := -lm -lGLESv2 -lglfw
LIBS += $(EXTRA_LIBS)

# Source files
NVG_SRC := nvg.c
DEMO_SRC := demo.c

# Output files
NVG_LIB := libnvg.so
DEMO := demo

# Default target
all: $(NVG_LIB) $(DEMO)

# Rule to build the shared library
$(NVG_LIB): $(NVG_SRC)
	$(CC) $(CFLAGS) -fPIC $(LDFLAGS) -o $@ $< -fuse-ld=mold

# Rule to build the demo executable
$(DEMO): $(DEMO_SRC) $(NVG_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -lnvg $(LIBS) -Wl,-rpath,'$$ORIGIN' -fuse-ld=mold

# Clean target
clean:
	rm -f $(NVG_LIB) $(DEMO)

# Phony targets
.PHONY: all clean