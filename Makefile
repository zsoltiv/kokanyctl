CC = cc
CXX = c++
CINC = `pkg-config --cflags sdl2 SDL2_net`
CXXINC = `pkg-config --cflags opencv4 libavformat libavcodec libavutil`
STDC = -std=c11
CFLAGS = $(CINC) -g -D_XOPEN_SOURCE=700 -O0
CXXFLAGS = $(CXXINC) $(CFLAGS)
LDFLAGS = `pkg-config --libs opencv4 sdl2 SDL2_net libavformat libavcodec libavutil`

BIN = kokanyctl

SRCDIR = src
BUILDDIR = build

SRC := $(wildcard $(SRCDIR)/*.c)
SRC += $(wildcard $(SRCDIR)/*.cpp)
OBJ += $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%.o,$(SRC))

.PHONY: all

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

$(BUILDDIR)/%.c.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $< -c $(STDC) $(CFLAGS) -o $@

$(BUILDDIR)/%.cpp.o: $(SRCDIR)/%.cpp
	mkdir -p $(BUILDDIR)
	$(CXX) $< -c $(CXXFLAGS) -o $@

clean:
	rm -rf $(BUILDDIR) $(BIN)
