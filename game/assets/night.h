#ifndef NIGHT_H
#define NIGHT_H

#include <stdint.h>

#define NIGHT_FRAME_COUNT 1
#define NIGHT_FRAME_WIDTH 80
#define NIGHT_FRAME_HEIGHT 60

#define NIGHT_BACKGROUND_COLOR 0xffa91a48

extern const uint32_t night_data[NIGHT_FRAME_COUNT][NIGHT_FRAME_WIDTH * NIGHT_FRAME_HEIGHT];

#endif  // ARQUIVO_H
