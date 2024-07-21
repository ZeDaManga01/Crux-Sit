#ifndef MOUSE_H
#define MOUSE_H

#include <stdbool.h>

#define MOUSE_DEVICE "/dev/input/mice"

typedef struct Mouse {
    int fd;  // File descriptor of the mouse device
    unsigned char data[3];  // Buffer to store raw mouse data
    int dx;  // Horizontal movement offset of the mouse
    int dy;  // Vertical movement offset of the mouse
    bool event;  // Indicates if a mouse event occurred

    // Current state of the mouse buttons
    bool left_button_state;
    bool right_button_state;
    bool middle_button_state;

    // Previous state of the mouse buttons
    bool previous_left_button_state;
    bool previous_right_button_state;
    bool previous_middle_button_state;

    // Button transition events
    bool left_button_clicked;  // Indicates that the left button was pressed
    bool right_button_clicked;  // Indicates that the right button was pressed
    bool middle_button_clicked;  // Indicates that the middle button was pressed

    bool left_button_released;  // Indicates that the left button was released
    bool right_button_released;  // Indicates that the right button was released
    bool middle_button_released;  // Indicates that the middle button was released
} Mouse;

Mouse mouseinit(void);
inline int validatemouse(Mouse *mouse);
void mouseclose(Mouse *mouse);
int mouseread(Mouse *mouse);
void updatemousesensitivity(Mouse *mouse, int mx, int my);

#endif
