CC ?= cc
STDC = -std=c17
INC = `pkg-config --cflags zbar sdl2 SDL2_ttf libavformat libavcodec libavutil`
CFLAGS += $(INC) -g3 -D_XOPEN_SOURCE=700 -Og $(STDC)
LDFLAGS += `pkg-config --libs zbar sdl2 SDL2_ttf libavformat libavcodec libavutil`

BIN ?= kokanyctl

SRCDIR = src
BUILDDIR = build

SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%.o,$(SRC))

.PHONY: clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

$(BUILDDIR)/%.c.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $< -c $(STDC) $(CFLAGS) -o $@

clean:
	rm -rf $(BUILDDIR) $(BIN)
