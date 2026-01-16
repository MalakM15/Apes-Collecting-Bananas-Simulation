#include "local.h"

void init_maze(SharedData* shared, const SimConfig* config) {
    int i, j, k;
    int bananas_placed = 0;
    int target_bananas = config->total_bananas;
    
    shared->maze_rows = config->maze_rows;
    shared->maze_cols = config->maze_cols;
    
    /* Initialize all cells */
    for (i = 0; i < config->maze_rows; i++) {
        for (j = 0; j < config->maze_cols; j++) {
            MazeCell* cell = &shared->maze[i][j];
            
            /* Clear female tracking */
            for (k = 0; k < MAX_FAMILIES; k++) {
                cell->females_in_cell[k] = 0;
            }
            
            /* Note: Semaphores are initialized by sem_wrapper */
            
            /* First row (row 0) is the exit - no obstacles */
            if (i == 0) {
                cell->is_obstacle = 0;
                cell->bananas = 0;
                continue;
            }
            
            /* Random obstacles (not in first row) */
            if (random_chance(config->obstacle_probability)) {
                cell->is_obstacle = 1;
                cell->bananas = 0;
            } else {
                cell->is_obstacle = 0;
                cell->bananas = 0;
            }
        }
    }
    
    /* Ensure there's a clear path - make sure not all cells in any row are obstacles */
    for (i = 1; i < config->maze_rows; i++) {
        int has_passage = 0;
        for (j = 0; j < config->maze_cols; j++) {
            if (!shared->maze[i][j].is_obstacle) {
                has_passage = 1;
                break;
            }
        }
        if (!has_passage) {
            /* Clear a random cell in this row */
            int clear_col = random_int(0, config->maze_cols - 1);
            shared->maze[i][clear_col].is_obstacle = 0;
        }
    }
    
    /* Distribute bananas randomly (not on obstacles, not on first row) */
    while (bananas_placed < target_bananas) {
        int row = random_int(1, config->maze_rows - 1);
        int col = random_int(0, config->maze_cols - 1);
        
        MazeCell* cell = &shared->maze[row][col];
        
        if (!cell->is_obstacle && cell->bananas < config->max_bananas_per_cell) {
            cell->bananas++;
            bananas_placed++;
        }
    }
    
    shared->total_bananas_in_maze = target_bananas;
    
    log_event("Maze initialized: %dx%d with %d bananas", 
              config->maze_rows, config->maze_cols, target_bananas);
}

void print_maze(const SharedData* shared) {
    int i, j;
    
    printf("\n");
    
    /* Top border */
    printf("    ");
    for (j = 0; j < shared->maze_cols; j++) {
        printf("---");
    }
    printf("\n");
    
    for (i = 0; i < shared->maze_rows; i++) {
        printf("%2d |", i);
        for (j = 0; j < shared->maze_cols; j++) {
            const MazeCell* cell = &shared->maze[i][j];
            
            if (cell->is_obstacle) {
                printf("███");
            } else {
                /* Check if any female is here */
                int female_here = -1;
                int k;
                for (k = 0; k < shared->num_families; k++) {
                    if (cell->females_in_cell[k]) {
                        female_here = k;
                        break;
                    }
                }
                
                if (female_here >= 0) {
                    printf(" F%d", female_here);
                } else if (cell->bananas > 0) {
                    printf(" %d ", cell->bananas);
                } else {
                    printf(" . ");
                }
            }
        }
        printf("|\n");
    }
    
    /* Bottom border */
    printf("    ");
    for (j = 0; j < shared->maze_cols; j++) {
        printf("---");
    }
    printf("\n");
    
    /* Column numbers */
    printf("    ");
    for (j = 0; j < shared->maze_cols; j++) {
        printf("%2d ", j);
    }
    printf("\n\n");
}

