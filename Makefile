CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS ?=
LDLIBS ?=

TARGET ?= matmul
SRC ?= main.c src/matmul_naive.c src/matmul_naive_rowmajor.c src/matmul_block.c
OBJ := $(SRC:.c=.o)
ARGS ?=

.PHONY: all run debug release clean help

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

debug: CFLAGS = -std=c11 -Wall -Wextra -O0 -g
debug: clean all

release: CFLAGS = -std=c11 -Wall -Wextra -O3 -DNDEBUG
release: clean all

clean:
	rm -f $(OBJ) $(TARGET)

help:
	@echo "Usage:"
	@echo "  make                         Build ($(TARGET))"
	@echo "  make run ARGS='...'         Build and run with arguments"
	@echo "  make debug                  Rebuild with debug flags"
	@echo "  make release                Rebuild with release flags"
	@echo "  make clean                  Remove build artifacts"
	@echo ""
	@echo "Override defaults if needed:"
	@echo "  make TARGET=my_app SRC=foo.c"
	@echo "  make LDLIBS='-lm'"
