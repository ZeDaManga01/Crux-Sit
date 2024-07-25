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
#include "../include/fpga/fpga.h"

#include "../assets/day.h"
#include "../assets/night.h"

#include "../assets/main_menu_realistic.h"
#include "../assets/church.h"

#include "../assets/zombie.h"
#include "../assets/werewolf.h"
#include "../assets/vampire.h"
#include "../assets/aim.h"
#include "../assets/bullet.h"
#include "../assets/silver.h"
#include "../assets/garlic.h"

#define TRUE 1
#define FALSE 0

#define CURSOR_INITIALIZER { 0, 0, 0, 0 }

#define MAX_ENTITIES 20
#define MAX_BULLETTYPES 3
#define MAX_LIVES 5

#define ZOMBIE_POINTS 100
#define WEREWOLF_POINTS 300
#define VAMPIRE_POINTS 500

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
void fitinborder(int *n, int limit);
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
void updatemouseandcursor(mouse_t *mouse, cursor_t *cursor);
int createrandomentity(entity_t *entities, int *is_entity_in_slot);
void updateentities(FILE* gpu, entity_t *entities, int *is_entity_in_slot, double period);
void killentity(entity_t *entities, int *is_entity_in_slot, cursor_t cursor);
void rendercursor(FILE *gpu, cursor_t cursor);
void handleuserinput(mouse_t *mouse, int *bullet, volatile int *running);
void numbertohex(fpga_map_arm_t fpga_map, int number);
void *mouse_thread_func(void *arg);
void *game_thread_func(void *arg);
void *button_thread_func(void *arg);

// Used to control the frequency of the game
int FPS;
double PERIOD;
int U_PERIOD;

int running = TRUE; // Used to control the execution of the threads

int pause_mouse = FALSE;
int pause_game = FALSE;
int pause_entities = FALSE;

pthread_cond_t mouse_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t game_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t entities_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mouse_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t entities_lock = PTHREAD_MUTEX_INITIALIZER;

FILE *gpu = NULL;
mouse_t mouse;
cursor_t cursor = CURSOR_INITIALIZER;
fpga_map_arm_t fpga_map = FPGA_MAP_ARM_INITIALIZER;

int bullet = 0;
entity_t entities[MAX_ENTITIES];
int is_entity_in_slot[MAX_ENTITIES];

int points = 0;
int night = 0;

int remaining_lives;

int entities_to_kill = 40;
int remaining_entities;

volatile int pause_code = FALSE;
volatile int stage = 0;  

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
    if (gpuopen(&gpu, DRIVER_NAME) < 0) {
        printf("Error opening GPU driver\n");
        return -1;
    }

    mouse = mouseinit();

    if (mouse.fd < 0) {
        printf("Error opening mouse file\n");
        return -1;
    }

    if (fpgainit(&fpga_map) < 0) {
        printf("Error initializing FPGA\n");
        return -1;
    }

    srand(time(NULL));

    FPS = 60;
    PERIOD = 1.0/FPS;
    U_PERIOD = PERIOD * 1000000;

    // Add the sprites to the sprite memory
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

    pthread_t mouse_thread, game_thread, button_thread;

    pthread_create(&mouse_thread, NULL, mouse_thread_func, NULL);
    pthread_create(&button_thread, NULL, button_thread_func, NULL);
    pthread_create(&game_thread, NULL, game_thread_func, (void *)gpu);

    pthread_join(mouse_thread, NULL);
    pthread_join(button_thread, NULL);
    pthread_join(game_thread, NULL);

    mouseclose(&mouse);
    gpuclose(&gpu);
    fpgaclose(&fpga_map);

    return 0;
}

/**
 * @brief Generates a random number within the specified range.
 *
 * This function generates a random number using the rand() function.
 * If the lower bound (m) is greater than or equal to the upper bound (n),
 * the function returns -1. Otherwise, it returns a random number within the specified range.
 *
 * @param m The lower bound of the range.
 * @param n The upper bound of the range.
 *
 * @return A random number within the specified range, or -1 if the lower bound is greater than or equal to the upper bound.
 */
inline int getrandomnumber(int m, int n) 
{
    if (m >= n)
        return -1;

    return rand() % (n - m) + m;
}

/**
 * @brief Adjusts the value of a variable to fit within the specified range.
 *
 * This function checks if the value of the given variable is less than zero or greater than or equal to the specified limit.
 * If the value is less than zero, it sets the value to zero. If the value is greater than or 
 * equal to the limit, it sets the value to one less than the limit.
 *
 * @param n A pointer to the integer variable to be adjusted.
 * @param limit An integer representing the upper limit of the range.
 *
 * @return void
 */
