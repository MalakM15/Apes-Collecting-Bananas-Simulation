#include "local.h"

#define MAX_LINE_LENGTH 256

static char* trim_whitespace(char* str) {
    char* end;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    
    return str;
}

void set_default_config(SimConfig* config) {
    /* Maze settings */
    config->maze_rows = 15;
    config->maze_cols = 20;
    config->obstacle_probability = 0.15f;
    config->max_bananas_per_cell = 5;
    config->total_bananas = 100;
    
    /* Family settings */
    config->num_families = 4;
    config->babies_per_family = 2;
    
    /* Female settings */
    config->female_initial_energy = 100;
    config->female_rest_threshold = 20;
    config->female_rest_recovery = 30;
    config->female_collection_goal = 8;
    config->female_move_energy_cost = 1;
    config->female_fight_energy_cost = 5;
    
    /* Male settings */
    config->male_initial_energy = 100;
    config->male_withdraw_threshold = 15;
    config->male_fight_energy_cost = 10;
    
    /* Fight settings */
    config->fight_probability_base = 0.05f;
    config->fight_probability_per_banana = 0.01f;
    config->fight_max_probability = 0.80f;
    
    /* Termination thresholds */
    config->max_withdrawn_families = 2;
    config->winning_basket_threshold = 50;
    config->baby_eaten_threshold = 15;
    config->max_simulation_time_seconds = 120;
}

static void parse_config_line(SimConfig* config, const char* key, const char* value) {
    if (strcmp(key, "maze_rows") == 0) {
        config->maze_rows = atoi(value);
    } else if (strcmp(key, "maze_cols") == 0) {
        config->maze_cols = atoi(value);
    } else if (strcmp(key, "obstacle_probability") == 0) {
        config->obstacle_probability = atof(value);
    } else if (strcmp(key, "max_bananas_per_cell") == 0) {
        config->max_bananas_per_cell = atoi(value);
    } else if (strcmp(key, "total_bananas") == 0) {
        config->total_bananas = atoi(value);
    }
    else if (strcmp(key, "num_families") == 0) {
        config->num_families = atoi(value);
    } else if (strcmp(key, "babies_per_family") == 0) {
        config->babies_per_family = atoi(value);
    }

    else if (strcmp(key, "female_initial_energy") == 0) {
        config->female_initial_energy = atoi(value);
    } else if (strcmp(key, "female_rest_threshold") == 0) {
        config->female_rest_threshold = atoi(value);
    } else if (strcmp(key, "female_rest_recovery") == 0) {
        config->female_rest_recovery = atoi(value);
    } else if (strcmp(key, "female_collection_goal") == 0) {
        config->female_collection_goal = atoi(value);
    } else if (strcmp(key, "female_move_energy_cost") == 0) {
        config->female_move_energy_cost = atoi(value);
    } else if (strcmp(key, "female_fight_energy_cost") == 0) {
        config->female_fight_energy_cost = atoi(value);
    }

    else if (strcmp(key, "male_initial_energy") == 0) {
        config->male_initial_energy = atoi(value);
    } else if (strcmp(key, "male_withdraw_threshold") == 0) {
        config->male_withdraw_threshold = atoi(value);
    } else if (strcmp(key, "male_fight_energy_cost") == 0) {
        config->male_fight_energy_cost = atoi(value);
    }

    else if (strcmp(key, "fight_probability_base") == 0) {
        config->fight_probability_base = atof(value);
    } else if (strcmp(key, "fight_probability_per_banana") == 0) {
        config->fight_probability_per_banana = atof(value);
    } else if (strcmp(key, "fight_max_probability") == 0) {
        config->fight_max_probability = atof(value);
    }

    else if (strcmp(key, "max_withdrawn_families") == 0) {
        config->max_withdrawn_families = atoi(value);
    } else if (strcmp(key, "winning_basket_threshold") == 0) {
        config->winning_basket_threshold = atoi(value);
    } else if (strcmp(key, "baby_eaten_threshold") == 0) {
        config->baby_eaten_threshold = atoi(value);
    } else if (strcmp(key, "max_simulation_time_seconds") == 0) {
        config->max_simulation_time_seconds = atoi(value);
    }
    else {
        fprintf(stderr, "Warning: Unknown config key '%s'\n", key);
    }
}