void print_maze_colored(const SharedData* shared) {
    int i, j;
    
    printf("\n");
    
    /* Top border */
    printf("    ");
    for (j = 0; j < shared->maze_cols; j++) {
        printf("───");
    }
    printf("─\n");
    
    for (i = 0; i < shared->maze_rows; i++) {
        printf("%2d │", i);
        for (j = 0; j < shared->maze_cols; j++) {
            const MazeCell* cell = &shared->maze[i][j];
            
            if (cell->is_obstacle) {
                set_color(COLOR_WHITE);
                printf("███");
                reset_color();
            } else {
                /* Check if any female is here */
                int female_here = -1;
                int k;
                for (k = 0; k < shared->num_families; k++) {
                    if (cell->females_in_cell[k]) {
                        female_here = k;
                        break;
                    }
                }
                
                if (female_here >= 0) {
                    set_color(COLOR_MAGENTA);
                    printf(" F%d", female_here);
                    reset_color();
                } else if (cell->bananas > 0) {
                    set_color(COLOR_YELLOW);
                    printf(" %d ", cell->bananas);
                    reset_color();
                } else {
                    printf(" · ");
                }
            }
        }
        printf("│\n");
    }
    
    /* Bottom border */
    printf("    ");
    for (j = 0; j < shared->maze_cols; j++) {
        printf("───");
    }
    printf("─\n");
}

void print_maze_compact(const SharedData* shared) {
    int i, j;
    
    /* Family colors for females */
    const char* family_colors[] = {
        "\033[91m",  /* Red */
        "\033[92m",  /* Green */
        "\033[94m",  /* Blue */
        "\033[95m",  /* Magenta */
        "\033[96m",  /* Cyan */
        "\033[93m",  /* Yellow */
    };
    int num_colors = 6;
    
    /* Top border */
    printf("   ┌");
    for (j = 0; j < shared->maze_cols; j++) {
        printf("──");
    }
    printf("┐\n");
    
    for (i = 0; i < shared->maze_rows; i++) {
        printf("%2d │", i);
        for (j = 0; j < shared->maze_cols; j++) {
            const MazeCell* cell = &shared->maze[i][j];
            
            if (cell->is_obstacle) {
                printf("\033[47m  \033[0m");  /* White background block */
            } else {
                /* Check if any female is here */
                int female_here = -1;
                int k;
                for (k = 0; k < shared->num_families; k++) {
                    if (cell->females_in_cell[k]) {
                        female_here = k;
                        break;
                    }
                }
                
                if (female_here >= 0) {
                    /* Female ape - show with family color */
                    printf("%s", family_colors[female_here % num_colors]);
                    printf("🐒");
                    printf("\033[0m");
                } else if (cell->bananas > 0) {
                    /* Bananas - yellow */
                    printf("\033[93m");
                    if (cell->bananas >= 5) {
                        printf("🍌");
                    } else {
                        printf("%d ", cell->bananas);
                    }
                    printf("\033[0m");
                } else {
                    /* Empty cell */
                    printf("· ");
                }
            }
        }
        printf("│\n");
    }
    
    /* Bottom border */
    printf("   └");
    for (j = 0; j < shared->maze_cols; j++) {
        printf("──");
    }
    printf("┘\n");
}

int get_bananas_at(SharedData* shared, int x, int y) {
    int bananas;
    
    if (!is_valid_cell(shared, x, y)) return 0;
    
    sem_wait(&shared->maze_locks[x][y]);
    bananas = shared->maze[x][y].bananas;
    sem_post(&shared->maze_locks[x][y]);
    
    return bananas;
}

int take_bananas(SharedData* shared, int x, int y, int count) {
    int taken = 0;
    
    if (!is_valid_cell(shared, x, y)) return 0;
    
    sem_wait(&shared->maze_locks[x][y]);
    
    MazeCell* cell = &shared->maze[x][y];
    
    if (cell->bananas >= count) {
        taken = count;
    } else {
        taken = cell->bananas;
    }
    
    cell->bananas -= taken;
    
    /* Update global count */
    sem_wait(&shared->global_lock);
    shared->total_bananas_in_maze -= taken;
    sem_post(&shared->global_lock);
    
    sem_post(&shared->maze_locks[x][y]);
    
    return taken;
}

int is_obstacle(const SharedData* shared, int x, int y) {
    if (!is_valid_cell(shared, x, y)) return 1;
    return shared->maze[x][y].is_obstacle;
}

int is_valid_cell(const SharedData* shared, int x, int y) {
    return (x >= 0 && x < shared->maze_rows && 
            y >= 0 && y < shared->maze_cols);
}


int is_passable(const SharedData* shared, int x, int y) {
    return is_valid_cell(shared, x, y) && !is_obstacle(shared, x, y);
}