void fitinborder(int *n, int limit)
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

/**
 * @brief Updates the position of an entity in the game.
 *
 * This function takes the current position and velocity of the entity, as well as the elapsed time since the last update.
 * It then calculates the new position by adding the product of the velocity and elapsed time to the current position.
 * Finally, it ensures that the entity stays within the screen boundaries by calling the fitinborder function.
 *
 * @param x A pointer to the integer representing the x-coordinate of the entity's position.
 * @param y A pointer to the integer representing the y-coordinate of the entity's position.
 * @param vx An integer representing the x-component of the entity's velocity.
 * @param vy An integer representing the y-component of the entity's velocity.
 * @param elapsed_time A double representing the time elapsed since the last update.
 *
 * @return void
 */
void updateposition(int *x, int *y, int vx, int vy, double elapsed_time)
{
    *x += (int) vx * elapsed_time;
    *y += (int) vy * elapsed_time;
    fitinborder(y, SCREEN_HEIGHT - 20);
}

/**
 * @brief Updates the velocity of an entity based on acceleration and elapsed time.
 *
 * This function calculates the new velocity of an entity by adding the product 
 * of acceleration and elapsed time to the current velocity.
 *
 * @param vx A pointer to the x-component of the entity's velocity.
 * @param vy A pointer to the y-component of the entity's velocity.
 * @param ax The x-component of the entity's acceleration.
 * @param ay The y-component of the entity's acceleration.
 * @param elapsedTime The time period between updates.
 *
 * @return void
 */
void updatevelocity(int *vx, int *vy, int ax, int ay, double elapsed_time) {
    *vx += (int) ax * elapsed_time;
    *vy += (int) ay * elapsed_time;
}

/**
 * @brief Updates the position and cursor based on mouse input.
 *
 * This function reads the mouse input, updates the mouse sensitivity, and then updates the cursor position.
 *
 * @param cursor A pointer to the cursor_t structure representing the player's cursor position.
 * @param mouse A pointer to the mouse_t structure containing mouse input data.
 *
 * @return void
 */
void updatecursor(cursor_t *cursor, mouse_t *mouse)
{
    // Set the previous cursor position before updating the cursor current position
    cursor->px = cursor->x;
    cursor->py = cursor->y;

    // Update the cursor position based on mouse displacement
    cursor->x += mouse->dx;
    cursor->y -= mouse->dy;

    // Adjust the cursor position to fit inside the screen
    fitinborder(&cursor->x, SCREEN_WIDTH);
    fitinborder(&cursor->y, SCREEN_HEIGHT);

    return;
}

/**
 * @brief Checks for a collision between two rectangles.
 *
 * This function takes the coordinates and dimensions of two rectangles and returns true if they intersect,
 * and false otherwise.
 *
 * @param x1 The x-coordinate of the top-left corner of the first rectangle.
 * @param y1 The y-coordinate of the top-left corner of the first rectangle.
 * @param x2 The x-coordinate of the top-left corner of the second rectangle.
 * @param y2 The y-coordinate of the top-left corner of the second rectangle.
 * @param size_x1 The width of the first rectangle.
 * @param size_y1 The height of the first rectangle.
 * @param size_x2 The width of the second rectangle.
 * @param size_y2 The height of the second rectangle.
 *
 * @return True if the rectangles intersect, false otherwise.
 */
inline int checkcollision(int x1, int y1, int x2, int y2, int size_x1, int size_y1, int size_x2, int size_y2)
{
	return x1 < (x2 + size_x2) && (x1 + size_x1) > x2 && y1 < (y2 + size_y2) && (y1 + size_y1) > y2;
}

/**
 * @brief Creates an entity and adds it to the game.
 *
 * This function iterates through the entity list to find an inactive entity.
 * If an inactive entity is found, it sets the entity's properties and marks it as active.
 * If no inactive entity is found, it returns -1.
 *
 * @param entities An array of entity_t structures representing the game's entities.
 * @param is_entity_in_slot An array of integers representing the status of each entity (1 for active, 0 for inactive).
 * @param mob An entity_t structure representing the entity to be created.
 * @param size The size of the is_entity_in_slot array.
 *
 * @return The index of the created entity in the entities array, or -1 if the entity type is invalid.
 */
