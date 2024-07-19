#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/time.h>
#include "mice.h"
#include "colenda.h"

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
} rgb_color;

typedef struct {
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a;
} rgba_color;

typedef struct {
    double lastTime;
    double currentTime;
    double elapsedTime;
}timetracker;

typedef struct {
    int type;

    double x, y;
    double xo, yo;

    double size_x, size_y;

    double vx, vy;
    double ax, ay;

    double hitbox_x, hitbox_y;
    double hitbox_x_size, hitbox_y_size;
} entity;

typedef struct cursor {
    int x;
    int y;
    int previous_x;
    int previous_y;
} cursor;

rgb_color hex_to_rgb(unsigned int hex)
{
    rgb_color color;
    color.b = (hex >> 16) & 0xFF;
    color.g = (hex >> 8) & 0xFF;
    color.r = hex & 0xFF;
    return color;
}

rgba_color hex_to_rgba(unsigned int hex)
{
    rgba_color color;
    color.a = (hex >> 24) & 0xFF;
    color.b = (hex >> 16) & 0xFF;
    color.g = (hex >> 8) & 0xFF;
    color.r = hex & 0xFF;
    return color;
}

void normalize_rgb(rgb_color *color)
{
    color->r /= 32;
    color->g /= 32;
    color->b /= 32;
}

void normalize_rgba(rgba_color *color)
{
    color->r /= 32;
    color->g /= 32;
    color->b /= 32;
    color->a = (color->a > 0) ? 1 : 0;
}

double getTime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void updateClock(timetracker *timer)
{
    timer->currentTime = getTime();
    timer->elapsedTime = timer->currentTime - timer->lastTime;
    timer->lastTime = timer->currentTime;
}

void updatePosition(double *x, double *y, double vx, double vy, double elapsedTime)
{
    *x += vx * elapsedTime;
    *y += vy * elapsedTime;
}

void updateVelocity(double *vx, double *vy, double ax, double ay, double elapsedTime) {
    *vx += ax * elapsedTime;
    *vy += ay * elapsedTime;
}

void update_mice_pos(cursor *cursor, Mice *mice)
{
    cursor->previous_x = cursor->x;
    cursor->previous_y = cursor->y;

    /* atualiza a posição do mouse */
    cursor->x += mice->dx;
    cursor->y -= mice->dy;

    /* condicionais para manter o cursor dentro da tela
    sem ele, o cursor iria "fugir", andando infinitamente para os cantos */
    if (cursor->x < 0) {
        cursor->x = 0;
    } 
    else if (cursor->x >= MAX_X) {
        cursor->x = MAX_X-1;
    }

    if (cursor->y < 0) {
        cursor->y = 0;
    }
    else if (cursor->y >= MAX_Y) {
        cursor->y = MAX_Y-1;
    }

    return;
}

int check_collision(int x1, int y1, int x2, int y2, int size_x1, int size_y1, int size_x2, int size_y2)
{
	int c1 = x1 < (x2 + size_x2);
	int c2 = (x1 + size_x1) > x2;
	int c3 = y1 < (y2 + size_y2);
	int c4 = (y1 + size_y1) > y2;
	
	return c1 && c2 && c3 && c4;
}

int createentity(entity *entities, bool *hasEntity, entity mob, size_t size)
{
    size_t i;

    for (i = 0; i < size && hasEntity[i]; i++) {}

    if (i == size)
        return -1;

    entities[i] = mob;
    hasEntity[i] = true;

    return (int) i;
}

void removeentity(bool *hasEntity, size_t index, size_t size) 
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

