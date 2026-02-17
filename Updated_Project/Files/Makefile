# ─────────────────────────────────────────────────────────────────────────────
# Makefile – Advanced File Encryption System
# ─────────────────────────────────────────────────────────────────────────────

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -Iinclude \
           $(shell pkg-config --cflags gtk4) \
           $(shell pkg-config --cflags openssl)
LDFLAGS := $(shell pkg-config --libs gtk4) \
           $(shell pkg-config --libs openssl)

SRC_DIR := src
OBJ_DIR := build
BIN     := file_encryptor

SRCS := $(SRC_DIR)/main.c \
        $(SRC_DIR)/encryption.c \
        $(SRC_DIR)/file_handler.c \
        $(SRC_DIR)/logger.c

OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(BIN)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)
	@echo "Cleaned."
