CC = gcc

CFLAGS = -Wall -I$(LIB_DIR) -std=c99

LIB_DIR = include

COLENDA_DIR = colenda

MICE_DIR = mice

LIB_SRCS = $(LIB_DIR)/$(COLENDA_DIR)/colenda.c $(LIB_DIR)/$(MICE_DIR)/mice.c

LIB_OBJS = colenda.o mice.o

MAIN_OBJ = main.o

TARGET = main

all: $(TARGET)

$(TARGET): $(MAIN_OBJ) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(MAIN_OBJ) $(LIB_OBJS) -lrt -lm

$(MAIN_OBJ): main.c $(LIB_DIR)/$(COLENDA_DIR)/colenda.h $(LIB_DIR)/$(MICE_DIR)/mice.h
	$(CC) $(CFLAGS) -c main.c 

colenda.o: $(LIB_DIR)/$(COLENDA_DIR)/colenda.c $(LIB_DIR)/$(COLENDA_DIR)/colenda.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/$(COLENDA_DIR)/colenda.c

mice.o: $(LIB_DIR)/$(MICE_DIR)/mice.c $(LIB_DIR)/$(MICE_DIR)/mice.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/$(MICE_DIR)/mice.c

clean:
	rm -f $(TARGET) $(MAIN_OBJ) $(LIB_OBJS)

