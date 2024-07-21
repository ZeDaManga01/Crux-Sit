#include "colenda.h"
#include <string.h>

/**
* @brief Opens a file in read/write mode for GPU operations.
* This function attempts to open a file with the given driver name in read/write mode.
* If the file is successfully opened, the file pointer is stored in the provided address.
* If the file cannot be opened, an error code is returned.
* @param file A pointer to a FILE pointer. The opened file will be stored in this address.
* @param driver_name The name of the GPU driver file to be opened.
* @return SUCCESS if the file is opened successfully.
* @return -EOPEN if the file cannot be opened.
 */
int gpuopen(FILE **file, const char *driver_name)
{
    *file = fopen(driver_name, "r+");

    if (!(*file)) {
        return -EOPEN;
    }

    return SUCCESS;
}

/**
* @brief Closes an open file for GPU operations.
* This function attempts to close a file that has been previously opened for GPU operations.
* If the file is successfully closed, the file pointer is set to NULL.
* If the file cannot be closed, an error code is returned.
* @param file A pointer to a FILE pointer. The opened file will be closed and the pointer will be set to NULL.
* @return SUCCESS if the file is closed successfully.
* @return -ECLOSE if the file cannot be closed.
*/
int gpuclose(FILE **file)
{
    if(!fclose(*file)) {
        return -ECLOSE;
    }

    *file = NULL;

    return SUCCESS;
}

/**
* @brief Writes data from a buffer to an open file for GPU operations.
* This function attempts to write data from a buffer to an open file for GPU operations.
* It uses the fwrite function to write the data, and then seeks back to the beginning of the file.
* If the number of bytes written is not equal to the BUFFER_SIZE, an error code is returned.
* @param file A pointer to the FILE object representing the open file.
* @param buffer A pointer to the buffer containing the data to be written.
* @return SUCCESS if the data is written successfully.
* @return -EWRITE if the number of bytes written is not equal to BUFFER_SIZE.
*/
int gpuwrite(FILE *file, const char *buffer)
{
    int bytes = fwrite(buffer, sizeof(char), BUFFER_SIZE, file);

    if (bytes != BUFFER_SIZE) {
        return -EWRITE;
    }

    fseek(file, 0, SEEK_SET);

    return SUCCESS;
}

/**
* @brief Copies a 64-bit instruction to a buffer in little-endian format.
* This function takes a 64-bit instruction and copies it to a buffer in little-endian format.
* It checks if the buffer has enough space to store the instruction. If the buffer is too small,
* it returns -ESIZE. Otherwise, it copies the instruction byte by byte, starting from the least
* significant byte (LSB) and moving to the most significant byte (MSB).
* @param buffer A pointer to the buffer where the instruction will be copied.
* @param size The size of the buffer in bytes.
* @param instruction The 64-bit instruction to be copied to the buffer.
* @return SUCCESS if the instruction is copied successfully to the buffer.
* @return -ESIZE if the buffer is too small to store the instruction.
*/
int copytobuffer(char *buffer, size_t size, uint64_t instruction) {
    memset(buffer, 0, BUFFER_SIZE);

    if (size < BUFFER_SIZE) {
        return -ESIZE;
    }

    for (int i = sizeof(instruction) - 1, j = 0; i >= 0; i--, j++) {
        buffer[i] = (instruction >> j * 8);
    }

    return SUCCESS;
}

/**
 * @brief Defines the game's background color.
 *
 * This function generates a uint64_t instruction that represents a command to be sent to the character driver.
 * The instruction includes the opcode (WBR) and the RGB color values. The RGB values are packed into the instruction
 * using bitwise operations. The instruction is then split into 8-bit blocks and stored in the provided buffer.
 *
 * @param gpu A pointer to the GPU file.
 * @param red The red component of the color (0-7).
 * @param green The green component of the color (0-7).
 * @param blue The blue component of the color (0-7).
 *
 * @return SUCCESS if the operation is successful, EINPUT if the input values are invalid,
 * ESIZE if the buffer does not have enough space to store the instruction,
 * EWRITE if a write error occurs in the file.
 */
int setbackground(FILE *gpu, uint8_t red, uint8_t green, uint8_t blue)
{
    char buffer[BUFFER_SIZE];

    if ((red | green | blue) & ~0x7) /* Entrada do usuário inválida */ {
        return -EINPUT;
    }

    /* dataA*/
    uint64_t instruction = WBR; // Atribuindo o opcode da instrução

    /* dataB */
    instruction |= (uint64_t) (red & 0x7) << 32; // Início da área reservada para o segundo registrador
    instruction |= (uint64_t) (green & 0x7) << 35; // 3 bits da cor passada
    instruction |= (uint64_t) (blue & 0x7) << 38; // 6 bits

    if (copytobuffer(buffer, BUFFER_SIZE, instruction) != 0) {
        return -ESIZE;
    }

    return gpuwrite(gpu, buffer);
}

/**
 * @brief Defines the sprite parameters in the instruction buffer.
 *
 * This function generates a 64-bit instruction that represents a command to be sent to the character driver.
 * The instruction includes the opcode (WBR), sprite register, visibility flag, coordinates, and offset.
 * The parameters are packed into the instruction using bitwise operations. The instruction is then split into
 * 8-bit blocks and stored in the provided buffer.
 *
 * @param gpu A pointer to the GPU file.
 * @param layer The sprite register (0-31).
 * @param show The visibility flag (0: hidden, 1: visible).
 * @param x The x-coordinate of the sprite (0-1023).
 * @param y The y-coordinate of the sprite (0-1023).
 * @param sprite The sprite offset in memory (0-511).
 *
 * @return SUCCESS if the operation is successful, EINPUT if the input values are invalid,
 * ESIZE if the buffer does not have enough space to store the instruction,
 * EWRITE if a write error occurs in the file.
 */
