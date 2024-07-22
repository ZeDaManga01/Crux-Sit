#include <mouse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *mouse_thread_func(void *arg);
void *test_thread_func(void *arg);

volatile bool running = true;

pthread_mutex_t mouse_lock = PTHREAD_MUTEX_INITIALIZER;
mouse_t mouse;

int main(void)
{
    mouse = mouseinit();

    if (mouse.fd < 0) {
        printf("Erro\n");
        return -1;
    }

    pthread_t mouse_thread, test_thread;

    pthread_create(&mouse_thread, NULL, mouse_thread_func, NULL);
    pthread_create(&test_thread, NULL, test_thread_func, NULL);
    pthread_join(mouse_thread, NULL);
    pthread_join(test_thread, NULL);
    
    return 0;
}

void *mouse_thread_func(void *arg)
{
    while (running) {
        pthread_mutex_lock(&mouse_lock);
        mouseread(&mouse);
        updatemousesensitivity(&mouse, 1, 1);
        pthread_mutex_unlock(&mouse_lock);

        if (mouse.event) {
            printf("Mouse move: dx=%d, dy=%d\n", mouse.dx, mouse.dy);
            if (mouse.left_button_clicked) {
                printf("Left button pressed\n");
            }
            if (mouse.left_button_released) {
                printf("Left button released\n");
            }
            if (mouse.left_button_state) {
                printf("Left button held\n");
            }
            if (mouse.right_button_clicked) {
                printf("Right button pressed\n");
            }
            if (mouse.right_button_released) {
                printf("Right button released\n");
            }
            if(mouse.right_button_state) {
                printf("Right button held\n");
            }
            if (mouse.middle_button_clicked) {
                printf("Middle button pressed\n");
            }
            if (mouse.middle_button_released) {
                running = false;
            }
            if (mouse.middle_button_state) {
                printf("Middle button held\n");
            }
        }
        usleep(1000);
    }
    return NULL;
}

void *test_thread_func(void *arg)
{
    while (running) {
        usleep(5000000);
        pthread_mutex_lock(&mouse_lock);
        printf("Locking mouse thread.\n");
        usleep(5000000);
        pthread_mutex_unlock(&mouse_lock);
        printf("Mouse thread unlocked.\n");
    }
    return NULL;
}