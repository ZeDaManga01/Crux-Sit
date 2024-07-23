#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <linux/time.h>
#include <pthread.h>
#include "../include/mouse/mouse.h"
#include "../include/colenda/colenda.h"
#include "../include/hexdecode/hexdecode.h"

#include "../assets/day.h"
#include "../assets/night.h"
#include "../assets/zombie.h"
#include "../assets/werewolf.h"
#include "../assets/vampire.h"
#include "../assets/aim.h"
#include "../assets/bullet.h"
#include "../assets/silver.h"
#include "../assets/garlic.h"

#define MAX_ENTITIES 20
#define MAX_BULLETTYPES 3
#define BULLET_OFFSET 5

#define ZOMBIE 0
#define WEREWOLF 1
#define VAMPIRE 2

#define NORMAL_BULLET 0
#define SILVER_BULLET 1
#define GARLIC_BULLET 2

#define BULLET_SPRITE_OFFSET 1
#define ENTITY_SPRITE_LAYER_OFFSET 3

#define ZOMBIE_OFFSET 4
#define WEREWOLF_OFFSET 7
#define VAMPIRE_OFFSET 10

typedef struct {
    double lastTime;
    double currentTime;
    double elapsedTime;
} timer_t;

typedef struct {
    int x, y;
    int size_x, size_y;
} rectangle_t;

typedef struct {
    int x, y;
} position_t;

typedef struct {
    int x, y;
} velocity_t;

typedef struct {
    int x, y;
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
    int x, y;
    int px, py;
} cursor_t;

double gettime();
void updatetime(timer_t *timer);
void fitinscreenborder(int *n, int limit);
void updateposition(int *x, int *y, int vx, int vy, double elapsedTime);
void updatevelocity(int *vx, int *vy, int ax, int ay, double elapsedTime);
void updatecursor(cursor_t *cursor, mouse_t *mouse);
int checkcollision(int x1, int y1, int x2, int y2, int size_x1, int size_y1, int size_x2, int size_y2);
int createentity(entity_t *entities, int *is_entity_in_slot, entity_t mob, size_t size);
void deleteentity(int *is_entity_in_slot, size_t index, size_t size);
void changebullettype(int *bullet);
void clearbackground(FILE *gpu);
void addsprite(FILE *gpu, const uint32_t *data, int reg, size_t width, size_t height);
void fillbackground(FILE *gpu, const uint32_t *data, size_t x, size_t y, uint32_t background_color);
void clearsprites(FILE *gpu);
void initializeentitylist(int *is_entity_in_slot, size_t size);
void initializecursor(cursor_t *cursor);
void updatemouseandcursor(mouse_t *mouse, cursor_t *cursor);
int createrandomentity(entity_t *entities, int *is_entity_in_slot);
void updateentities(FILE* gpu, entity_t *entities, int *is_entity_in_slot, timer_t *timer);
void rendercursor(FILE *gpu, cursor_t *cursor);
void handleuserinput(mouse_t *mouse, int *bullet, volatile int *running);
void *mouse_thread_func(void *arg);
void *game_thread_func(void *arg);

int running = 1; // Usado para controlar a execução das threads

int pause_mouse = 0;
int pause_game = 0;
int pause_entities = 0;
pthread_cond_t mouse_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t game_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t entities_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mouse_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t entities_lock = PTHREAD_MUTEX_INITIALIZER;
mouse_t mouse;
cursor_t cursor;

FILE *gpu;

int bullet = 0;
entity_t entities[MAX_ENTITIES];
int is_entity_in_slot[MAX_ENTITIES];

/**
 * @author G03
 * @brief Main function of the game.
 *
 * This function initializes the GPU, mouse, cursor, and other game elements.
 * It also creates and manages the threads for mouse and game logic.
 *
 * @return 0 if the game runs successfully, -1 if an error occurs during initialization.
 */
