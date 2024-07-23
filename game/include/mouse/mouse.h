#ifndef MOUSE_H
#define MOUSE_H

#define MOUSE_DEVICE "/dev/input/mice"

typedef struct mouse_t {
    int fd;  // File descriptor of the mouse device
    unsigned char data[3];  // Buffer to store raw mouse data
    signed char dx;  // Horizontal movement offset of the mouse
    signed char dy;  // Vertical movement offset of the mouse
    int event;  // Indicates if a mouse event occurred

    // Current state of the mouse buttons
    int left_button_state;
    int right_button_state;
    int middle_button_state;

    // Previous state of the mouse buttons
    int previous_left_button_state;
    int previous_right_button_state;
    int previous_middle_button_state;

    // Button transition events
    int left_button_clicked;  // Indicates that the left button was pressed
    int right_button_clicked;  // Indicates that the right button was pressed
    int middle_button_clicked;  // Indicates that the middle button was pressed

    int left_button_released;  // Indicates that the left button was released
    int right_button_released;  // Indicates that the right button was released
    int middle_button_released;  // Indicates that the middle button was released
} mouse_t;

int validatemouse(mouse_t *mouse);
mouse_t mouseinit(void);
void mouseclose(mouse_t *mouse);
int mouseread(mouse_t *mouse);
void updatemousesensitivity(mouse_t *mouse, int mx, int my);

#endif
