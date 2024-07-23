# Nome do executável
TARGET = main

# Diretórios
SRC_DIR = src
INCLUDE_DIR = include

COLENDA_DIR = $(INCLUDE_DIR)/colenda
COLENDA_PATH = $(COLENDA_DIR)/colenda.c

MOUSE_DIR = $(INCLUDE_DIR)/mouse
MOUSE_PATH = $(MOUSE_DIR)/mouse.c

HEXDEC_DIR = $(INCLUDE_DIR)/hexdecode
HEXDEC_PATH = $(HEXDEC_DIR)/hexdecode.c

ASSETS_DIR = assets

LD_FLAGS = -lrt -lm -lpthread

# Arquivos fonte
SOURCES = $(SRC_DIR)/main.c $(COLENDA_PATH) $(MOUSE_PATH) $(HEXDEC_PATH) $(ASSETS_DIR)/night.c \
							$(ASSETS_DIR)/day.c $(ASSETS_DIR)/bullet.c $(ASSETS_DIR)/silver.c $(ASSETS_DIR)/garlic.c \
							$(ASSETS_DIR)/zombie.c $(ASSETS_DIR)/werewolf.c $(ASSETS_DIR)/vampire.c \
							$(ASSETS_DIR)/aim.c

# Arquivos objeto
OBJECTS = $(SOURCES:.c=.o)

# Compilador e flags
CC = gcc
CFLAGS = -I$(COLENDA_DIR) -I$(MOUSE_DIR) -I$(HEXDEC_DIR) -I$(ASSETS_DIR) -Wall -Wextra -O2 -std=c99

# Regra padrão
all: $(TARGET)

# Linkagem do executável
$(TARGET): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LD_FLAGS)

# Compilação dos arquivos objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza dos arquivos objeto e executável
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean

