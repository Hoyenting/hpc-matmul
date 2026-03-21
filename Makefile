CC ?= cc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -O2
ASMFLAGS ?= -O3 -march=native -S
LDFLAGS ?=
LDLIBS ?=

TARGET ?= matmul
SRC ?= main.c src/matmul_naive.c src/matmul_naive_rowmajor.c src/matmul_block.c
OBJ := $(SRC:.c=.o)
ASM_SRC ?= src/matmul_block.c
ASM_OUT ?= $(ASM_SRC:.c=.s)
ASM_OBJ := $(SRC:.c=.s)
ARGS ?=

.PHONY: all run debug release asm asm-all clean help

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

debug: CFLAGS += -O0 -g
debug: clean all

release: CFLAGS += -O3 -DNDEBUG
release: clean all

asm:
	$(CC) $(CPPFLAGS) $(ASMFLAGS) $(ASM_SRC) -o $(ASM_OUT)

asm-all: $(ASM_OBJ)

%.s: %.c
	$(CC) $(CPPFLAGS) $(ASMFLAGS) $< -o $@

clean:
	rm -f $(OBJ) $(ASM_OBJ) $(TARGET)

help:
	@echo "Usage:"
	@echo "  make                         Build ($(TARGET))"
	@echo "  make run ARGS='...'         Build and run with arguments"
	@echo "  make debug                  Rebuild with debug flags"
	@echo "  make release                Rebuild with release flags"
	@echo "  make asm ASM_SRC=foo.c      Generate assembly for one source file"
	@echo "  make asm-all                Generate assembly for all current SRC files"
	@echo "  make clean                  Remove build artifacts"
	@echo ""
	@echo "Override defaults if needed:"
	@echo "  make TARGET=my_app SRC=foo.c"
	@echo "  make asm ASM_SRC=src/matmul_block.c ASM_OUT=block.s"
	@echo "  make LDLIBS='-lm'"
