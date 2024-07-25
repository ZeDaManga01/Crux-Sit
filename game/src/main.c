#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include "../../driver/address_map_arm.h"
#include "../include/mouse/mouse.h"
#include "../include/colenda/colenda.h"
#include "../include/hexdecode/hexdecode.h"

#include "../assets/day.h"
#include "../assets/night.h"

#include "../assets/church.h"

#include "../assets/zombie.h"
#include "../assets/werewolf.h"
#include "../assets/vampire.h"
#include "../assets/aim.h"
#include "../assets/bullet.h"
#include "../assets/silver.h"
#include "../assets/garlic.h"

#define MAX_ENTITIES 20
#define MAX_BULLETTYPES 3
#define MAX_LIVES 5

#define ZOMBIE_POINTS 10
#define WEREWOLF_POINTS 30
#define VAMPIRE_POINTS 50

#define ZOMBIE 0
#define WEREWOLF 1
#define VAMPIRE 2

#define ZOMBIE_OFFSET 5
#define WEREWOLF_OFFSET 8
#define VAMPIRE_OFFSET 11
#define BULLET_OFFSET 2

#define NORMAL_BULLET 0
#define SILVER_BULLET 1
#define GARLIC_BULLET 2

#define BULLET_SPRITE_OFFSET 1
#define ENTITY_SPRITE_LAYER_OFFSET 3

#define LIVES_OFFSET 22

#define U_WAIT_TIME_AFTER_SHOOTING 100000

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

int getrandomnumber(int m, int n);
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
void updateentities(FILE* gpu, entity_t *entities, int *is_entity_in_slot, double period);
void killentity(entity_t *entities, int *is_entity_in_slot, cursor_t cursor);
void rendercursor(FILE *gpu, cursor_t cursor);
void handleuserinput(mouse_t *mouse, int *bullet, volatile int *running);
void *mouse_thread_func(void *arg);
void *game_thread_func(void *arg);
void *button_thread_func(void *arg);

int FPS;
double PERIOD;
int U_PERIOD;

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

FILE *gpu = NULL;

int bullet = 0;
entity_t entities[MAX_ENTITIES];
int is_entity_in_slot[MAX_ENTITIES];

int points = 0;
int night = 0;

int remaining_lives;

int entities_to_kill = 30;
int remaining_entities;

volatile int *KEY_ptr;

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
    FPS = 60;
    PERIOD = 1.0/FPS;
    U_PERIOD = PERIOD * 1000000;

    int fd = -1;

    if ((fd = open ("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf ("ERROR: could not open \"/dev/mem\"...\n");
        return -1;
    }

    void *LW_virtual;
    LW_virtual = mmap(NULL, LW_BRIDGE_SPAN, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE) ;

    if (LW_virtual == MAP_FAILED) {
        printf ("ERROR: mmap() failed...\n") ;
        close (fd) ;
        return (-1);
    }

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

    addsprite(gpu, aim_data[AIM_FRAME], 0, AIM_FRAME_WIDTH, AIM_FRAME_HEIGHT);
    addsprite(gpu, aim_data[SHOOT_FRAME], 1, AIM_FRAME_WIDTH, AIM_FRAME_HEIGHT);
    addsprite(gpu, bullet_data[0], 2, BULLET_FRAME_WIDTH, BULLET_FRAME_HEIGHT);
    addsprite(gpu, silver_data[0], 3, SILVER_FRAME_WIDTH, SILVER_FRAME_HEIGHT);
    addsprite(gpu, garlic_data[0], 4, GARLIC_FRAME_WIDTH, GARLIC_FRAME_HEIGHT);
    addsprite(gpu, zombie_data[0], 5, ZOMBIE_FRAME_WIDTH, ZOMBIE_FRAME_HEIGHT);
    addsprite(gpu, zombie_data[1], 6, ZOMBIE_FRAME_WIDTH, ZOMBIE_FRAME_HEIGHT);
    addsprite(gpu, zombie_data[2], 7, ZOMBIE_FRAME_WIDTH, ZOMBIE_FRAME_HEIGHT);
    addsprite(gpu, werewolf_data[0], 8, WEREWOLF_FRAME_WIDTH, WEREWOLF_FRAME_HEIGHT);
    addsprite(gpu, werewolf_data[1], 9, WEREWOLF_FRAME_WIDTH, WEREWOLF_FRAME_HEIGHT);
    addsprite(gpu, werewolf_data[2], 10, WEREWOLF_FRAME_WIDTH, WEREWOLF_FRAME_HEIGHT);
    addsprite(gpu, vampire_data[0], 11, VAMPIRE_FRAME_WIDTH, VAMPIRE_FRAME_HEIGHT);

    clearsprites(gpu);
    fillbackground(gpu, church[NIGHT_FRAME], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, NIGHT_BACKGROUND_COLOR);

    pthread_t mouse_thread, game_thread, button_thread;

    pthread_create(&mouse_thread, NULL, mouse_thread_func, NULL);
    pthread_create(&button_thread, NULL, button_thread_func, NULL);
    pthread_create(&game_thread, NULL, game_thread_func, (void *)gpu);

    pthread_join(mouse_thread, NULL);
    pthread_join(game_thread, NULL);
    
    if (remaining_lives) {
        clearsprites(gpu);
        // clearbackground(gpu);
        fillbackground(gpu, church[DAY_FRAME], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, DAY_BACKGROUND_COLOR);
    } else {
        // setbackground(gpu, 5, 0, 0);
        fillbackground(gpu, church[NO_REMAINING_LIVES_FRAME], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, NIGHT_BACKGROUND_COLOR);
        printf("Você perdeu :(\n");
    }

    mouseclose(&mouse);
    gpuclose(&gpu);
    close(fd);

    return 0;
}