int createentity(entity_t *entities, int *is_entity_in_slot, entity_t mob, size_t size)
{
    size_t i;

    for (i = 0; i < size && is_entity_in_slot[i]; i++) {}

    if (i == size)
        return -1;

    entities[i] = mob;
    is_entity_in_slot[i] = TRUE;

    return (int) i;
}

/**
 * @brief Deletes an entity from the game.
 *
 * This function sets the status of the specified entity to inactive (0) in the is_entity_in_slot array.
 * If the index is out of bounds, the function returns without making any changes.
 *
 * @param is_entity_in_slot An array of integers representing the status of each entity (1 for active, 0 for inactive).
 * @param index The index of the entity to be deleted.
 * @param size The size of the is_entity_in_slot array.
 *
 * @return void
 */
void deleteentity(int *is_entity_in_slot, size_t index, size_t size) 
{
    if (index >= size)
        return;

    is_entity_in_slot[index] = FALSE;
}

/**
 * @brief Changes the type of bullet selected by the player.
 *
 * This function increments the current bullet type by 1. If the bullet type exceeds the maximum allowed value (3),
 * it resets the bullet type to 0.
 *
 * @param bullet A pointer to an integer representing the current type of bullet selected by the player.
 *
 * @return void
 */
void changebullettype(int *bullet) 
{
    (*bullet)++;

    if (*bullet >= 3) {
        *bullet = 0;
    }
}

/**
 * @brief Clears the background of the game with the specified color.
 *
 * This function iterates through the background blocks and sets each block to the specified color.
 * The color is represented by the RGB values (6, 7, 7), which corresponds to an invisible color.
 *
 * @param gpu A pointer to the FILE structure representing the GPU.
 *
 * @return void
 */
void clearbackground(FILE *gpu)
{
    setbackground(gpu, 0, 0, 0);

    for (int i = 0; i < 60; i++) {
        for (int j = 0; j < 80; j++) {
            setbackgroundblock(gpu, j, i, 6, 7, 7);
        }
    }
}

/**
 * @brief Adds a sprite to the GPU's sprite memory using the provided image data.
 *
 * This function iterates through the provided image data and sets the sprite memory registers
 * with the corresponding RGB values. If the alpha value of the color is not zero, it sets the
 * sprite memory register with the corresponding RGB values. If the alpha value is zero, it sets
 * the sprite memory register to invisible (6, 7, 7).
 *
 * @param gpu A pointer to the FILE structure representing the GPU.
 * @param data A pointer to an array of uint32_t representing the image data.
 * @param reg An integer representing the sprite memory register to be used.
 * @param width A size_t representing the width of the sprite.
 * @param height A size_t representing the height of the sprite.
 *
 * @return void
 */
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

/**
 * @brief Fills the background of the game with the provided image data.
 *
 * This function iterates through the provided image data and sets the background color and block color accordingly.
 * If the alpha value of the color is not zero, it sets the background block color to the corresponding RGB values.
 * If the alpha value is zero, it sets the background block color to invisible (6, 7, 7).
 *
 * @param gpu A pointer to the FILE structure representing the GPU.
 * @param data A pointer to an array of uint32_t representing the image data.
 * @param x The width of the background image.
 * @param y The height of the background image.
 * @param background_color A uint32_t representing the background color in hexadecimal format.
 *
 * @return void
 */
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

/**
 * @brief Clears the sprite memory on the GPU.
 *
 * This function iterates through the sprite memory registers from 1 to 31 and sets each sprite to be invisible.
 * This is done by calling the setsprite function with the appropriate parameters.
 *
 * @param gpu A pointer to the FILE structure representing the GPU.
 *
 * @return void
 */
void clearsprites(FILE *gpu)
{
    for (size_t i = 1; i < 32; i++) {
        setsprite(gpu, i, FALSE, 0, 0, 0);
    }
}

/**
 * @brief Initializes the entity list by setting all entities to inactive.
 *
 * This function iterates through the entity list and sets the status of each entity to inactive (0).
 * This is done by initializing each element of the is_entity_in_slot array to 0.
 *
 * @param is_entity_in_slot An array of integers representing the status of each entity (1 for active, 0 for inactive).
 * @param size The size of the is_entity_in_slot array.
 *
 * @return void
 */
void initializeentitylist(int *is_entity_in_slot, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        is_entity_in_slot[i] = FALSE;
    }
}

/**
 * @brief Updates the position and cursor based on mouse input.
 *
 * This function reads the mouse input, updates the mouse sensitivity, and then updates the cursor position.
 *
 * @param mouse A pointer to the mouse_t structure containing mouse input data.
 * @param cursor A pointer to the cursor_t structure representing the player's cursor position.
 *
 * @return void
 */
