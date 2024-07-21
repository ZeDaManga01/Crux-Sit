#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <linux/time.h>
#include "../include/mouse/mouse.h"
#include "../include/colenda/colenda.h"

#include "night.h"

#define MAX_X 640
#define MAX_Y 480

#define MAX_ENTITIES 20
#define MAX_BULLETTYPES 3
#define BULLET_OFFSET 5

#define ZOMBIE 0
#define WEREWOLF 1
#define VAMPIRE 2

#define NORMAL_BULLET 0
#define SILVER_BULLET 1
#define GARLIC_BULLET 2

#define ENTITY_SPRITE_LAYER_OFFSET 4

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

typedef struct {
    double lastTime;
    double currentTime;
    double elapsedTime;
} timetracker_t;

typedef struct {
    double x, y;
    double size_x, size_y;
} rectangle_t;

typedef struct {
    double x, y;
} position_t;

typedef struct {
    double x, y;
} velocity_t;

typedef struct {
    double x, y;
} acceleration_t;

typedef struct {
    int type;

    position_t start_position;
    position_t position;

    velocity_t velocity;
    acceleration_t acceleration;

    rectangle_t hitbox;
} entity_t;

typedef struct {
    position_t position;
    position_t previous_position;
} cursor_t;

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

void normalizergb(rgb_t *color)
{
    color->r /= 32;
    color->g /= 32;
    color->b /= 32;
}

void normalizergba(rgba_t *color)
{
    color->r /= 32;
    color->g /= 32;
    color->b /= 32;
    color->a = (color->a > 0) ? 1 : 0;
}

double gettime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void updatetime(timetracker_t *timer)
{
    timer->currentTime = gettime();
    timer->elapsedTime = timer->currentTime - timer->lastTime;
    timer->lastTime = timer->currentTime;
}

void updateposition(double *x, double *y, double vx, double vy, double elapsedTime)
{
    *x += vx * elapsedTime;
    *y += vy * elapsedTime;
}

void updatevelocity(double *vx, double *vy, double ax, double ay, double elapsedTime) {
    *vx += ax * elapsedTime;
    *vy += ay * elapsedTime;
}

void updatecursor(cursor_t *cursor, mouse_t *mouse)
{
    cursor->previous_position.x = cursor->position.x;
    cursor->previous_position.y = cursor->position.y;

    /* atualiza a posição do mouse */
    cursor->position.x += mouse->dx;
    cursor->position.y -= mouse->dy;

    /* condicionais para manter o cursor dentro da tela
    sem ele, o cursor iria "fugir", andando infinitamente para os cantos */
    if (cursor->position.x < 0) {
        cursor->position.x = 0;
    } 
    else if (cursor->position.x >= MAX_X) {
        cursor->position.x = MAX_X-1;
    }

    if (cursor->position.y < 0) {
        cursor->position.y = 0;
    }
    else if (cursor->position.y >= MAX_Y) {
        cursor->position.y = MAX_Y-1;
    }

    return;
}

int checkcollision(int x1, int y1, int x2, int y2, int size_x1, int size_y1, int size_x2, int size_y2)
{
	int c1 = x1 < (x2 + size_x2);
	int c2 = (x1 + size_x1) > x2;
	int c3 = y1 < (y2 + size_y2);
	int c4 = (y1 + size_y1) > y2;
	
	return c1 && c2 && c3 && c4;
}

int createentity(entity_t *entities, bool *hasEntity, entity_t mob, size_t size)
{
    size_t i;

    for (i = 0; i < size && hasEntity[i]; i++) {}

    if (i == size)
        return -1;

    entities[i] = mob;
    hasEntity[i] = true;

    return (int) i;
}

void deleteentity(bool *hasEntity, size_t index, size_t size) 
{
    if (index >= size)
        return;

    hasEntity[index] = false;
}

void changebullettype(int *bullet) 
{
    *bullet += 1;

    if (*bullet >= 3) {
        *bullet = 0;
    }
}

int clearbackground(FILE **gpu)
{
    for (int i = 0; i < 60; i++) {
        for (int j = 0; j < 80; j++) {
            setbackgroundblock(*gpu, i, j, 6, 7, 7);
        }
    }
}

