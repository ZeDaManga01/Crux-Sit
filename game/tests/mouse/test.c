#include <mouse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    mouse_t mouse = mouseinit();

    if (mouse.fd < 0) {
        printf("Erro\n");
        return -1;
    }

    while (1) {
        mouseread(&mouse);
        updatemousesensitivity(&mouse, 1, 1);

        if (mouse.event) {
            printf("Mouse move: dx=%d, dy=%d\n", mouse.dx, mouse.dy);
            if (mouse.left_button_clicked) {
                printf("Left button pressed\n");
            }
            if (mouse.left_button_released) {
                printf("Left button released\n");
            }
            if (mouse.left_button_state) {
                printf("Left button held\n");
            }
            if (mouse.right_button_clicked) {
                printf("Right button pressed\n");
            }
            if (mouse.right_button_released) {
                printf("Right button released\n");
            }
            if(mouse.right_button_state) {
                printf("Right button held\n");
            }
            if (mouse.middle_button_clicked) {
                printf("Middle button pressed\n");
            }
            if (mouse.middle_button_released) {
                break;
            }
            if (mouse.middle_button_state) {
                printf("Middle button held\n");
            }
        }

        usleep(10000);
    }

    return 0;
}