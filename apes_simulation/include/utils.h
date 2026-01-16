/*
 * utils.h
 * Utility functions for the simulation
 * Apes Collecting Bananas Simulation
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <time.h>

/* ==================== Random Functions ==================== */

/*
 * Initialize random seed
 */
void init_random(void);

/*
 * Generate random integer in range [min, max] (inclusive)
 */
int random_int(int min, int max);

/*
 * Generate random float in range [min, max]
 */
float random_float(float min, float max);

/*
 * Return 1 with given probability (0.0 to 1.0), 0 otherwise
 */
int random_chance(float probability);

/* ==================== Time Functions ==================== */

/*
 * Get elapsed seconds since start_time
 */
double get_elapsed_seconds(time_t start_time);

/*
 * Sleep for specified milliseconds
 */
void sleep_ms(int milliseconds);

/*
 * Get current time as string (for logging)
 */
void get_time_string(char* buffer, size_t size);

/* ==================== Logging Functions ==================== */

/*
 * Log an event with timestamp
 */
void log_event(const char* format, ...);

/*
 * Log a family-specific event
 */
void log_family(int family_id, const char* format, ...);

/*
 * Log a female action
 */
void log_female(int family_id, const char* format, ...);

/*
 * Log a male action
 */
void log_male(int family_id, const char* format, ...);

/*
 * Log a baby action
 */
void log_baby(int family_id, int baby_id, const char* format, ...);

/* ==================== Shared Memory Helpers ==================== */

/*
 * Create shared memory segment
 * Returns shared memory ID on success, -1 on failure
 */
int create_shared_memory(size_t size, int key);

/*
 * Attach to existing shared memory segment
 * Returns pointer to shared memory, NULL on failure
 */
void* attach_shared_memory(int shm_id);

/*
 * Detach from shared memory
 */
void detach_shared_memory(void* ptr);

/*
 * Destroy shared memory segment
 */
void destroy_shared_memory(int shm_id);

/* ==================== Console Helpers ==================== */

/*
 * Clear console screen
 */
void clear_screen(void);

/*
 * Move cursor to position
 */
void move_cursor(int row, int col);

/*
 * Set console text color
 */
void set_color(int color);

/*
 * Reset console color
 */
void reset_color(void);

/* Color constants */
#define COLOR_RESET   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

/* ==================== String Helpers ==================== */

/*
 * Trim whitespace from string (modifies in place)
 */
char* trim(char* str);

/*
 * Safe string copy
 */
void safe_strcpy(char* dest, const char* src, size_t size);

/*
 * Log event to file with timestamp
 */
void log_event_to_file(const char* format, ...);

/* Forward declaration for SharedData (defined in shared_data.h) */
struct SharedData;

/*
 * Add event to shared memory buffer for live display
 */
void add_shared_event(struct SharedData* shared, const char* format, ...);

#endif /* UTILS_H */

