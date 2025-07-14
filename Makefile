include Makefile.inc

# Directorios principales
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# Fuentes por módulos
BASE_SOURCES=$(wildcard $(SRC_DIR)/*.c)
AUTH_SOURCES=$(wildcard $(SRC_DIR)/auth/*.c)
HANDSHAKE_SOURCES=$(wildcard $(SRC_DIR)/handshake/*.c)
SOCKS5_SOURCES=$(wildcard $(SRC_DIR)/socks5/*.c)
REQUEST_SOURCES=$(wildcard $(SRC_DIR)/request/*.c)
CLIENT_SOURCES=$(wildcard $(SRC_DIR)/client/*.c)
MANAGEMENT_SOURCES=$(wildcard $(SRC_DIR)/management/*.c)
METRICS_SOURCES=$(wildcard $(SRC_DIR)/metrics/*.c)

# Objetos
BASE_OBJECTS=$(BASE_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
AUTH_OBJECTS=$(AUTH_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
HANDSHAKE_OBJECTS=$(HANDSHAKE_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SOCKS5_OBJECTS=$(SOCKS5_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
REQUEST_OBJECTS=$(REQUEST_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CLIENT_OBJECTS=$(CLIENT_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
MANAGEMENT_OBJECTS=$(MANAGEMENT_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
METRICS_OBJECTS=$(METRICS_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Todos los objetos
ALL_OBJECTS=$(BASE_OBJECTS) $(AUTH_OBJECTS) $(HANDSHAKE_OBJECTS) $(SOCKS5_OBJECTS) $(REQUEST_OBJECTS) $(MANAGEMENT_OBJECTS) $(METRICS_OBJECTS)
ALL_OBJECTS=$(BASE_OBJECTS) $(AUTH_OBJECTS) $(HANDSHAKE_OBJECTS) $(SOCKS5_OBJECTS) $(REQUEST_OBJECTS) $(MANAGEMENT_OBJECTS) $(METRICS_OBJECTS)

# Target principal
TARGET := $(BIN_DIR)/socks5v


all: $(TARGET)

$(TARGET): $(ALL_OBJECTS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(ALL_OBJECTS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)

.PHONY: all clean

CLIENT_TARGET := $(BIN_DIR)/client

client: $(CLIENT_OBJECTS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLIENT_OBJECTS) -o $(CLIENT_TARGET)