int main(void)
{
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

    srand(time(NULL));

    initializecursor(&cursor);
    initializeentitylist(is_entity_in_slot, MAX_ENTITIES);

    addsprite(gpu, aim_data[0], 0, AIM_FRAME_WIDTH, AIM_FRAME_HEIGHT);
    addsprite(gpu, bullet_data[0], 1, BULLET_FRAME_WIDTH, BULLET_FRAME_HEIGHT);
    addsprite(gpu, silver_data[0], 2, SILVER_FRAME_WIDTH, SILVER_FRAME_HEIGHT);
    addsprite(gpu, garlic_data[0], 3, GARLIC_FRAME_WIDTH, GARLIC_FRAME_HEIGHT);
    addsprite(gpu, zombie_data[0], 4, ZOMBIE_FRAME_WIDTH, ZOMBIE_FRAME_HEIGHT);
    addsprite(gpu, zombie_data[1], 5, ZOMBIE_FRAME_WIDTH, ZOMBIE_FRAME_HEIGHT);
    addsprite(gpu, zombie_data[2], 6, ZOMBIE_FRAME_WIDTH, ZOMBIE_FRAME_HEIGHT);
    addsprite(gpu, werewolf_data[0], 7, WEREWOLF_FRAME_WIDTH, WEREWOLF_FRAME_HEIGHT);
    addsprite(gpu, werewolf_data[1], 8, WEREWOLF_FRAME_WIDTH, WEREWOLF_FRAME_HEIGHT);
    addsprite(gpu, werewolf_data[2], 9, WEREWOLF_FRAME_WIDTH, WEREWOLF_FRAME_HEIGHT);
    addsprite(gpu, vampire_data[0], 10, VAMPIRE_FRAME_WIDTH, VAMPIRE_FRAME_HEIGHT);

    clearsprites(gpu);
    clearbackground(gpu);
    fillbackground(gpu, night_data[0], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, NIGHT_BACKGROUND_COLOR);

    pthread_t mouse_thread, game_thread;

    pthread_create(&mouse_thread, NULL, mouse_thread_func, NULL);
    pthread_create(&game_thread, NULL, game_thread_func, (void *)gpu);

    pthread_join(mouse_thread, NULL);
    pthread_join(game_thread, NULL);
    
    clearsprites(gpu);
    clearbackground(gpu);
    fillbackground(gpu, day_data[0], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, DAY_BACKGROUND_COLOR);

    mouseclose(&mouse);
    gpuclose(&gpu);

    return 0;
}

double gettime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void updatetime(timer_t *timer)
{
    timer->currentTime = gettime();
    timer->elapsedTime = timer->currentTime - timer->lastTime;
    timer->lastTime = timer->currentTime;
}

void fitinscreenborder(int *n, int limit)
{
    if (*n < 0)
    {
        *n = 0;
    }
    else if (*n >= limit)
    {
        *n = limit - 1;
    }
}

void updateposition(int *x, int *y, int vx, int vy, double elapsedTime)
{
    *x += (int) vx * elapsedTime;
    *y += (int) vy * elapsedTime;

    fitinscreenborder(y, SCREEN_HEIGHT);
}

void updatevelocity(int *vx, int *vy, int ax, int ay, double elapsedTime) {
    *vx += (int) ax * elapsedTime;
    *vy += (int) ay * elapsedTime;
}

void updatecursor(cursor_t *cursor, mouse_t *mouse)
{
    cursor->px = cursor->x;
    cursor->py = cursor->y;

    /* atualiza a posição do mouse */
    cursor->x += mouse->dx;
    cursor->y -= mouse->dy;

    /* condicionais para manter o cursor dentro da tela
    sem ele, o cursor iria "fugir", andando infinitamente para os cantos */
    fitinscreenborder(&cursor->x, SCREEN_WIDTH);
    fitinscreenborder(&cursor->y, SCREEN_HEIGHT);

    return;
}

inline int checkcollision(int x1, int y1, int x2, int y2, int size_x1, int size_y1, int size_x2, int size_y2)
{
	return x1 < (x2 + size_x2) && (x1 + size_x1) > x2 && y1 < (y2 + size_y2) && (y1 + size_y1) > y2;
}

int createentity(entity_t *entities, int *is_entity_in_slot, entity_t mob, size_t size)
{
    size_t i;

    for (i = 0; i < size && is_entity_in_slot[i]; i++) {}

    if (i == size)
        return -1;

    entities[i] = mob;
    is_entity_in_slot[i] = 1;

    return (int) i;
}

