#include "fpga.h"

/**
 * @brief Initializes the FPGA by opening and mapping the /dev/mem file, and setting up pointers to the registers.
 *
 * @param fpga_map A pointer to the fpga_map_arm_t structure that will hold the file descriptor and pointers to the mapped memory.
 *
 * @return Returns 0 (SUCCESS) if the initialization is successful. If an error occurs, it returns -1.
 *
 * @note This function opens the /dev/mem file, maps the physical address range to virtual address space, and sets up pointers to the registers in the FPGA.
 *
 * @warning If an error occurs during initialization, the function will print an error message and clean up any resources allocated.
 */
int fpgainit(fpga_map_arm_t *fpga_map)
{
    // Try to open and return an error code if it fails
    fpga_map->fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fpga_map->fd == -1) {
        printf("Couldn't open /dev/mem");
        return -1;
    }

    // Map the physical address range to virtual address space
    fpga_map->mapped_ptr = mmap(NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fpga_map->fd, LW_BRIDGE_BASE);

    // Check if the mapping was successful
    if (fpga_map->mapped_ptr == MAP_FAILED) {
        printf("Couldn't map /dev/mem");
        close(fpga_map->fd);
        return ERROR;
    }
    
    // Set up the pointers to the registers in the FPGA
    fpga_map->KEY_ptr = (int *) (fpga_map->mapped_ptr + KEY_BASE);

    return SUCCESS;
}

/**
 * @brief Closes the FPGA by unmapping the memory and closing the file descriptor.
 *
 * @param fpga_map A pointer to the fpga_map_arm_t structure that holds the file descriptor and pointers to the mapped memory.
 *
 * @return Returns 0 (SUCCESS) if the closing is successful. If an error occurs, it returns -1.
 *
 * @note This function unmaps the physical address range from virtual address space and closes the /dev/mem file descriptor.
 *
 * @warning If an error occurs during closing, the function will print an error message.
 */
int fpgaclose(fpga_map_arm_t *fpga_map)
{
    // Try to unmap and return an error code if it fails
    if (munmap(fpga_map->mapped_ptr, LW_BRIDGE_SPAN) != 0) {
        printf("Couldn't munmap /dev/mem");
        return ERROR;
    }

    // Close the file descriptor
    if (fpga_map->fd >= 0) {
        close(fpga_map->fd);
        fpga_map->fd = -1;
    }

    return SUCCESS;
}

/**
 * @brief Reads the current state of the buttons on the FPGA and stores it in an array.
 *
 * @param fpga_map The fpga_map_arm_t structure that holds the file descriptor and pointers to the mapped memory.
 * @param pressed_keys A pointer to an array where the pressed state of each button will be stored.
 * @param size The size of the pressed_keys array.
 *
 * @return This function does not return a value.
 *
 * @note This function reads the inverted bits of the keys, shifts and masks to get the actual button states 
 * (pressed or not), and stores them in the pressed_keys array.
 *
 * @warning This function assumes that the fpga_map structure is properly 
 * initialized and that the size of the pressed_keys array is equal to the number of buttons on the FPGA.
 */
void readkeys(fpga_map_arm_t fpga_map, int *pressed_keys, size_t size) {
    // Shift and mask to get the actual button states (pressed or not)
    for (size_t i = 0; i < size; i++) {
        // Store the pressed state in the keys array
        pressed_keys[i] = (~*fpga_map.KEY_ptr >> i) & 1;
    }
}