int setsprite(FILE *gpu, uint8_t layer, bool show, uint16_t x, uint16_t y, uint16_t sprite)
{
    char buffer[BUFFER_SIZE];

    // Check for invalid input values
    if (((x | y) & ~0x3FF) || (sprite & ~0x1FF) || (layer & ~0x1F)) {
        return -EINPUT;
    }

    uint64_t instruction = WBR;

    // dataA
    instruction |= (uint64_t) (layer & 0x1F) << 4;
    
    // dataB
    instruction |= (uint64_t) (sprite & 0x1FF) << 32;
    instruction |= (uint64_t) (y & 0x3FF) << 41;
    instruction |= (uint64_t) (x & 0x3FF) << 51;
    instruction |= (uint64_t) (show & 0x1) << 61;
    
    copytobuffer(buffer, BUFFER_SIZE, instruction);

    if (copytobuffer(buffer, BUFFER_SIZE, instruction) != 0) {
        return -ESIZE;
    }

    return gpuwrite(gpu, buffer);
}

/**
 * @brief Defines the parameters of a polygon in the instruction buffer.
 *
 * This function generates a 64-bit instruction that represents a command to be sent to the character driver.
 * The instruction includes the opcode (DP), address, type, coordinates, color, and size.
 * The parameters are packed into the instruction using bitwise operations. The instruction is then split into
 * 8-bit blocks and stored in the provided buffer.
 *
 * @param gpu A pointer to the GPU file.
 * @param layer The polygon address (0-15).
 * @param type The type of the polygon (0: square, 1: triangle).
 * @param x The x-coordinate of the polygon (0-511).
 * @param y The y-coordinate of the polygon (0-511).
 * @param red The red component of the color (0-7).
 * @param green The green component of the color (0-7).
 * @param blue The blue component of the color (0-7).
 * @param size The size of the polygon (0-15).
 *
 * @return SUCCESS if the operation is successful, EINPUT if the input values are invalid,
 * ESIZE if the buffer does not have enough space to store the instruction,
 * EWRITE if a write error occurs in the file.
 */
int setpoligon(FILE *gpu, uint_fast8_t layer, uint_fast8_t type, uint16_t x, uint16_t y, uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue, uint_fast8_t size)
{
    char buffer[BUFFER_SIZE];

    if (((red | green | blue) & ~0x7) || (size & ~0xF) || ((x | y) & ~0x1FF) || (type & ~0x1) || (layer & ~0x1F)) /* Entrada do usuário inválida */ {
        return -EINPUT;
    }

    uint64_t instruction = DP;

    /* dataA */
    instruction |= (uint64_t) (layer & 0x1F) << 4;

    /* dataB */
    instruction |= (uint64_t) (x & 0x1FF) << 32;
    instruction |= (uint64_t) (y & 0x1FF) << 41;
    instruction |= (uint64_t) (size & 0xF) << 50;
    instruction |= (uint64_t) (red & 0x7) << 54;
    instruction |= (uint64_t) (green & 0x7) << 57;
    instruction |= (uint64_t) (blue & 0x7) << 60;
    instruction |= (uint64_t) (type & 0x1) << 63;

    if (copytobuffer(buffer, BUFFER_SIZE, instruction) != 0) {
        return -ESIZE;
    }

    return gpuwrite(gpu, buffer);
}

/**
 * @brief Defines the color of a specific block in the background.
 *
 * This function calculates the block address based on the provided rows and columns,
 * and then generates a WBM instruction to write the color data to the specified address in the background memory.
 * The function checks for valid input values and returns an error code if necessary.
 *
 * @param gpu A pointer to the GPU file.
 * @param x The column index of the block (zero-based).
 * @param y The row index of the block (zero-based).
 * @param red The red component of the color (0-7).
 * @param green The green component of the color (0-7).
 * @param blue The blue component of the color (0-7).
 *
 * @return SUCCESS if the operation is successful, EINPUT if the input values are invalid,
 * ESIZE if the buffer does not have enough space to store the instruction,
 * EWRITE if a write error occurs in the file.
 */
int setbackgroundblock(FILE *gpu, uint_fast8_t x, uint_fast8_t y, uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue)
{
    char buffer[BUFFER_SIZE];

    uint16_t address = y * (SCREEN_WIDTH / BLOCK_SIZE) + x;

    if ((red | green | blue) & ~0x7 || (address & ~0x3FFF) >= ((SCREEN_HEIGHT / BLOCK_SIZE) * (SCREEN_WIDTH / BLOCK_SIZE))) /* Entrada do usuário inválida */ {
        return -EINPUT;
    }

    uint64_t instruction = WBM;

    /* dataA */
    instruction |= (uint64_t) (address & 0x3FFF) << 4;

    /* dataB */
    instruction |= (uint64_t) (red & 0x7) << 32; // Início da área reservada para o segundo registrador
    instruction |= (uint64_t) (green & 0x7) << 35; // 3 bits da cor passada
    instruction |= (uint64_t) (blue & 0x7) << 38; // 6 bits

    if (copytobuffer(buffer, BUFFER_SIZE, instruction) != 0) {
        return -ESIZE;
    }

    return gpuwrite(gpu, buffer);
}