void deleteentity(int *is_entity_in_slot, size_t index, size_t size) 
{
    if (index >= size)
        return;

    is_entity_in_slot[index] = 0;
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

void addsprite(FILE *gpu, const uint32_t *data, int reg, size_t width, size_t height)
{
    rgba_t color;

    for (size_t j = 0; j < width * height; j++) {
        color = normalizergba(hextorgba(data[j]));

        if (color.a) {
            setspritememorywithreg(gpu, reg, j, color.r, color.g, color.b);
        } else {
            setspritememorywithreg(gpu, reg, j, 6, 7, 7);
        }
    }
}

void fillbackground(FILE *gpu, const uint32_t *data, size_t x, size_t y, uint32_t background_color)
{
    rgba_t bg_color = normalizergba(hextorgba(background_color));
    int count_background = 0;
    rgba_t color;

    for (size_t i = 0; i < y; i++) {
        for (size_t j = 0; j < x; j++) {
            color = normalizergba(hextorgba(data[count_background++]));

            if (color.a) {
                setbackgroundblock(gpu, j, i, color.r, color.g, color.b);
            } else {
                setbackgroundblock(gpu, j, i, 6, 7, 7);
            }
        }
    }
}

void clearsprites(FILE *gpu)
{
    for (size_t i = 1; i < 32; i++) {
        setsprite(gpu, i, 0, 0, 0, 0);
    }
}

void initializeentitylist(int *is_entity_in_slot, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        is_entity_in_slot[i] = 0;
    }
}

void initializecursor(cursor_t *cursor)
{
    cursor->x = 0;
    cursor->y = 0;
    cursor->px = 0;
    cursor->py = 0;
}

void updatemouseandcursor(mouse_t *mouse, cursor_t *cursor)
{
    mouseread(mouse);
    updatemousesensitivity(mouse, 1, 1);
    updatecursor(cursor, mouse);
}

int createrandomentity(entity_t *entities, int *is_entity_in_slot)
{
    entity_t example;

    example.type = rand() % 3;

    example.position.x = SCREEN_WIDTH;
    example.start_position.x = example.position.x;
    example.position.y = 0;
    example.start_position.y = example.position.y;
    example.velocity.x = 0;
    example.velocity.y = 0;
    example.acceleration.x = 0;
    example.acceleration.y = 0;
    example.hitbox.x = example.position.x - 5;
    example.hitbox.y = example.position.y - 5;
    example.hitbox.size_x = 30;
    example.hitbox.size_y = 30;

    switch (example.type) {
        case ZOMBIE:
            example.velocity.x = -(rand() % 100 + 50);
            example.position.y = rand() % SCREEN_HEIGHT + (SCREEN_HEIGHT / 2);
            break;

        case WEREWOLF:
            example.acceleration.x = -(rand() % 100 + 50);
            example.position.y = rand() % SCREEN_HEIGHT + (SCREEN_HEIGHT / 2);
            
            if (rand() % 2 == 1) {
                example.acceleration.x = -example.acceleration.x;
                example.velocity.x = -(rand() % 400 + 200);
            } else {
                example.velocity.x = -(rand() % 150 + 50);
            }
            break;

        case VAMPIRE:
            example.velocity.y = rand() % 200;
            example.acceleration.y = -example.velocity.y;
            example.position.y = rand() % SCREEN_HEIGHT + 50;
            break;
        default:
            return -1;
    }

    return createentity(entities, is_entity_in_slot, example, MAX_ENTITIES);
}

