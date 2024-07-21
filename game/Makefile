# Nome do executável
TARGET = main

# Diretórios
SRC_DIR = src
INCLUDE_DIR = include
COLENDA_DIR = $(INCLUDE_DIR)/colenda
MOUSE_DIR = $(INCLUDE_DIR)/mouse
ASSETS_DIR = assets

# Arquivos fonte
SOURCES = $(SRC_DIR)/main.c $(COLENDA_DIR)/colenda.c $(MOUSE_DIR)/mouse.c $(ASSETS_DIR)/night.c

# Arquivos objeto
OBJECTS = $(SOURCES:.c=.o)

# Compilador e flags
CC = gcc
CFLAGS = -I$(COLENDA_DIR) -I$(MOUSE_DIR) -I$(ASSETS_DIR) -Wall -Wextra -O2 -std=c99

# Regra padrão
all: $(TARGET)

# Linkagem do executável
$(TARGET): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ -lrt -lm 

# Compilação dos arquivos objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza dos arquivos objeto e executável
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean

