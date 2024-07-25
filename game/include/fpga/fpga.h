#ifndef FPGA_H
#define FPGA_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../../driver/address_map_arm.h"

#define SUCCESS 0
#define ERROR -1

#define BUTTON_INITIALIZER { 0, 0, 0, 0 }
#define FPGA_MAP_ARM_INITIALIZER {0, NULL, NULL};

// Struct that holds the pointers to the used registers
typedef struct fpga_map_arm_t{
    int fd;
    void *mapped_ptr;

    volatile int *KEY_ptr;
}fpga_map_arm_t;

int fpgainit(fpga_map_arm_t *fpga_map);
int fpgaclose(fpga_map_arm_t *fpga_map);
void readkeys(fpga_map_arm_t fpga_map, int *pressed_keys, size_t size);

#endif