SimConfig* load_config(const char* filename) {
    FILE* file;
    char line[MAX_LINE_LENGTH];
    SimConfig* config;
    

    config = (SimConfig*)malloc(sizeof(SimConfig));
    if (config == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for config\n");
        return NULL;
    }
    

    set_default_config(config);
    
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Warning: Could not open config file '%s', using defaults\n", filename);
        return config;
    }
    

    while (fgets(line, sizeof(line), file) != NULL) {
        char* trimmed = trim_whitespace(line);
        char* key;
        char* value;
        char* equals;
        

        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }
        

        equals = strchr(trimmed, '=');
        if (equals == NULL) {
            continue;
        }
        
        
        *equals = '\0';
        key = trim_whitespace(trimmed);
        value = trim_whitespace(equals + 1);
        
        
        parse_config_line(config, key, value);
    }
    
    fclose(file);
    
    
    if (config->maze_rows > MAX_ROWS) {
        fprintf(stderr, "Warning: maze_rows exceeds MAX_ROWS (%d), capping\n", MAX_ROWS);
        config->maze_rows = MAX_ROWS;
    }
    if (config->maze_cols > MAX_COLS) {
        fprintf(stderr, "Warning: maze_cols exceeds MAX_COLS (%d), capping\n", MAX_COLS);
        config->maze_cols = MAX_COLS;
    }
    if (config->num_families > MAX_FAMILIES) {
        fprintf(stderr, "Warning: num_families exceeds MAX_FAMILIES (%d), capping\n", MAX_FAMILIES);
        config->num_families = MAX_FAMILIES;
    }
    if (config->babies_per_family > MAX_BABIES) {
        fprintf(stderr, "Warning: babies_per_family exceeds MAX_BABIES (%d), capping\n", MAX_BABIES);
        config->babies_per_family = MAX_BABIES;
    }
    
    return config;
}


void print_config(const SimConfig* config) {
    printf("\n========== SIMULATION CONFIGURATION ==========\n");
    printf("\n--- Maze Settings ---\n");
    printf("  maze_rows:              %d\n", config->maze_rows);
    printf("  maze_cols:              %d\n", config->maze_cols);
    printf("  obstacle_probability:   %.2f\n", config->obstacle_probability);
    printf("  max_bananas_per_cell:   %d\n", config->max_bananas_per_cell);
    printf("  total_bananas:          %d\n", config->total_bananas);
    
    printf("\n--- Family Settings ---\n");
    printf("  num_families:           %d\n", config->num_families);
    printf("  babies_per_family:      %d\n", config->babies_per_family);
    
    printf("\n--- Female Settings ---\n");
    printf("  female_initial_energy:  %d\n", config->female_initial_energy);
    printf("  female_rest_threshold:  %d\n", config->female_rest_threshold);
    printf("  female_rest_recovery:   %d\n", config->female_rest_recovery);
    printf("  female_collection_goal: %d\n", config->female_collection_goal);
    printf("  female_move_energy_cost:%d\n", config->female_move_energy_cost);
    printf("  female_fight_energy_cost:%d\n", config->female_fight_energy_cost);
    
    printf("\n--- Male Settings ---\n");
    printf("  male_initial_energy:    %d\n", config->male_initial_energy);
    printf("  male_withdraw_threshold:%d\n", config->male_withdraw_threshold);
    printf("  male_fight_energy_cost: %d\n", config->male_fight_energy_cost);
    
    printf("\n--- Fight Settings ---\n");
    printf("  fight_probability_base: %.2f\n", config->fight_probability_base);
    printf("  fight_prob_per_banana:  %.3f\n", config->fight_probability_per_banana);
    printf("  fight_max_probability:  %.2f\n", config->fight_max_probability);
    
    printf("\n--- Termination Thresholds ---\n");
    printf("  max_withdrawn_families: %d\n", config->max_withdrawn_families);
    printf("  winning_basket_threshold:%d\n", config->winning_basket_threshold);
    printf("  baby_eaten_threshold:   %d\n", config->baby_eaten_threshold);
    printf("  max_simulation_time:    %d seconds\n", config->max_simulation_time_seconds);
    printf("===============================================\n\n");
}


void free_config(SimConfig* config) {
    if (config != NULL) {
        free(config);
    }
}