inline int getrandomnumber(int m, int n) 
{
    if (m >= n) {
        return -1;
    }

    return rand() % (n - m) + m;
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
    fitinscreenborder(y, SCREEN_HEIGHT - 20);
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
    setbackground(gpu, bg_color.r, bg_color.g, bg_color.b);

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

    example.type = getrandomnumber(0, 4);

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
            example.velocity.x = -getrandomnumber(50, 101);
            example.position.y = (getrandomnumber(SCREEN_HEIGHT / 2 + 50, SCREEN_HEIGHT - 20));
            break;

        case WEREWOLF:
            example.position.y = (getrandomnumber(SCREEN_HEIGHT / 2 + 50, SCREEN_HEIGHT - 20));
            
            if (getrandomnumber(0, 2) == 1) {
                example.acceleration.x = getrandomnumber(30, 61);
                example.velocity.x = -getrandomnumber(300, 401);
            } else {
                example.acceleration.x = -getrandomnumber(20, 51);
                example.velocity.x = -getrandomnumber(100, 151);
            }
            break;

        case VAMPIRE:
            example.position.y = (getrandomnumber(100, SCREEN_HEIGHT - 20));
            example.start_position.y = example.position.y;
            example.velocity.x = -(getrandomnumber(100, 201));
            example.velocity.y = (getrandomnumber(300, 401));
            example.acceleration.y = -example.velocity.y;
            break;
        default:
            return -1;
    }

    return createentity(entities, is_entity_in_slot, example, MAX_ENTITIES);
}

void updateentities(FILE *gpu, entity_t *entities, int *is_entity_in_slot, double period)
{
    int pos;
    
    for (int i = 0; i < MAX_ENTITIES; i++) {
        pthread_mutex_lock(&entities_lock);
        while (pause_entities) {
            pthread_cond_wait(&entities_cond, &entities_lock);
        }

        if (!is_entity_in_slot[i]) {
            pthread_mutex_unlock(&entities_lock);
            continue;
        }

        // Atualização de velocidade
        updatevelocity(&entities[i].velocity.x, &entities[i].velocity.y, entities[i].acceleration.x, entities[i].acceleration.y, period);

        // Atualização de posição
        updateposition(&entities[i].position.x, &entities[i].position.y, entities[i].velocity.x, entities[i].velocity.y, period);
        pos = entities[i].position.x;

        switch (entities[i].type) {
            case ZOMBIE:
                if (pos % 100 < 25) {
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, ZOMBIE_OFFSET);
                } else if (pos % 100 < 50){
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, ZOMBIE_OFFSET + 1);
                } else if (pos % 100 < 75) {
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, ZOMBIE_OFFSET);
                } else {
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, ZOMBIE_OFFSET + 2);
                }
                break;

            case WEREWOLF:
                if (pos % 100 < 25) {
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, WEREWOLF_OFFSET);
                } else if (pos % 100 < 50){
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, WEREWOLF_OFFSET + 1);
                } else if (pos % 100 < 75) {
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, WEREWOLF_OFFSET);
                } else {
                    setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, WEREWOLF_OFFSET + 2);
                }
                break;

            case VAMPIRE:
                setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, VAMPIRE_OFFSET);
                break;
        }

        if (entities[i].type == VAMPIRE && \
            ((entities[i].acceleration.y < 0 && entities[i].position.y < entities[i].start_position.y) || \
            (entities[i].acceleration.y > 0 && entities[i].position.y > entities[i].start_position.y))) {
            
            entities[i].acceleration.y = -entities[i].acceleration.y;
        }

        if (entities[i].position.x < 0) {
            deleteentity(is_entity_in_slot, i, MAX_ENTITIES);
            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
            setsprite(gpu, LIVES_OFFSET + (remaining_lives - 1), 0, 0, 0, 0);
            remaining_lives--;
            printf("Você perdeu uma vida! Vidas restantes: %d\n", remaining_lives);
        }

        pthread_mutex_unlock(&entities_lock);
    }
}


