#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <linux/time.h>
#include <pthread.h>
#include "../include/mouse/mouse.h"
#include "../include/colenda/colenda.h"

#include "night.h"

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

rgb_t hextorgb(unsigned int hex);
rgba_t hextorgba(unsigned int hex);
void normalizergb(rgb_t *color);
void normalizergba(rgba_t *color);
double gettime();
void updatetime(timetracker_t *timer);
void updateposition(double *x, double *y, double vx, double vy, double elapsedTime);
void updatevelocity(double *vx, double *vy, double ax, double ay, double elapsedTime);
void updatecursor(cursor_t *cursor, mouse_t *mouse);
int checkcollision(int x1, int y1, int x2, int y2, int size_x1, int size_y1, int size_x2, int size_y2);
int createentity(entity_t *entities, bool *is_entity_in_slot, entity_t mob, size_t size);
void deleteentity(bool *is_entity_in_slot, size_t index, size_t size);
void changebullettype(int *bullet);
void clearbackground(FILE *gpu);
void fillbackground(FILE *gpu, const uint32_t *data, size_t x, size_t y);
void clearsprites(FILE *gpu);
void initializeentitylist(bool *is_entity_in_slot, size_t size);
void initializecursor(cursor_t *cursor);
void updatemouseandcursor(mouse_t *mouse, cursor_t *cursor);
void createnewentity(entity_t *entities, bool *is_entity_in_slot, double *count, double spawnEntitytime, double elapsed_time);
void updateentities(entity_t *entities, bool *is_entity_in_slot, cursor_t *cursor, bool has_shot, FILE *gpu, bool *collision, int bullet, timetracker_t *timer);
void renderhudsprites(FILE *gpu, cursor_t *cursor, bool collision, int bullet);
void handleuserinput(mouse_t *mouse, int *bullet, volatile bool *running);
void *mouse_thread_func(void *arg);
void *game_thread_func(void *arg);

volatile bool running = true; // Usado para controlar a execução das threads
pthread_mutex_t mouse_lock = PTHREAD_MUTEX_INITIALIZER;
mouse_t mouse;
cursor_t cursor;

FILE *gpu;

int bullet = 0;

/**
 * @author G03
 * @brief Main function of the game.
 *
 * This function initializes the GPU, mouse, cursor, and other game elements.
 * It also creates and manages the threads for mouse and game logic.
 *
 * @return 0 if the game runs successfully, -1 if an error occurs during initialization.
 */
