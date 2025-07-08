# Compilador y flags
CC = gcc
CFLAGS = -D_GNU_SOURCE
LDFLAGS = -lpthread

# Directorios
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# Subdirectorios fuente
SUBDIRS := auth handshake socks5 request

# Archivos fuente en src raíz
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Archivos fuente en subdirectorios
SRCS += $(foreach dir,$(SUBDIRS),$(wildcard $(SRC_DIR)/$(dir)/*.c))

# Archivos objeto
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Binario destino
TARGET := $(BIN_DIR)/server

# Regla principal
all: $(TARGET)

# Compilar binario
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Compilar cada .c a .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Crear carpetas
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Limpiar
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
