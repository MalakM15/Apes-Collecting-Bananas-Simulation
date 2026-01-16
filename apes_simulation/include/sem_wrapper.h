/*
 * sem_wrapper.h
 * Cross-platform semaphore wrapper
 * Uses named semaphores on macOS, unnamed on Linux
 */

#ifndef SEM_WRAPPER_H
#define SEM_WRAPPER_H

#include <semaphore.h>

#ifdef __APPLE__
    /* macOS: Use named semaphores */
    #define USE_NAMED_SEMAPHORES 1
    
    /* Store array of named semaphore pointers */
    extern sem_t* maze_lock_ptrs[50][50];
    extern sem_t* basket_lock_ptrs[10];
    extern sem_t* global_lock_ptr;
    
#else
    /* Linux: Use unnamed semaphores */
    #define USE_NAMED_SEMAPHORES 0
#endif

/*
 * Initialize semaphores for simulation
 * Returns 0 on success, -1 on failure
 */
int init_simulation_semaphores(void* shared_data, int num_families, int rows, int cols);

/*
 * Cleanup semaphores
 */
void cleanup_simulation_semaphores(int num_families, int rows, int cols);

/*
 * Semaphore operations wrappers
 */
int sem_wait_wrapper(sem_t* sem, int row, int col, int is_maze);
int sem_post_wrapper(sem_t* sem, int row, int col, int is_maze);
int sem_wait_basket(sem_t* sem, int family_id);
int sem_post_basket(sem_t* sem, int family_id);
int sem_wait_global(sem_t* sem);
int sem_post_global(sem_t* sem);

#endif /* SEM_WRAPPER_H */