int main(void) {
    gpu = NULL;

    if (gpuopen(&gpu, DRIVER_NAME) < 0) {
        printf("Error opening GPU driver\n");
        return -1;
    }

    mouse = mouseinit();

    if (mouse.fd < 0) {
        printf("Error opening mouse file\n");
        return -1;
    }

    initializecursor(&cursor);

    clearbackground(gpu);
    setbackground(gpu, 2, 1, 5);
    fillbackground(gpu, night_data[0], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT);

    pthread_t mouse_thread, game_thread;

    pthread_create(&mouse_thread, NULL, mouse_thread_func, NULL);
    pthread_create(&game_thread, NULL, game_thread_func, (void *)gpu);

    pthread_join(mouse_thread, NULL);
    pthread_join(game_thread, NULL);
    
    clearsprites(gpu);
    clearbackground(gpu);
    setbackground(gpu, 0, 0, 0);

    mouseclose(&mouse);
    gpuclose(&gpu);

    return 0;
}

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
    else if (cursor->position.x >= SCREEN_WIDTH) {
        cursor->position.x = SCREEN_WIDTH-1;
    }

    if (cursor->position.y < 0) {
        cursor->position.y = 0;
    }
    else if (cursor->position.y >= SCREEN_HEIGHT) {
        cursor->position.y = SCREEN_HEIGHT-1;
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

int createentity(entity_t *entities, bool *is_entity_in_slot, entity_t mob, size_t size)
{
    size_t i;

    for (i = 0; i < size && is_entity_in_slot[i]; i++) {}

    if (i == size)
        return -1;

    entities[i] = mob;
    is_entity_in_slot[i] = true;

    return (int) i;
}

void deleteentity(bool *is_entity_in_slot, size_t index, size_t size) 
{
    if (index >= size)
        return;

    is_entity_in_slot[index] = false;
}

void changebullettype(int *bullet) 
{
    *bullet += 1;

    if (*bullet >= 3) {
        *bullet = 0;
    }
}

void clearbackground(FILE *gpu)
{
    for (int i = 0; i < 60; i++) {
        for (int j = 0; j < 80; j++) {
            setbackgroundblock(gpu, j, i, 6, 7, 7);
        }
    }
}

void fillbackground(FILE *gpu, const uint32_t *data, size_t x, size_t y)
{
    int count_background = 0;
    rgba_t color;

    for (int i = 0; i < y; i++) {
        for (int j = 0; j < x; j++) {
            color = hextorgba(data[count_background++]);
            normalizergba(&color);

            if (color.a)
                setbackgroundblock(gpu, i, j, color.r, color.g, color.b);
        }
    }
}

void clearsprites(FILE *gpu)
{
    for (int i = 1; i < 32; i++) {
        setsprite(gpu, i, 0, 0, 0, 0);
    }
}

void initializeentitylist(bool *is_entity_in_slot, size_t size)
{
    for (int i = 0; i < size; i++) {
        is_entity_in_slot[i] = false;
    }
}

void initializecursor(cursor_t *cursor)
{
    cursor->position.x = 0;
    cursor->position.y = 0;
    cursor->previous_position.x = 0;
    cursor->previous_position.y = 0;
}

void updatemouseandcursor(mouse_t *mouse, cursor_t *cursor) {
    mouseread(mouse);
    updatemousesensitivity(mouse, 1, 1);
    updatecursor(cursor, mouse);
}

void createnewentity(entity_t *entities, bool *is_entity_in_slot, double *count, double spawnEntitytime, double elapsed_time) {
    entity_t example;

    if (*count < spawnEntitytime) {
        *count += elapsed_time;
    } else {
        example.type = rand() % 3;
        example.position.x = SCREEN_WIDTH;
        example.start_position.x = example.position.x;
        example.position.y = rand() % SCREEN_HEIGHT;
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

        int result = createentity(entities, is_entity_in_slot, example, MAX_ENTITIES);
        *count = 0;
    }
}

void updateentities(entity_t *entities, bool *is_entity_in_slot, cursor_t *cursor, bool has_shot, FILE *gpu, bool *collision, int bullet, timetracker_t *timer) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!is_entity_in_slot[i]) {
            continue;
        }

        if (checkcollision(cursor->position.x, cursor->position.y, entities[i].position.x, entities[i].position.y, 20, 20, entities[i].hitbox.size_x, entities[i].hitbox.size_y)) {
            *collision = true;

            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, entities[i].type * 2 + 1);

            if (has_shot && bullet == entities[i].type) {
                deleteentity(is_entity_in_slot, i, MAX_ENTITIES);
                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
                break;
            }
        } else {
            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, entities[i].type * 2);
        }

        updatevelocity(&entities[i].velocity.x, &entities[i].velocity.y, entities[i].acceleration.x, entities[i].acceleration.y, timer->elapsedTime);
        updateposition(&entities[i].position.x, &entities[i].position.y, entities[i].velocity.x, entities[i].velocity.y, timer->elapsedTime);

        if (entities[i].type == VAMPIRE && 
        ((entities[i].position.y > entities[i].start_position.y && entities[i].acceleration.y > 0) || 
        (entities[i].position.y < entities[i].start_position.y && entities[i].acceleration.y < 0))) {
            entities[i].acceleration.y = -entities[i].acceleration.y; 
        }

        if (entities[i].position.x < -20) {
            deleteentity(is_entity_in_slot, i, MAX_ENTITIES);
            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
            break;
        }
    }
}

void renderhudsprites(FILE *gpu, cursor_t *cursor, bool collision, int bullet) {
    if (collision) {
        setsprite(gpu, 1, 0, cursor->position.x, cursor->position.y, 20);
    } else {
        setsprite(gpu, 1, 1, cursor->position.x, cursor->position.y, 20);
    }

    setsprite(gpu, 3, 1, 619, 459, bullet * 2);
}

void handleuserinput(mouse_t *mouse, int *bullet, volatile bool *running) {
    if (mouse->right_button_released) {
        changebullettype(bullet);
    }

    if (mouse->middle_button_released) {
        running = false;
    }
}

void *mouse_thread_func(void *arg) {
    while (running) {
        pthread_mutex_lock(&mouse_lock);
        updatemouseandcursor(&mouse, &cursor);
        handleuserinput(&mouse, &bullet, &running);
        pthread_mutex_unlock(&mouse_lock);
        usleep(1000);
    }

    return NULL;
}

void *game_thread_func(void *arg) {
    timetracker_t timer;
    timer.lastTime = gettime();

    entity_t entities[MAX_ENTITIES];
    bool is_entity_in_slot[MAX_ENTITIES];
    initializeentitylist(is_entity_in_slot, MAX_ENTITIES);

    double spawnentitytime = 1;
    double count = 0;
    bool collision = false;
    bool running = true;

    while (running) {
        updatetime(&timer);
        createnewentity(entities, is_entity_in_slot, &count, spawnentitytime, timer.elapsedTime);
        pthread_mutex_lock(&mouse_lock);
        updateentities(entities, is_entity_in_slot, &cursor, mouse.left_button_clicked, gpu, &collision, bullet, &timer);
        renderhudsprites(gpu, &cursor, collision, bullet);
        pthread_mutex_unlock(&mouse_lock);

        collision = false;
    }

    return NULL;
}