void updatemouseandcursor(mouse_t *mouse, cursor_t *cursor)
{
    mouseread(mouse);
    updatemousesensitivity(mouse, 1, 1);
    updatecursor(cursor, mouse);
}

/**
 * @brief Creates a random entity and adds it to the game.
 *
 * This function generates a random entity based on the type, position, velocity, and acceleration.
 * It then adds the entity to the game by calling the createentity function.
 *
 * @param entities An array of entity_t structures representing the game's entities.
 * @param is_entity_in_slot An array of integers representing the status of each entity (1 for active, 0 for inactive).
 *
 * @return The index of the created entity in the entities array, or -1 if the entity type is invalid.
 */
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

/**
 * @brief Updates the position and sprite of entities in the game.
 *
 * This function iterates through the entities array, updates their position and sprite based on their type,
 * and handles entity destruction when necessary. It also handles the movement and animation of the entities.
 *
 * @param gpu A pointer to the FILE structure representing the GPU.
 * @param entities An array of entity_t structures representing the game's entities.
 * @param is_entity_in_slot An array of integers representing the status of each entity (1 for active, 0 for inactive).
 * @param period A double representing the time period between updates.
 *
 * @return void
 */
void updateentities(FILE *gpu, entity_t *entities, int *is_entity_in_slot, double period)
{
    for (int i = 0; i < MAX_ENTITIES; i++) {
        // When a player destroys an entity, it has priority over this function check
        // Thus, this pause variable is used in case the user presses the left mouse button
        pthread_mutex_lock(&entities_lock);
        while (pause_entities) {
            pthread_cond_wait(&entities_cond, &entities_lock);
        }

        // If the entity is inactive, unlock the mutex and continue
        if (!is_entity_in_slot[i]) {
            pthread_mutex_unlock(&entities_lock);
            continue;
        }

        // Velocity update
        updatevelocity(&entities[i].velocity.x, &entities[i].velocity.y, entities[i].acceleration.x, entities[i].acceleration.y, period);

        // Position update
        updateposition(&entities[i].position.x, &entities[i].position.y, entities[i].velocity.x, entities[i].velocity.y, period);
        
        int pos = entities[i].position.x;

        // Precompute sprite offset
        int sprite_offset;
        switch (entities[i].type) {
            case ZOMBIE:
                sprite_offset = (pos % 100 < 25) ? ZOMBIE_OFFSET :
                                (pos % 100 < 50) ? ZOMBIE_OFFSET + 1 :
                                (pos % 100 < 75) ? ZOMBIE_OFFSET :
                                                   ZOMBIE_OFFSET + 2;
                break;

            case WEREWOLF:
                sprite_offset = (pos % 100 < 25) ? WEREWOLF_OFFSET :
                                (pos % 100 < 50) ? WEREWOLF_OFFSET + 1 :
                                (pos % 100 < 75) ? WEREWOLF_OFFSET :
                                                   WEREWOLF_OFFSET + 2;
                break;

            case VAMPIRE:
                sprite_offset = VAMPIRE_OFFSET;
                break;
        }

        // Set sprite
        setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 1, entities[i].position.x, entities[i].position.y, sprite_offset);

        // Specific logic for VAMPIRE
        if (entities[i].type == VAMPIRE &&
            ((entities[i].acceleration.y < 0 && entities[i].position.y < entities[i].start_position.y) ||
             (entities[i].acceleration.y > 0 && entities[i].position.y > entities[i].start_position.y))) {
            entities[i].acceleration.y = -entities[i].acceleration.y;
        }

        // Entity removal logic
        if (entities[i].position.x < 0) {
            deleteentity(is_entity_in_slot, i, MAX_ENTITIES);
            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, FALSE, 0, 0, 0);
            setsprite(gpu, LIVES_OFFSET + (remaining_lives - 1), FALSE, 0, 0, 0);
            remaining_lives--;
        }

        pthread_mutex_unlock(&entities_lock);
    }
}

/**
 * @brief Renders the cursor sprite on the GPU.
 *
 * This function sets the sprite on the GPU to display the cursor at the specified position.
 *
 * @param gpu A pointer to the FILE structure representing the GPU.
 * @param cursor A cursor_t structure representing the player's cursor position.
 *
 * @return void
 */
void rendercursor(FILE *gpu, cursor_t cursor)
{
    setsprite(gpu, 1, TRUE, cursor.x, cursor.y, 0);
}

