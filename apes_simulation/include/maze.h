/*
 * maze.h
 * Maze generation and operations
 * Apes Collecting Bananas Simulation
 */

#ifndef MAZE_H
#define MAZE_H

#include "shared_data.h"
#include "config.h"

/*
 * Initialize the maze with random obstacles and bananas
 * Also initializes the per-cell semaphores
 */
void init_maze(SharedData* shared, const SimConfig* config);

/*
 * Print the maze to console (for debugging)
 * Shows obstacles, bananas, and female positions
 */
void print_maze(const SharedData* shared);

/*
 * Print maze with colored output
 */
void print_maze_colored(const SharedData* shared);

/*
 * Print compact maze with colors (2 chars per cell)
 */
void print_maze_compact(const SharedData* shared);

/*
 * Get number of bananas at a specific cell
 * Thread-safe: uses cell lock
 */
int get_bananas_at(SharedData* shared, int x, int y);

/*
 * Take bananas from a cell
 * Returns the actual number of bananas taken (may be less than requested)
 * Thread-safe: uses cell lock
 */
int take_bananas(SharedData* shared, int x, int y, int count);

/*
 * Check if a cell is an obstacle
 */
int is_obstacle(const SharedData* shared, int x, int y);

/*
 * Check if coordinates are within maze bounds
 */
int is_valid_cell(const SharedData* shared, int x, int y);

/*
 * Check if a cell is passable (valid, not obstacle)
 */
int is_passable(const SharedData* shared, int x, int y);

/*
 * Get a random valid starting position for a female
 * Returns 1 on success, 0 on failure
 */
int get_random_start_position(const SharedData* shared, int* x, int* y);

/*
 * Get next move direction towards exit (simple pathfinding)
 * Exit is defined as row 0 (top of maze)
 * Returns direction constant (DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT) or -1 if stuck
 */
int get_direction_to_exit(const SharedData* shared, int x, int y);

/*
 * Get next move direction to explore maze (find bananas)
 * Prefers cells with bananas, avoids obstacles
 */
int get_direction_to_explore(const SharedData* shared, int x, int y);

/*
 * Move from current position in given direction
 * Updates x, y with new position
 * Returns 1 if move successful, 0 if blocked
 */
int move_in_direction(const SharedData* shared, int* x, int* y, int direction);

/*
 * Mark a female as present/absent in a cell
 */
void set_female_in_cell(SharedData* shared, int x, int y, int family_id, int present);

/*
 * Check if another female is in the same cell
 * Returns the family_id of the other female, or -1 if none
 */
int check_female_collision(const SharedData* shared, int x, int y, int my_family_id);

/*
 * Cleanup maze semaphores
 */
void cleanup_maze(SharedData* shared);

#endif /* MAZE_H */