int main(void) {
    FILE *gpu = NULL;

    if (gpuopen(&gpu, DRIVER_NAME) < 0) {
        printf("Error opening GPU driver\n");
        return -1;
    }

    for (int i = 0; i < 60; i++) {
        for (int j = 0; j < 80; j++) {
            setbackgroundblock(gpu, i, j, 6, 7, 7);
        }
    }

    setbackground(gpu, 2, 1, 5);

    int count_background = 0;
    rgba_t color;
    for (int i = 0; i < 60; i++) {
        for (int j = 0; j < 80; j++) {
            color = hextorgba(night_data[0][count_background++]);
            normalizergba(&color);

            if (color.a)
                setbackgroundblock(gpu, i, j, color.r, color.g, color.b);
        }
    }

    mouse_t mouse = mouseinit();
    cursor_t cursor = { {0, 0}, {0, 0} };

    if (mouse.fd < 0) {
        printf("Erro\n");
        return -1;
    }

    timetracker_t timer;
    timer.lastTime = gettime();

    entity_t entities[MAX_ENTITIES];
    bool hasEntity[MAX_ENTITIES];

    for (int i = 0; i < MAX_ENTITIES; i++) {
        hasEntity[i] = false;
    }

    int bullet = 0;

    entity_t example;

    double spawnEntitytime = 1;
    double count = 0;

    bool collision = false;

    for (int i = 1; i < 32; i++) {
        setsprite(gpu, i, 0, 0, 0, 0);
    }

    while (1) {
        updatetime(&timer);

        mouseread(&mouse);
        updatemousesensitivity(&mouse, 1, 1);
        updatecursor(&cursor, &mouse);

        if (mouse.left_button_clicked) {
            printf("clicou!\n");
        }

        if (count < spawnEntitytime) {
            count += timer.elapsedTime;
        } else {
            example.type = rand() % 3;
            example.position.x = MAX_X;
            example.start_position.x = example.position.x;
            example.position.y = rand() % MAX_Y;
            example.start_position.y = example.position.y;
            example.velocity.x = -(rand() % 300 + 50);
            example.velocity.y = 0;
            example.acceleration.x = 0;
            example.acceleration.y = 0;
            example.hitbox.x = example.position.x - 5;
            example.hitbox.y = example.position.y - 5;
            example.hitbox.size_x = 30;
            example.hitbox.size_y = 30;

            if (example.type == VAMPIRE) {
                example.velocity.y = rand() % 200;
                example.acceleration.y = -example.velocity.y;
            }

            int result = createentity(entities, hasEntity, example, MAX_ENTITIES);
            count = 0;
        }

        for (int i = 0; i < MAX_ENTITIES; i++) {
            if (!hasEntity[i]) {
                continue;
            }

            if (checkcollision(cursor.position.x, cursor.position.y, entities[i].position.x, entities[i].position.y, 20, 20, entities[i].hitbox.size_x, entities[i].hitbox.size_y)) {
                collision = true;

                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, entities[i].type * 2 + 1);

                printf("colidiu!\n");

                if (mouse.left_button_clicked && bullet == entities[i].type) {
                    printf("matou!\n");
                    deleteentity(hasEntity, i, MAX_ENTITIES);
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
                    break;
                }
            } else {
                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, entities[i].type * 2);
            }

            updatevelocity(&entities[i].velocity.x, &entities[i].velocity.y, entities[i].acceleration.x, entities[i].acceleration.y, timer.elapsedTime);
            updateposition(&entities[i].position.x, &entities[i].position.y, entities[i].velocity.x, entities[i].velocity.y, timer.elapsedTime);

            if (entities[i].type == VAMPIRE && 
            ((entities[i].position.y > entities[i].start_position.y && entities[i].acceleration.y > 0) || 
            (entities[i].position.y < entities[i].start_position.y && entities[i].acceleration.y < 0))) {
                entities[i].acceleration.y = -entities[i].acceleration.y; 
            }

            if (entities[i].position.x < -20) {
                deleteentity(hasEntity, i, MAX_ENTITIES);
                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
                break;
            }
        }

        if (collision) {
            setsprite(gpu, 1, 0, cursor.position.x, cursor.position.y, 20);
        } else {
            setsprite(gpu, 1, 1, cursor.position.x, cursor.position.y, 20);
        }

        if (mouse.right_button_released) {
            changebullettype(&bullet);
        }
        if (mouse.middle_button_released) {
            break;
        }

        setsprite(gpu, 3, 1, 619, 459, bullet * 2);

        collision = false;
    }

    mouseclose(&mouse);
    gpuclose(&gpu);
    return 0;
}
