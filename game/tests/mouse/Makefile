# Definição do compilador
CC = gcc

# Flags do compilador
CFLAGS = -I../../include/mouse -Wall -Wextra

LD_FLAGS = -lrt -lm -lpthread

# Arquivo objeto
OBJ = test.o mouse.o

# Nome do executável
TARGET = test

# Regras de compilação
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LD_FLAGS)

test.o: test.c
	$(CC) $(CFLAGS) -c test.c -o test.o

mouse.o: ../../include/mouse/mouse.c
	$(CC) $(CFLAGS) -c ../../include/mouse/mouse.c -o mouse.o -lm

# Limpeza dos arquivos objetos e executável
clean:
	rm -f $(OBJ) $(TARGET)

# Remover arquivos compilados para recompilar do zero
.PHONY: clean all
