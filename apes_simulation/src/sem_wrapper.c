/*
 * sem_wrapper.c
 * Cross-platform semaphore implementation
 */

#include "local.h"

#ifdef __APPLE__
/* Named semaphore storage for macOS */
sem_t* maze_lock_ptrs[MAX_ROWS][MAX_COLS];
sem_t* basket_lock_ptrs[MAX_FAMILIES];
sem_t* global_lock_ptr = NULL;

int init_simulation_semaphores(void* shared_data_ptr, int num_families, int rows, int cols) {
    char sem_name[64];
    int i, j;
    
    (void)shared_data_ptr;  /* Unused on macOS */
    
    /* Initialize global lock */
    snprintf(sem_name, sizeof(sem_name), "/apes_global_%d", getpid());
    sem_unlink(sem_name);  /* Remove if exists from previous run */
    global_lock_ptr = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
    if (global_lock_ptr == SEM_FAILED) {
        perror("Failed to create global semaphore");
        return -1;
    }
    
    /* Initialize basket locks */
    for (i = 0; i < num_families; i++) {
        snprintf(sem_name, sizeof(sem_name), "/apes_basket_%d_%d", getpid(), i);
        sem_unlink(sem_name);
        basket_lock_ptrs[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
        if (basket_lock_ptrs[i] == SEM_FAILED) {
            perror("Failed to create basket semaphore");
            return -1;
        }
    }
    
    /* Initialize maze locks */
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            snprintf(sem_name, sizeof(sem_name), "/apes_maze_%d_%d_%d", getpid(), i, j);
            sem_unlink(sem_name);
            maze_lock_ptrs[i][j] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
            if (maze_lock_ptrs[i][j] == SEM_FAILED) {
                perror("Failed to create maze semaphore");
                return -1;
            }
        }
    }
    
    return 0;
}

void cleanup_simulation_semaphores(int num_families, int rows, int cols) {
    char sem_name[64];
    int i, j;
    
    /* Close and unlink global lock */
    if (global_lock_ptr != NULL) {
        sem_close(global_lock_ptr);
        snprintf(sem_name, sizeof(sem_name), "/apes_global_%d", getpid());
        sem_unlink(sem_name);
    }
    
    /* Close and unlink basket locks */
    for (i = 0; i < num_families; i++) {
        if (basket_lock_ptrs[i] != NULL) {
            sem_close(basket_lock_ptrs[i]);
            snprintf(sem_name, sizeof(sem_name), "/apes_basket_%d_%d", getpid(), i);
            sem_unlink(sem_name);
        }
    }
    
    /* Close and unlink maze locks */
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            if (maze_lock_ptrs[i][j] != NULL) {
                sem_close(maze_lock_ptrs[i][j]);
                snprintf(sem_name, sizeof(sem_name), "/apes_maze_%d_%d_%d", getpid(), i, j);
                sem_unlink(sem_name);
            }
        }
    }
}

int sem_wait_wrapper(sem_t* sem, int row, int col, int is_maze) {
    (void)sem;  /* Unused on macOS */
    if (is_maze) {
        return sem_wait(maze_lock_ptrs[row][col]);
    }
    return -1;
}

int sem_post_wrapper(sem_t* sem, int row, int col, int is_maze) {
    (void)sem;  /* Unused on macOS */
    if (is_maze) {
        return sem_post(maze_lock_ptrs[row][col]);
    }
    return -1;
}

int sem_wait_basket(sem_t* sem, int family_id) {
    (void)sem;  /* Unused on macOS */
    return sem_wait(basket_lock_ptrs[family_id]);
}

int sem_post_basket(sem_t* sem, int family_id) {
    (void)sem;  /* Unused on macOS */
    return sem_post(basket_lock_ptrs[family_id]);
}

int sem_wait_global(sem_t* sem) {
    (void)sem;  /* Unused on macOS */
    return sem_wait(global_lock_ptr);
}

int sem_post_global(sem_t* sem) {
    (void)sem;  /* Unused on macOS */
    return sem_post(global_lock_ptr);
}

#else
/* Linux: Use unnamed semaphores directly */

int init_simulation_semaphores(void* shared_data_ptr, int num_families, int rows, int cols) {
    SharedData* shared = (SharedData*)shared_data_ptr;
    int i, j;
    
    /* Initialize global lock */
    if (sem_init(&shared->global_lock, 1, 1) != 0) {
        perror("Failed to init global semaphore");
        return -1;
    }
    
    /* Initialize basket locks */
    for (i = 0; i < num_families; i++) {
        if (sem_init(&shared->basket_locks[i], 1, 1) != 0) {
            perror("Failed to init basket semaphore");
            return -1;
        }
    }
    
    /* Initialize maze locks */
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            sem_init(&shared->maze_locks[i][j], 1, 1);
        }
    }
    
    return 0;
}

void cleanup_simulation_semaphores(int num_families, int rows, int cols) {
    /* Linux cleanup handled elsewhere */
    (void)num_families;
    (void)rows;
    (void)cols;
}

int sem_wait_wrapper(sem_t* sem, int row, int col, int is_maze) {
    (void)row; (void)col; (void)is_maze;
    return sem_wait(sem);
}

int sem_post_wrapper(sem_t* sem, int row, int col, int is_maze) {
    (void)row; (void)col; (void)is_maze;
    return sem_post(sem);
}

int sem_wait_basket(sem_t* sem, int family_id) {
    (void)family_id;
    return sem_wait(sem);
}

int sem_post_basket(sem_t* sem, int family_id) {
    (void)family_id;
    return sem_post(sem);
}

int sem_wait_global(sem_t* sem) {
    return sem_wait(sem);
}

int sem_post_global(sem_t* sem) {
    return sem_post(sem);
}

#endif
