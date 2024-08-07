CC ?= cc
INC = `pkg-config --cflags zbar sdl2 SDL2_ttf libavformat libavcodec libavutil`
CFLAGS ?= -g3 -D_XOPEN_SOURCE=700 -Og
ALLCFLAGS = $(CFLAGS) $(STDC) -D_XOPEN_SOURCE=700 -std=c17 $(INC)
LDFLAGS = `pkg-config --libs zbar sdl2 SDL2_ttf libavformat libavcodec libavutil`

BIN ?= kokanyctl

SRCDIR = src
BUILDDIR = build

SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%.o,$(SRC))

.PHONY: clean rules

all: $(BIN) yolo/model.onnx

$(BIN): $(OBJ)
	$(CC) $^ $(INC) $(ALLCFLAGS) $(LDFLAGS) -o $@

$(BUILDDIR)/%.c.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $< -c $(INC) $(ALLCFLAGS) -o $@

yolo/model.onnx: yolo/model.pt
	ultralytics export model=$< format=onnx $(UFLAGS) opset=12

rules:
	install -m 660 rules/* /etc/udev/rules.d/

clean:
	rm -rf $(BUILDDIR) $(BIN)
