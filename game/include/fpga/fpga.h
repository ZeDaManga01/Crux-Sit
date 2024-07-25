#ifndef FPGA_H
#define FPGA_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../address_map_arm.h"

#define SUCCESS 0
#define ERROR -1

#define BUTTON_INITIALIZER { 0, 0, 0, 0 }
#define FPGA_MAP_ARM_INITIALIZER {0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

// Struct that holds the pointers to the used registers
typedef struct fpga_map_arm_t{
    int fd;
    void *mapped_ptr;

    volatile int *KEY_ptr;

    volatile int *HEX5_ptr;
    volatile int *HEX4_ptr;
    volatile int *HEX3_ptr;
    volatile int *HEX2_ptr;
    volatile int *HEX1_ptr;
    volatile int *HEX0_ptr;
} fpga_map_arm_t;

int fpgainit(fpga_map_arm_t *fpga_map);
int fpgaclose(fpga_map_arm_t *fpga_map);
void readkeys(fpga_map_arm_t fpga_map, int *pressed_keys, size_t size);
int numbertodigit (int number);
void setdigit(fpga_map_arm_t fpga_map, int number, int hex);

#endif