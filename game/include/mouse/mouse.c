#include "mouse.h"
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/**
 * @brief Initializes a mouse structure and opens the mouse device file.
 *
 * This function initializes a mouse structure and opens the mouse device file.
 * It sets the file descriptor of the mouse structure to -1 if an error occurs during
 * the file opening process.
 *
 * @return A mouse_t structure containing the initialized values.
 * @retval A mouse_t structure with the file descriptor set to -1 if an error occurs.
 *
 * @note The mouse device file is opened in non-blocking mode.
 * @note The mouse structure is initialized with default values.
 */
mouse_t mouseinit()
{
    // Open the mouse device file
    int fd = open((const char*) MOUSE_DEVICE, O_RDONLY | O_NONBLOCK);

    // Return the initialized mouse structure with the file descriptor set to -1 if an error occurs
    if (fd == -1) {
        return (mouse_t){.fd = -1};
    }

    // Return the initialized mouse structure with default values
    return (mouse_t){
        .fd = fd,
        .data = {0, 0, 0},
        .dx = 0,
        .dy = 0,
        .event = false,
        .left_button_state = false,
        .right_button_state = false,
        .middle_button_state = false,
        .previous_left_button_state = false,
        .previous_right_button_state = false,
        .previous_middle_button_state = false,
        .left_button_clicked = false,
        .right_button_clicked = false,
        .middle_button_clicked = false,
        .left_button_released = false,
        .right_button_released = false,
        .middle_button_released = false
    };
}

/**
 * @brief Validates the mouse structure and its file descriptor.
 *
 * This function checks if the mouse structure and its file descriptor are valid.
 * It returns an error code if the structure is not initialized or if the file
 * descriptor is invalid.
 *
 * @param mouse A pointer to the mouse structure to be validated.
 *
 * @return An integer representing the result of the validation.
 * @retval 0 The mouse structure and its file descriptor are valid.
 * @retval -EINVAL The mouse structure is not initialized.
 * @retval -EBADF The mouse file descriptor is invalid.
 *
 * @note This function does not modify the mouse structure or its file descriptor.
 */
inline int validatemouse(mouse_t *mouse)
{
     // Check if the mouse structure is not initialized
    if (mouse == NULL)
        return -EINVAL;
    
    // Check if the mouse file descriptor is invalid
    if (mouse->fd < 0)
        return -EBADF;

    // If everything is fine, return 0
    return 0;
}

/**
* @brief Closes the mouse device file descriptor.
* This function closes the file descriptor of the mouse device.
* It sets the file descriptor to -1 to indicate that the device is no longer open.
* @param mouse A pointer to the mouse structure containing the file descriptor.
* @return void
* @note This function does not return any value.
* @note The function checks if the file descriptor is valid before closing it.
*/
void mouseclose(mouse_t *mouse)
{
    if (mouse->fd >= 0) {
        close(mouse->fd);
        mouse->fd = -1; 
    }

    return;
}

/**
* @brief Reads data from the mouse device and updates the mouse state.
* This function reads data from the mouse device file descriptor and updates the
* mouse state based on the read data. It also detects button press and release
* transitions.
* @param mouse A pointer to the mouse structure containing the file descriptor and
* other mouse state variables.
* @return The number of bytes read from the mouse device. If an error occurs,
* it returns a negative error code.
* @retval -EINVAL If the mouse pointer is NULL.
* @retval -EBADF If the mouse file descriptor is invalid.
* @note The function updates the mouse state variables in the mouse structure.
*/
int mouseread(mouse_t *mouse)
{   
    int is_valid = validatemouse(mouse);

    if (is_valid < 0)
        return is_valid;
    
    // Clear the data buffer to prepare for new read
    memset(mouse->data, 0, sizeof(mouse->data));

    // Read data from the mouse device file descriptor
    int bytes = read(mouse->fd, mouse->data, sizeof(mouse->data));

    if (bytes > 0) {
        // An event has occurred
        mouse->event = true;

        // Update movement offsets based on the read data
        mouse->dx = mouse->data[1];
        mouse->dy = mouse->data[2];

        // Store previous button states
        mouse->previous_left_button_state = mouse->left_button_state;
        mouse->previous_right_button_state = mouse->right_button_state;
        mouse->previous_middle_button_state = mouse->middle_button_state;

        // Update button states based on the read data
        mouse->left_button_state = mouse->data[0] & 0x1;
        mouse->right_button_state = mouse->data[0] & 0x2;
        mouse->middle_button_state = mouse->data[0] & 0x4;

        // Detect button press transition (only when the button is pressed, not held)
        mouse->left_button_clicked = !mouse->previous_left_button_state && mouse->left_button_state;
        mouse->right_button_clicked = !mouse->previous_right_button_state && mouse->right_button_state;
        mouse->middle_button_clicked = !mouse->previous_middle_button_state && mouse->middle_button_state;

        // Detect button release transitions
        mouse->left_button_released = mouse->previous_left_button_state && !mouse->left_button_state;
        mouse->right_button_released = mouse->previous_right_button_state && !mouse->right_button_state;
        mouse->middle_button_released = mouse->previous_middle_button_state && !mouse->middle_button_state;
    } else {
        // No data read, so no event detected
        mouse->event = false;
        mouse->dx = 0;
        mouse->dy = 0;
    }

    // Return the number of bytes read or an error code
    return bytes;
}

/**
 * @brief Adjusts the sensitivity of the mouse movement.
 *
 * This function adjusts the sensitivity of the mouse movement by scaling the
 * horizontal and vertical movement offsets based on the given sensitivity factors.
 * The new movement offsets are calculated by multiplying the original offsets with
 * the sensitivity factors and rounding them to the nearest integer.
 *
 * @param mouse A pointer to the mouse structure containing the movement offsets.
 * @param mx The horizontal sensitivity factor. A value greater than 1 increases the sensitivity.
 * @param my The vertical sensitivity factor. A value greater than 1 increases the sensitivity.
 *
 * @return void
 *
 * @note The function assumes that the mouse structure is already initialized and
 * contains valid movement offsets. If the mouse structure is not initialized or
 * the file descriptor is invalid, it will do nothing.
 * @note The function updates the movement offsets in the mouse structure.
 */
void updatemousesensitivity(mouse_t *mouse, int mx, int my)
{
    if(validatemouse(mouse) < 0)
        return;

    // Adjust the movement offsets based on the sensitivity factors
    double dx_new = (double) mouse->dx * mx;
    double dy_new = (double) mouse->dy * my;

    // Adjust to integer values. 
    // If the movement is positive, use ceil() to round to the integer above.
    // Else, use floor() to round to the integer below.
    // This ensures that the movement is never lost if the velocity is too slow.
    mouse->dx = mouse->dx >= 0 ? (int) ceil(dx_new) : (int) floor(dx_new); 
    mouse->dy = mouse->dy >= 0 ? (int) ceil(dy_new) : (int) floor(dy_new);

    return;
}
