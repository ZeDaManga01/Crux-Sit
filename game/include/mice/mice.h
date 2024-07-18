#ifndef MICE_H
#define MICE_H

#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @struct Mice
 * @brief Representa as informações do mouse.
 * 
 * Essa struct guarda informações do mouse: se houve algum evento, 
 * o seu deslocamento e qual dos botões foram clicados.
 */
    typedef struct Mice {
        int fd;
        unsigned char data[3];

        signed char dx; /**< O deslocamento horizontal do mouse. */
        signed char dy; /**< O deslocamento vertical do mouse. */
        
        bool event;

        bool left_press;
        bool right_press;
        bool middle_press;

        bool previous_left_press;
        bool previous_right_press; 
        bool previous_middle_press;

        bool left_release;
        bool right_release;
        bool middle_release;
    } Mice;

    Mice mice_open();

    void mice_close(struct Mice *mice);

    void mice_read(Mice *mice);

    void correct_mice_sensitivity(Mice *mice, int x_divider, int y_divider);

#endif