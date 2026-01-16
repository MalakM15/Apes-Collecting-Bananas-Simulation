/*
 * shared_data.h
 * Shared memory structures for inter-process communication
 * Apes Collecting Bananas Simulation
 */

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <semaphore.h>
#include <time.h>
#include <pthread.h>

/* Maximum limits */
#define MAX_ROWS 50
#define MAX_COLS 50
#define MAX_FAMILIES 10
#define MAX_BABIES 5

/* Event log settings */
#define MAX_EVENTS 10
#define MAX_EVENT_LEN 120

/* Termination reasons */
#define TERM_RUNNING 0
#define TERM_WITHDRAWN_THRESHOLD 1
#define TERM_BASKET_THRESHOLD 2
#define TERM_BABY_ATE_THRESHOLD 3
#define TERM_TIMEOUT 4

/* Direction constants for movement */
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

/*
 * Single cell in the maze
 */
typedef struct {
    int bananas;                        // Number of bananas in this cell
    int is_obstacle;                    // 1 if obstacle, 0 if passable
    int females_in_cell[MAX_FAMILIES];  // Track which females are in this cell (1=present)
} MazeCell;

/*
 * Public family status (visible to all processes via shared memory)
 */
typedef struct {
    int basket_bananas;                 // Bananas in this family's basket
    int is_active;                      // 1 = participating, 0 = withdrawn
    int male_fighting;                  // 1 = male currently in fight
    int female_fighting;                // 1 = female currently in fight
    int female_opponent;                // ID of opponent (if fighting), -1 if not fighting
    int male_energy;                    // Current male energy (for display)
    int female_energy;                  // Current female energy (for display)
    int female_x, female_y;             // Female's current position in maze
    int female_in_maze;                 // 1 = female is in maze, 0 = at basket
    int female_resting;                 // 1 = female is resting (vulnerable if carrying bananas!)
    int female_collected;               // Bananas female is currently carrying
    int baby_bananas_eaten[MAX_BABIES]; // Bananas eaten by each baby
    int total_collected;                // Total bananas collected by female
    
    // Detailed banana flow tracking
    int bananas_from_maze;              // Collected directly from maze by female
    int bananas_from_male_fights;       // Gained through male fights
    int bananas_from_female_fights;     // Gained through female fights
    int bananas_lost_male_fights;       // Lost through male fights
    int bananas_lost_female_fights;     // Lost through female fights
} FamilyStatus;

/*
 * Recent event entry for live display
 */
typedef struct {
    char message[MAX_EVENT_LEN];
    double timestamp;
} EventEntry;

/*
 * Main shared memory structure
 * This is shared between the main process and all family processes
 * Note: Named struct allows forward declaration in other headers
 */
typedef struct SharedData {
    // Maze data
    MazeCell maze[MAX_ROWS][MAX_COLS];
    int maze_rows;
    int maze_cols;
    int total_bananas_in_maze;          // Track remaining bananas
    
    // Family status array
    FamilyStatus families[MAX_FAMILIES];
    int num_families;
    
    // Global simulation state
    int withdrawn_count;
    int simulation_running;             // 1 = running, 0 = stopped
    int termination_reason;             // TERM_* constant
    int winning_family;                 // Family ID that caused termination (-1 if none)
    time_t start_time;
    
    // Recent events circular buffer for live display
    EventEntry recent_events[MAX_EVENTS];
    int event_head;                      // Next write position
    sem_t event_lock;                    // Lock for event buffer
    
    // Synchronization primitives
    // Note: These semaphores are initialized with pshared=1 for inter-process use
    sem_t maze_locks[MAX_ROWS][MAX_COLS];   // Per-cell locks
    sem_t basket_locks[MAX_FAMILIES];        // Per-basket locks
    sem_t global_lock;                       // For global state updates
    
} SharedData;

/*
 * Shared memory key (for shmget)
 */
#define SHM_KEY 0x1234

#endif /* SHARED_DATA_H */

