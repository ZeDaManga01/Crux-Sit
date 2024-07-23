#ifndef HEXDECODE_H
#define HEXDECODE_H

typedef struct {
    unsigned int r;
    unsigned int g;
    unsigned int b;
} rgb_t;

typedef struct {
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a;
} rgba_t;

rgb_t hextorgb(unsigned int hex);
rgba_t hextorgba(unsigned int hex);
rgb_t normalizergb(rgb_t color);
rgba_t normalizergba(rgba_t color);

#endif