void updateentities(FILE *gpu, entity_t *entities, int *is_entity_in_slot, timer_t *timer)
{
    for (int i = 0; i < MAX_ENTITIES; i++) {
        pthread_mutex_lock(&entities_lock);
        while(pause_entities) {
            pthread_cond_wait(&entities_cond, &entities_lock);
        }
        pthread_mutex_unlock(&entities_lock);

        if (!is_entity_in_slot[i]) {
            continue;
        }

        pthread_mutex_lock(&entities_lock);
        updatevelocity(&entities[i].velocity.x, &entities[i].velocity.y, entities[i].acceleration.x, entities[i].acceleration.y, timer->elapsedTime);
        pthread_mutex_unlock(&entities_lock);

        pthread_mutex_lock(&entities_lock);
        updateposition(&entities[i].position.x, &entities[i].position.y, entities[i].velocity.x, entities[i].velocity.y, timer->elapsedTime);
        pthread_mutex_unlock(&entities_lock);

        switch (entities[i].type) {
            case ZOMBIE:
                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, ZOMBIE_OFFSET);
                break;

            case WEREWOLF:
                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, WEREWOLF_OFFSET);
                break;

            case VAMPIRE:
                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, VAMPIRE_OFFSET);
                break;
        }

        
        if (entities[i].type == VAMPIRE && 
        ((entities[i].position.y > entities[i].start_position.y && entities[i].acceleration.y > 0) || 
        (entities[i].position.y < entities[i].start_position.y && entities[i].acceleration.y < 0))) {
            pthread_mutex_lock(&entities_lock);
            entities[i].acceleration.y = -entities[i].acceleration.y;
            pthread_mutex_unlock(&entities_lock);
        }

        if (entities[i].position.x < 0) {
            pthread_mutex_lock(&entities_lock);
            deleteentity(is_entity_in_slot, i, MAX_ENTITIES);
            pthread_mutex_unlock(&entities_lock);
            
            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
        }
    }
}

void rendercursor(FILE *gpu, cursor_t *cursor)
{
    setsprite(gpu, 1, 1, cursor->x, cursor->y, 0);
}

void renderhudsprites(FILE *gpu, int bullet)
{
    setsprite(gpu, 2, 1, 619, 459, bullet + BULLET_OFFSET);
}

void killentity(entity_t *entities, int *is_entity_in_slot) 
{
    for (size_t i = 0; i < MAX_ENTITIES; i++) {
        if (!is_entity_in_slot[i]) {
            continue;
        }

        if (checkcollision(cursor.x, cursor.y, entities[i].position.x, entities[i].position.y, 20, 20, entities[i].hitbox.size_x, entities[i].hitbox.size_y) && bullet == entities[i].type) {
            pthread_mutex_lock(&entities_lock);
            pause_entities = 1;
            pthread_mutex_unlock(&entities_lock);     

            deleteentity(is_entity_in_slot, i, MAX_ENTITIES);

            pthread_mutex_lock(&entities_lock);
            pause_entities = 0;
            pthread_cond_signal(&entities_cond);
            pthread_mutex_unlock(&entities_lock);

            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
            break;
        }
    }
}

void handleuserinput(mouse_t *mouse, int *bullet, volatile int *running) {
    if (mouse->left_button_clicked) {
        killentity(entities, is_entity_in_slot);
    }

    if (mouse->right_button_clicked) {
        changebullettype(bullet);
    }

    if (mouse->middle_button_released) {
        *running = 0;
    }
}

void *mouse_thread_func(void *arg) {
    while (running) {
        pthread_mutex_lock(&mouse_lock);
        while(pause_mouse) {
            pthread_cond_wait(&mouse_cond, &mouse_lock);
        }
        pthread_mutex_unlock(&mouse_lock);

        updatemouseandcursor(&mouse, &cursor);
        printf("%d %d %d %d\n", cursor.x, cursor.y, mouse.dx, mouse.dy);
        handleuserinput(&mouse, &bullet, &running);
    }

    return NULL;
}

void *game_thread_func(void *arg) {
    timer_t timer;
    timer.lastTime = gettime();

    double count = 0;
    double spawnentitytime = 1;

    while (running) {
        pthread_mutex_lock(&game_lock);
        while(pause_game) {
            pthread_cond_wait(&game_cond, &game_lock);
        }
        pthread_mutex_unlock(&game_lock);

        updatetime(&timer);

        if (count > spawnentitytime) {
            createrandomentity(entities, is_entity_in_slot);
            count = 0;
        } else {
            count += timer.elapsedTime;
        }
        
        updateentities(gpu, entities, is_entity_in_slot, &timer);
        renderhudsprites(gpu, bullet);
        
        pthread_mutex_lock(&mouse_lock);
        pause_mouse = 1;
        pthread_mutex_unlock(&mouse_lock);
        
        rendercursor(gpu, &cursor);

        pthread_mutex_lock(&mouse_lock);
        pause_mouse = 0;
        pthread_cond_signal(&mouse_cond);
        pthread_mutex_unlock(&mouse_lock);

        usleep(10000);
    }

    return NULL;
}