/**
 * @brief Renders the HUD sprites for the game.
 *
 * This function sets the sprite on the GPU to display the current type of bullet
 * selected by the player. The bullet type is represented by the 'bullet' parameter,
 * which is added to the BULLET_OFFSET to determine the correct sprite position.
 *
 * @param gpu A pointer to the FILE structure representing the GPU.
 * @param bullet An integer representing the type of bullet selected by the player.
 *
 * @return void
 */
void renderhudsprites(FILE *gpu, int bullet)
{
    setsprite(gpu, 2, TRUE, 619, 459, bullet + BULLET_OFFSET);
}

/**
 * @brief Handles the destruction of entities based on user input.
 *
 * This function checks for collisions between the cursor and entities. If a collision is detected,
 * the corresponding entity is destroyed, the game's points are updated, and the remaining entities
 * count is decremented.
 *
 * @param entities An array of entity_t structures representing the game's entities.
 * @param is_entity_in_slot An array of integers representing the status of each entity (1 for active, 0 for inactive).
 * @param cursor A cursor_t structure representing the player's cursor position.
 */
void killentity(entity_t *entities, int *is_entity_in_slot, cursor_t cursor) 
{
    for (size_t i = 0; i < MAX_ENTITIES; i++) {
        if (!is_entity_in_slot[i]) {
            continue;
        }

        if (checkcollision(cursor.x, cursor.y, 
            entities[i].position.x, entities[i].position.y, 
            20, 20, entities[i].hitbox.size_x, entities[i].hitbox.size_y) && 
            bullet == entities[i].type) {
            pthread_mutex_lock(&entities_lock);
            pause_entities = 1;
            pthread_mutex_unlock(&entities_lock);

            pthread_mutex_lock(&game_lock);
            pause_game = 1;
            pthread_mutex_unlock(&game_lock); 

            setsprite(gpu, 1, 1, cursor.x, cursor.y, 1);
            usleep(100000);

            pthread_mutex_lock(&entities_lock);
            pause_entities = 0;
            pthread_cond_signal(&entities_cond);
            pthread_mutex_unlock(&entities_lock);

            pthread_mutex_lock(&game_lock);
            pause_game = 0;
            pthread_cond_signal(&game_cond);
            pthread_mutex_unlock(&game_lock); 

            remaining_entities--;
            deleteentity(is_entity_in_slot, i, MAX_ENTITIES);

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

            setsprite(gpu, i + ENTITY_SPRITE_LAYER_OFFSET, 0, 0, 0, 0);
            break;
        }
    }
}

void numbertohex(fpga_map_arm_t fpga_map, int number)
{
    for (int i = 0; i < 6; i++)
    {
        setdigit(fpga_map, number % 10, i);
        number /= 10;
    }
}

void cleardisplay(fpga_map_arm_t fpga_map)
{
    for (int i = 0; i < 6; i++)
    {
        setdigit(fpga_map, -1, i);
    }
}

/**
 * @brief Handles user input related to the game.
 *
 * This function checks for specific mouse button clicks and performs corresponding actions.
 * If the left button is clicked, it calls the killentity function to handle entity destruction.
 * If the right button is clicked, it changes the type of bullet.
 * If the middle button is released, it stops the game by setting the running variable to 0.
 *
 * @param mouse A pointer to the mouse_t structure containing mouse input data.
 * @param bullet A pointer to the integer representing the type of bullet.
 * @param running A pointer to the integer representing the game's running state.
 */
void handleuserinput(mouse_t *mouse, int *bullet, volatile int *running) {
    if (mouse->left_button_clicked) {
        killentity(entities, is_entity_in_slot, cursor);
    }

    if (mouse->right_button_clicked) {
        changebullettype(bullet);
    }
}

