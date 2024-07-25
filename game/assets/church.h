#ifndef CHURCH_H
#define CHURCH_H

#include <stdint.h>

#define CHURCH_FRAME_COUNT 3
#define CHURCH_FRAME_WIDTH 80
#define CHURCH_FRAME_HEIGHT 60

#define NIGHT_FRAME 0
#define NO_REMAINING_LIVES_FRAME 1
#define DAY_FRAME 2

extern const uint32_t church[3][4800];

#endif