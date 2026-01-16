/*
 * config.h
 * Configuration structure and parser declarations
 * Apes Collecting Bananas Simulation
 */

#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    // Maze settings
    int maze_rows;
    int maze_cols;
    float obstacle_probability;
    int max_bananas_per_cell;
    int total_bananas;
    
    // Family settings
    int num_families;
    int babies_per_family;
    
    // Female settings
    int female_initial_energy;
    int female_rest_threshold;
    int female_rest_recovery;
    int female_collection_goal;
    int female_move_energy_cost;
    int female_fight_energy_cost;
    
    // Male settings
    int male_initial_energy;
    int male_withdraw_threshold;
    int male_fight_energy_cost;
    
    // Fight settings
    float fight_probability_base;
    float fight_probability_per_banana;
    float fight_max_probability;
    
    // Termination thresholds
    int max_withdrawn_families;
    int winning_basket_threshold;
    int baby_eaten_threshold;
    int max_simulation_time_seconds;
    
} SimConfig;

/*
 * Load configuration from file
 * Returns pointer to SimConfig on success, NULL on failure
 */
SimConfig* load_config(const char* filename);

/*
 * Print configuration values (for debugging)
 */
void print_config(const SimConfig* config);

/*
 * Free configuration memory
 */
void free_config(SimConfig* config);

/*
 * Set default configuration values
 */
void set_default_config(SimConfig* config);

#endif /* CONFIG_H */

