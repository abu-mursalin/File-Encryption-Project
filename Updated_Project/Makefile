#==============================================================
# Makefile – File Encryption Project
# Compiler : gcc
# Requires : GTK4, OpenSSL (libssl / libcrypto)
#
# Build targets:
#   make          – compile everything → ./file_encryptor
#   make clean    – remove object files and the binary
#   make run      – build and launch the application
#==============================================================

# ── Toolchain ──────────────────────────────────────────────
CC      := gcc
BINARY  := file_encryptor

# ── Directories ────────────────────────────────────────────
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

# ── Compiler flags ─────────────────────────────────────────
# pkg-config supplies GTK4 include paths and link libs.
GTK_CFLAGS  := $(shell pkg-config --cflags gtk4)
GTK_LDFLAGS := $(shell pkg-config --libs   gtk4)

CFLAGS  := -std=c17 -Wall -Wextra -Wpedantic -g \
           -I$(INC_DIR) \
           $(GTK_CFLAGS)

LDFLAGS := $(GTK_LDFLAGS) \
           -lssl -lcrypto

# ── Source and object files ─────────────────────────────────
SRCS := $(SRC_DIR)/main.c              \
        $(SRC_DIR)/logger.c            \
        $(SRC_DIR)/file_handler.c      \
        $(SRC_DIR)/caesar_cipher.c     \
        $(SRC_DIR)/xor_cipher.c        \
        $(SRC_DIR)/aes_gcm.c           \
        $(SRC_DIR)/crypto_dispatcher.c \
        $(SRC_DIR)/ui.c

OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ── Default target ──────────────────────────────────────────
.PHONY: all clean run

all: $(OBJ_DIR) $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build succeeded → $(BINARY)"

# ── Compile each .c → .o ───────────────────────────────────
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Create obj directory if it does not exist ───────────────
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ── Remove build artefacts ──────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(BINARY) file_encryption.log

# ── Build and run ───────────────────────────────────────────
run: all
	./$(BINARY)
