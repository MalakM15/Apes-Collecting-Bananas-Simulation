/*
 * family.h
 * Family process with male, female, and baby threads
 * Apes Collecting Bananas Simulation
 */

#ifndef FAMILY_H
#define FAMILY_H

#include <pthread.h>
#include "shared_data.h"
#include "config.h"

/*
 * Local family data (private to each family process)
 * This is NOT in shared memory - each process has its own copy
 */
typedef struct {
    int family_id;
    
    // Basket (synced to shared memory)
    int basket_bananas;
    
    // Male state
    int male_energy;
    int male_fighting;              // Flag to signal babies
    
    // Female state
    int female_energy;
    int female_x, female_y;         // Current position
    int female_collected;           // Bananas being carried
    int female_in_maze;             // 1 = in maze, 0 = at basket
    int female_resting;             // 1 = resting
    
    // Baby states
    int num_babies;
    int baby_eaten[MAX_BABIES];     // Per-baby eaten count
    
    // Control flags
    int should_withdraw;            // Set by male when energy low
    
    // Thread synchronization (within this process)
    pthread_mutex_t family_lock;
    pthread_cond_t fight_started;   // Signal babies when male fights
    pthread_cond_t fight_ended;     // Signal when fight is over
    
    // References to shared data
    SharedData* shared;
    const SimConfig* config;
    
} FamilyLocal;

/*
 * Baby thread argument structure
 */
typedef struct {
    int baby_id;
    FamilyLocal* family;
} BabyArg;

/*
 * Main entry point for family process
 * Called after fork() in child process
 */
void run_family_process(int family_id, SharedData* shared, const SimConfig* config);

/*
 * Thread function: Female ape
 * - Navigates maze collecting bananas
 * - Fights other females on collision
 * - Deposits bananas to basket
 * - Rests when energy is low
 */
void* female_thread(void* arg);

/*
 * Thread function: Male ape
 * - Monitors neighbors for fights
 * - Initiates fights based on probability
 * - Signals babies during fights
 * - Triggers family withdrawal when energy low
 */
void* male_thread(void* arg);

/*
 * Thread function: Baby ape
 * - Waits for male fight opportunity
 * - Steals from other family baskets
 * - Decides to eat or give to dad
 */
void* baby_thread(void* arg);

/*
 * Get neighboring family IDs (linear basket arrangement)
 * Sets left and right to neighbor IDs, or -1 if no neighbor
 */
void get_neighbors(int family_id, int num_families, int* left, int* right);

/*
 * Calculate fight probability between two males
 * Based on combined banana count in both baskets
 */
float calculate_fight_probability(int my_bananas, int their_bananas, const SimConfig* config);

/*
 * Execute female fight
 * Called when two females are in the same cell
 * Random winner takes loser's collected bananas
 */
void female_fight(FamilyLocal* local, int other_family_id);

/*
 * Execute male fight
 * Called when male decides to fight neighbor
 * Random winner takes loser's basket
 * Signals babies during fight
 */
void male_fight(FamilyLocal* local, int opponent_id);

/*
 * Thread-safe basket operations
 * These handle synchronization between local and shared memory
 */
int add_to_basket(FamilyLocal* local, int amount);
int get_basket_count(FamilyLocal* local);

/*
 * Check if simulation should continue
 */
int should_continue(FamilyLocal* local);

/*
 * Initialize family local data
 */
void init_family_local(FamilyLocal* local, int family_id, 
                       SharedData* shared, const SimConfig* config);

/*
 * Cleanup family resources
 */
void cleanup_family_local(FamilyLocal* local);

#endif /* FAMILY_H */