int get_random_start_position(const SharedData* shared, int* x, int* y) {
    int attempts = 0;
    int max_attempts = 100;
    
    /* Start from the bottom border row (entry point) */
    int bottom_row = shared->maze_rows - 1;
    
    while (attempts < max_attempts) {
        int col = random_int(0, shared->maze_cols - 1);
        
        if (is_passable(shared, bottom_row, col)) {
            *x = bottom_row;
            *y = col;
            return 1;
        }
        attempts++;
    }
    
    return 0;  /* Failed to find valid position */
}

int get_direction_to_exit(const SharedData* shared, int x, int y) {
    /* Priority: UP (towards exit(row0)), then sideways, then down */
    
    /* Try UP first */
    if (is_passable(shared, x - 1, y)) {
        return DIR_UP;
    }
    
    /* Try sideways */
    int try_left = random_chance(0.5);
    
    if (try_left) {
        if (is_passable(shared, x, y - 1)) return DIR_LEFT;
        if (is_passable(shared, x, y + 1)) return DIR_RIGHT;
    } else {
        if (is_passable(shared, x, y + 1)) return DIR_RIGHT;
        if (is_passable(shared, x, y - 1)) return DIR_LEFT;
    }
    
    /* Try down as last resort */
    if (is_passable(shared, x + 1, y)) {
        return DIR_DOWN;
    }
    
    return -1;  /* Stuck */
}

/*
 * Get direction to explore maze (find bananas)
 */
int get_direction_to_explore(const SharedData* shared, int x, int y) {
    int directions[4] = {DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT};
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};
    int best_dir = -1;
    int best_bananas = -1;
    int i;
    
    /* Shuffle directions for randomness */
    for (i = 3; i > 0; i--) {
        int j = random_int(0, i);
        int temp = directions[i];
        directions[i] = directions[j];
        directions[j] = temp;
    }
    
    /* Check each direction */
    for (i = 0; i < 4; i++) {
        int dir = directions[i];
        int nx = x + dx[dir];
        int ny = y + dy[dir];
        
        if (is_passable(shared, nx, ny)) {
            int bananas = shared->maze[nx][ny].bananas;
            
            /* Prefer cells with bananas */
            if (bananas > best_bananas) {
                best_bananas = bananas;
                best_dir = dir;
            } else if (best_dir < 0) {
                /* Accept any passable cell if nothing better */
                best_dir = dir;
            }
        }
    }
    
    return best_dir;
}

/*
 * Move from current position in given direction
 */
int move_in_direction(const SharedData* shared, int* x, int* y, int direction) {
    int nx = *x, ny = *y;
    
    switch (direction) {
        case DIR_UP:    nx = *x - 1; break;
        case DIR_DOWN:  nx = *x + 1; break;
        case DIR_LEFT:  ny = *y - 1; break;
        case DIR_RIGHT: ny = *y + 1; break;
        default: return 0;
    }
    
    if (is_passable(shared, nx, ny)) {
        *x = nx;
        *y = ny;
        return 1;
    }
    
    return 0;
}


void set_female_in_cell(SharedData* shared, int x, int y, int family_id, int present) {
    if (!is_valid_cell(shared, x, y)) return;
    if (family_id < 0 || family_id >= MAX_FAMILIES) return;
    
    sem_wait(&shared->maze_locks[x][y]);
    shared->maze[x][y].females_in_cell[family_id] = present;
    sem_post(&shared->maze_locks[x][y]);
}

int check_female_collision(const SharedData* shared, int x, int y, int my_family_id) {
    int other_female = -1;
    int k;
    
    if (!is_valid_cell(shared, x, y)) return -1;
    
    /* Note: Caller should hold the cell lock for thread safety */
    const MazeCell* cell = &shared->maze[x][y];
    
    for (k = 0; k < shared->num_families; k++) {
        if (k != my_family_id && cell->females_in_cell[k]) {
            other_female = k;
            break;
        }
    }
    
    return other_female;
}

void cleanup_maze(SharedData* shared) {
    /* Note: Semap hore cleanup handled by sem_wrapper */
    (void)shared;  /* Unused parameter */
    
    log_event("Maze cleanup complete");
}