int main(void)
{
    FILE *gpu = NULL;

    if (gpu_open(&gpu, DRIVER_NAME) < 0) {
        printf("Erro\n");
        return -1;
    }

    for (int i = 0; i < 60; i++) {
        for (int j = 0; j < 80; j++) {
            set_background_block(gpu, i, j, 6, 7, 7);
        }
    }

    set_background(gpu, 2, 1, 5);

    int count_background = 0;
    rgba_color color;
    for (int i = 0; i < 60; i++) {
        for (int j = 0; j < 80; j++) {
            color = hex_to_rgba(night_data[0][count_background++]);
            normalize_rgba(&color);

            printf("%d %d %d %d\n", color.r, color.g, color.b, color.a);

            if (color.a)
                set_background_block(gpu, i, j, color.r, color.g, color.b);
        }
    }

    Mice mouse = mice_open();
    cursor cursor = {0, 0, 0, 0};

    if (mouse.fd < 0) {
        printf("Erro\n");
        return -1;
    }

    timetracker timer;
    timer.lastTime = getTime();

    entity entities[MAX_ENTITIES];
    bool hasEntity[MAX_ENTITIES];

    for (int i = 0; i < MAX_ENTITIES; i++) {
        hasEntity[i] = false;
    }

    int bullet = 0;

    entity example;

    double spawnentitytime = 1;
    double count = 0;

    bool collision = false;

    for (int i = 1; i < 32; i++) {
        set_sprite(gpu, i, 0, 0, 0, 0);
    }

    while (1) {
        updateClock(&timer);

        mice_read(&mouse);
        correct_mice_sensitivity(&mouse, 1, 1);
        update_mice_pos(&cursor, &mouse);

        if (mouse.left_press_not_hold) {
            printf("clicou!\n");
        }

        if (count < spawnentitytime) {
            count += timer.elapsedTime;
        }
        else {
            example.type = rand() % 3;
            example.x = MAX_X;
            example.xo = example.x;
            example.y = rand() % MAX_Y;
            example.yo = example.y;
            example.size_x = 20;
            example.size_y = 20;
            example.vx = -(rand() % 300 + 50);
            example.vy = 0;
            example.ax = 0;
            example.ay = 0;
            example.hitbox_x = example.x - 5;
            example.hitbox_x = example.y - 5;
            example.hitbox_x_size = 30;
            example.hitbox_y_size = 30;

            if(example.type == VAMPIRE) {
                example.vy = rand() % 200;
                example.ay = -example.vy;
            }
    
            int result = createentity(entities, hasEntity, example, MAX_ENTITIES);
            count = 0;
        }

        for (int i = 0; i < MAX_ENTITIES; i++) {
            if (!hasEntity[i]) {
                continue;
            }

            if (check_collision(cursor.x, cursor.y, entities[i].x, entities[i].y, 20, 20, entities[i].hitbox_x_size, entities[i].hitbox_y_size)) {
                collision = true;
                
                set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].x, entities[i].y, entities[i].type * 2 + 1);
                
                printf("colidiu!\n");
                
                if (mouse.left_press_not_hold && bullet == entities[i].type) {
                    printf("matou!\n");
                    removeentity(hasEntity, i, MAX_ENTITIES);
                    set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
                    break;
                }
            } else {
                set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].x, entities[i].y, entities[i].type * 2);
            }

            updateVelocity(&entities[i].vx, &entities[i].vy, entities[i].ax, entities[i].ay, timer.elapsedTime);
            updatePosition(&entities[i].x, &entities[i].y, entities[i].vx, entities[i].vy, timer.elapsedTime);

            if (entities[i].type == VAMPIRE && \
            ((entities[i].y > entities[i].yo && entities[i].ay > 0) || \
            (entities[i].y < entities[i].yo && entities[i].ay < 0))) {
                entities[i].ay = -entities[i].ay; 
            }

            if (entities[i].x < -20) {
                removeentity(hasEntity, i, MAX_ENTITIES);
                set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
                break;
            }
        }

        if (collision) {
            set_sprite(gpu, 1, 0, cursor.x, cursor.y, 20);
        } else {
            set_sprite(gpu, 1, 1, cursor.x, cursor.y, 20);
        }
        

        if (mouse.right_release) {
            changebullettype(&bullet);
        }
        if (mouse.middle_release) {
            break;
        }
        
        set_sprite(gpu, 3, 1, 619, 459, bullet * 2);

        collision = false;
    }

    mice_close(&mouse);

    gpu_close(&gpu);
    return 0;
}
