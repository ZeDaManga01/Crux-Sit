#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/time.h>
#include "mice.h"
#include "colenda.h"

#define MAX_X 640
#define MAX_Y 480

#define MAX_ENTITIES 20
#define MAX_BULLETTYPES 3
#define BULLET_OFFSET 5
#define ENTITY_SPRITE_LAYER_OFFSET 4

typedef struct {
    double lastTime;
    double currentTime;
    double elapsedTime;
}timetracker;

typedef struct {
    double x, y;
    double vx, vy;
    double ax, ay;
}entity;

typedef struct cursor {
    int x;
    int y;
    int previous_x;
    int previous_y;
} cursor;

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

int check_collision(int x1, int y1, int x2, int y2, int size_x, int size_y)
{
	int c1 = x1 < (x2 + size_x);
	int c2 = (x1 + size_x) > x2;
	int c3 = y1 < (y2 + size_y);
	int c4 = (y1 + size_y) > y2;
	
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
    Mice mouse = mice_open();
    cursor cursor = {0, 0, 0, 0};
    FILE *gpu = NULL;

    if (mouse.fd < 0 || gpu_open(&gpu, DRIVER_NAME) < 0) {
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
    
    set_background(gpu, 0, 0, 0);

    int bullet = 0;

    entity example;

    double spawnentitytime = 1;
    double count = 0;

    for (int i = 1; i < 32; i++) {
        set_sprite(gpu, i, 0, 0, 0, 0);
    }

    while (1) {
        updateClock(&timer);

        mice_read(&mouse);
        correct_mice_sensitivity(&mouse, 1, 1);
        update_mice_pos(&cursor, &mouse);

        if (count < spawnentitytime) {
            count += timer.elapsedTime;
        }
        else {
            example.x = MAX_X;
            example.y = rand() % MAX_Y;
            example.vx = -(rand() % 300 + 50);
            example.vy = 0;
            example.ax = 0;
            example.ay = 0;
    
            int result = createentity(entities, hasEntity, example, MAX_ENTITIES);
            printf("%d\n", result);

            count = 0;
        }

        for (int i = 0; i < MAX_ENTITIES; i++) {
            if (!hasEntity[i]) {
                continue;
            }

            if (check_collision(cursor.x, cursor.y, entities[i].x, entities[i].y, 20, 20)) {
                set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].x, entities[i].y, 0);
                printf("colidiu!\n");
                
                if (mouse.left_release) {
                    printf("matou!\n");
                    removeentity(hasEntity, i, MAX_ENTITIES);
                    set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
                    break;
                }
            } else {
                set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].x, entities[i].y, 1);
            }

            updateVelocity(&entities[i].vx, &entities[i].vy, entities[i].ax, entities[i].ay, timer.elapsedTime);
            updatePosition(&entities[i].x, &entities[i].y, entities[i].vx, entities[i].vy, timer.elapsedTime);

            if (entities[i].x < -20) {
                removeentity(hasEntity, i, MAX_ENTITIES);
                set_sprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
                printf("entidade %d saiu!\n", i);
                break;
            }
        }

        set_sprite(gpu, 1, 1, cursor.x, cursor.y, 20);

        if (mouse.right_release) {
            changebullettype(&bullet);
        }
        if (mouse.middle_release) {
            break;
        }
        
        set_sprite(gpu, 3, 1, 619, 459, bullet);
    }

    mice_close(&mouse);
    gpu_close(&gpu);
    return 0;
}


