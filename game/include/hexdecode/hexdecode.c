#include "hexdecode.h"

rgb_t hextorgb(unsigned int hex)
{
    rgb_t color;

    color.b = (hex >> 16) & 0xFF;
    color.g = (hex >> 8) & 0xFF;
    color.r = hex & 0xFF;

    return color;
}

rgba_t hextorgba(unsigned int hex)
{
    rgba_t color;

    color.a = (hex >> 24) & 0xFF;
    color.b = (hex >> 16) & 0xFF;
    color.g = (hex >> 8) & 0xFF;
    color.r = hex & 0xFF;

    return color;
}

rgb_t normalizergb(rgb_t color)
{
    color.r /= 32;
    color.g /= 32;
    color.b /= 32;

    return color;
}

rgba_t normalizergba(rgba_t color)
{
    color.r /= 32;
    color.g /= 32;
    color.b /= 32;
    color.a = (color.a > 0) ? 1 : 0;

    return color;
}