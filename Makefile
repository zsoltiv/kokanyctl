CC = cc
STDC = -std=c11
INC = `pkg-config --cflags zbar sdl2 SDL2_ttf libavformat libavcodec libavutil`
CFLAGS = $(INC) -g -D_XOPEN_SOURCE=700 -O0
LDFLAGS = `pkg-config --libs zbar sdl2 SDL2_ttf libavformat libavcodec libavutil`

BIN = kokanyctl

SRCDIR = src
BUILDDIR = build

SRC := $(wildcard $(SRCDIR)/*.c)
OBJ += $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%.o,$(SRC))

.PHONY: all

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

$(BUILDDIR)/%.c.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $< -c $(STDC) $(CFLAGS) -o $@

clean:
	rm -rf $(BUILDDIR) $(BIN)
