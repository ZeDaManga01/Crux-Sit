#ifndef COLENDA_H
#define COLENDA_H

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

#define DRIVER_NAME "/dev/colenda_driver"

#define WBR 0b0000 // Write to Register Bank
#define WSM 0b0001 // Write to Sprite Memory
#define WBM 0b0010 // Write to Background Memory
#define DP 0b0011 // Polygon Definition

#define SUCCESS 0
#define ESIZE EINVAL // Invalid argument (commonly used for buffer size errors)
#define EINPUT EINVAL // Invalid argument (general input error)
#define EOPEN EIO // Input/output error (error opening file)
#define ECLOSE EIO // Input/output error (error closing file)
#define EWRITE EIO // Input/output error (error writing to file)

#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 640

#define BLOCK_SIZE 8
#define BUFFER_SIZE 8

int gpuopen(FILE **file, const char *driver_name);
int gpuclose(FILE **file);
int gpuwrite(FILE *file, const char *buffer);
int copytobuffer(char *buffer, size_t size, uint64_t instruction);
int setbackground(FILE *gpu, uint8_t red, uint8_t green, uint8_t blue);
int setsprite(FILE *gpu, uint8_t layer, bool show, uint16_t x, uint16_t y, uint16_t sprite);
int setpoligon(FILE *gpu, uint_fast8_t layer, uint_fast8_t type, uint16_t x, uint16_t y, uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue, uint_fast8_t size);
int setbackgroundblock(FILE *gpu, uint_fast8_t row, uint_fast8_t column, uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue);

#endif