void rendercursor(FILE *gpu, cursor_t cursor)
{
    setsprite(gpu, 1, 1, cursor.x, cursor.y, 0);
}

void renderhudsprites(FILE *gpu, int bullet)
{
    setsprite(gpu, 2, 1, 619, 459, bullet + BULLET_OFFSET);
}

void killentity(entity_t *entities, int *is_entity_in_slot, cursor_t cursor) 
{
    for (size_t i = 0; i < MAX_ENTITIES; i++) {
        if (!is_entity_in_slot[i]) {
            continue;
        }

        if (checkcollision(cursor.x, cursor.y, entities[i].position.x, entities[i].position.y, 20, 20, entities[i].hitbox.size_x, entities[i].hitbox.size_y) && bullet == entities[i].type) {
            pthread_mutex_lock(&entities_lock);
            pause_entities = 1;
            pthread_mutex_unlock(&entities_lock);

            pthread_mutex_lock(&game_lock);
            pause_game = 1;
            pthread_mutex_unlock(&game_lock); 

            setsprite(gpu, 1, 1, cursor.x, cursor.y, 1);
            deleteentity(is_entity_in_slot, i, MAX_ENTITIES);
            usleep(U_WAIT_TIME_AFTER_SHOOTING);

            pthread_mutex_lock(&entities_lock);
            pause_entities = 0;
            pthread_cond_signal(&entities_cond);
            pthread_mutex_unlock(&entities_lock);

            pthread_mutex_lock(&game_lock);
            pause_game = 0;
            pthread_mutex_unlock(&game_lock); 

            switch (entities[i].type) {
                case ZOMBIE:
                    points += ZOMBIE_POINTS;
                    break;
                case WEREWOLF:
                    points += WEREWOLF_POINTS;
                    break;
                case VAMPIRE:
                    points += VAMPIRE_POINTS;
                default:
                    break;
            }

            remaining_entities--;
            printf("Remaining: %d\n", remaining_entities);

            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
            break;
        }
    }
}

void handleuserinput(mouse_t *mouse, int *bullet, volatile int *running) {
    if (mouse->left_button_clicked) {
        killentity(entities, is_entity_in_slot, cursor);
    }

    if (mouse->right_button_clicked) {
        changebullettype(bullet);
    }

    if (mouse->middle_button_released) {
        *running = 0;
    }
}

void *button_thread_func(void *arg) {
    while (running) {
        
    }

    return NULL;
}

void *mouse_thread_func(void *arg) {
    while (running) {
        pthread_mutex_lock(&mouse_lock);
        while(pause_mouse) {
            pthread_cond_wait(&mouse_cond, &mouse_lock);
        }
        pthread_mutex_unlock(&mouse_lock);

        updatemouseandcursor(&mouse, &cursor);
        handleuserinput(&mouse, &bullet, &running);
    }

    return NULL;
}

void *game_thread_func(void *arg) {
    double count = 0;
    double spawnentitytime = 1;

    remaining_entities = entities_to_kill;
    remaining_lives = MAX_LIVES;

    for (int i = 0; i < MAX_LIVES; i++) {
        setsprite(gpu, i + LIVES_OFFSET, 1, i * 20, SCREEN_HEIGHT - 20, 15);
    }

    while (running) {
        pthread_mutex_lock(&game_lock);
        while(pause_game) {
            pthread_cond_wait(&game_cond, &game_lock);
        }
        pthread_mutex_unlock(&game_lock);

        if (!remaining_lives || !remaining_entities) {
            running = 0;
        }

        if (count > spawnentitytime) {
            createrandomentity(entities, is_entity_in_slot);
            count = 0;
        } else {
            count += PERIOD;
        }
        
        updateentities(gpu, entities, is_entity_in_slot, PERIOD);
        renderhudsprites(gpu, bullet);
        rendercursor(gpu, cursor);

        usleep(U_PERIOD);
    }

    return NULL;
}