// Placeholder for the button thread
void *button_thread_func(void *arg) {
    size_t size = 4;
    int keys_state[size];
    int previous_keys[size];
    int keys_pressed[size];

    readkeys(fpga_map, keys_state, size);

    while (running) {
        for (size_t i = 0; i < size; i++) {
            keys_pressed[i] = !previous_keys[i] && keys_state[i];
            previous_keys[i] = keys_state[i];
        }

        if (keys_pressed[0]) {
            switch (stage) {
                case 0:
                    stage = 1;
                    break;
                case 1:
                    stage = 0;
                    break;
                default:
                    stage = 0;
                    break;
            }
        }

        if (keys_pressed[1] == TRUE && pause_code == FALSE) {
            pthread_mutex_lock(&game_lock);
            pause_game = TRUE;
            pthread_mutex_unlock(&game_lock);

            pthread_mutex_lock(&mouse_lock);
            pause_mouse = TRUE;
            pthread_mutex_unlock(&mouse_lock);

            pause_code = TRUE;
        } else if (keys_pressed[1] == TRUE && pause_code == TRUE) {
            pthread_mutex_lock(&game_lock);
            pause_game = FALSE;
            pthread_mutex_unlock(&game_lock);
            pthread_cond_signal(&game_cond);

            pthread_mutex_lock(&mouse_lock);
            pause_mouse = FALSE;
            pthread_mutex_unlock(&mouse_lock);
            pthread_cond_signal(&mouse_cond);

            pause_code = FALSE;
        }

        if (keys_pressed[2]) {
            pthread_mutex_lock(&game_lock);
            pause_game = FALSE;
            pthread_mutex_unlock(&game_lock);
            pthread_cond_signal(&game_cond);

            pthread_mutex_lock(&mouse_lock);
            pause_mouse = FALSE;
            pthread_mutex_unlock(&mouse_lock);
            pthread_cond_signal(&mouse_cond);

            pause_code = FALSE;

            running = FALSE;
        }

        readkeys(fpga_map, keys_state, size);
    }

    

    return NULL;
}

/**
 * @brief The main thread function for handling mouse input.
 *
 * This function continuously reads mouse input, updates the cursor position,
 * handles user input, and manages the game's flow control.
 *
 * @param arg A void pointer to any necessary arguments. In this case, it's not used.
 *
 * @return A void pointer, which is also not used in this function.
 */
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

/**
 * @brief The main game loop function.
 *
 * This function is responsible for managing the game's state, including updating entities, rendering sprites,
 * handling user input, and managing the game's flow control.
 *
 * @param arg A void pointer to any necessary arguments. In this case, it's not used.
 *
 * @return A void pointer, which is also not used in this function.
 */
void *game_thread_func(void *arg) {
    double count = 0;
    double spawnentitytime = 1;

    cleardisplay(fpga_map);

    while (running) {
        clearsprites(gpu);

        switch (stage) {
            case 0:
                fillbackground(gpu, main_menu_realistic[0], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, 0);
                while (stage == 0 && running == TRUE) {
                    usleep(U_PERIOD);
                }
                break;

            case 1:
                for (int i = 0; i < MAX_LIVES; i++) {
                    setsprite(gpu, i + LIVES_OFFSET, 1, i * 20, SCREEN_HEIGHT - 20, 15);
                }
                fillbackground(gpu, church[NIGHT_FRAME], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, NIGHT_BACKGROUND_COLOR);
                initializeentitylist(is_entity_in_slot, MAX_ENTITIES);

                count = 0;
                remaining_entities = entities_to_kill;
                remaining_lives = MAX_LIVES;
                points = 0;

                while (stage == 1 && running == TRUE) {
                    pthread_mutex_lock(&game_lock);
                    while (pause_game) {
                        pthread_cond_wait(&game_cond, &game_lock);
                    }
                    pthread_mutex_unlock(&game_lock);

                    if (!remaining_entities) {
                        stage = 2;
                        break;
                    }

                    if (!remaining_lives) {
                        stage = 3;
                        break;
                    }

                    numbertohex(fpga_map, points);

                    if (count > spawnentitytime) {
                        createrandomentity(entities, is_entity_in_slot);
                        count = 0;
                        spawnentitytime = 1;
                    } else {
                        count += PERIOD;
                    }
                    
                    updateentities(gpu, entities, is_entity_in_slot, PERIOD);
                    renderhudsprites(gpu, bullet);
                    rendercursor(gpu, cursor);
                    usleep(U_PERIOD);
                }
                break;
            case 2:
                fillbackground(gpu, church[DAY_FRAME], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, DAY_BACKGROUND_COLOR);
                setsprite(gpu, 1, FALSE, 0, 0, 0);
                sleep(3);
                stage = 0;
                break;
            case 3:
                fillbackground(gpu, church[NO_REMAINING_LIVES_FRAME], DIVIDED_SCREEN_WIDTH, DIVIDED_SCREEN_HEIGHT, NIGHT_BACKGROUND_COLOR);
                sleep(3);
                stage = 0;
                break;
            default:
                running = FALSE;
                break;
        }
    }

    cleardisplay(fpga_map);
    clearsprites(gpu);
    clearbackground(gpu);

    return NULL;
}
