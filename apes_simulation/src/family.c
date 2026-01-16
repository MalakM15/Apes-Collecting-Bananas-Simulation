#include "local.h"

/*
 * Get neighboring family IDs (linear basket arrangement)
 */
void get_neighbors(int family_id, int num_families, int* left, int* right) {
    *left = (family_id > 0) ? family_id - 1 : -1;
    *right = (family_id < num_families - 1) ? family_id + 1 : -1;
}

float calculate_fight_probability(int my_bananas, int their_bananas, const SimConfig* config) {
    int total = my_bananas + their_bananas;
    float prob = config->fight_probability_base + 
                 (total * config->fight_probability_per_banana);
    
    if (prob > config->fight_max_probability) {
        prob = config->fight_max_probability;
    }
    
    return prob;
}

void sync_basket_to_shared_unlocked(FamilyLocal* local) {
    local->shared->families[local->family_id].basket_bananas = local->basket_bananas;
}

void sync_basket_from_shared_unlocked(FamilyLocal* local) {
    local->basket_bananas = local->shared->families[local->family_id].basket_bananas;
}

/*
 * Add bananas to basket
 * Returns new basket total
 */
int add_to_basket(FamilyLocal* local, int amount) {
    SharedData* shared = local->shared;
    int family_id = local->family_id;
    int new_total;
    
    /* Check if we should continue before blocking */
    if (!should_continue(local)) {
        return local->basket_bananas;  /* Return current value */
    }
    
    sem_wait(&shared->basket_locks[family_id]);
    
    /* Always read from shared memory first (authoritative source) */
    local->basket_bananas = shared->families[family_id].basket_bananas;
    local->basket_bananas += amount;
    shared->families[family_id].basket_bananas = local->basket_bananas;
    new_total = local->basket_bananas;
    
    sem_post(&shared->basket_locks[family_id]);
    
    return new_total;
}

/*
 * Get current basket count
 */
int get_basket_count(FamilyLocal* local) {
    SharedData* shared = local->shared;
    int family_id = local->family_id;
    int count;
    
    /* Check if we should continue before blocking */
    if (!should_continue(local)) {
        return local->basket_bananas;  /* Return cached value */
    }
    
    sem_wait(&shared->basket_locks[family_id]);
    count = shared->families[family_id].basket_bananas;
    local->basket_bananas = count;  /* Update local cache */
    sem_post(&shared->basket_locks[family_id]);
    
    return count;
}

/*
 * Check if simulation should continue
 */
int should_continue(FamilyLocal* local) {
    return local->shared->simulation_running && 
           !local->should_withdraw &&
           local->shared->families[local->family_id].is_active;
}

/*
 * Initialize family local data
 */
void init_family_local(FamilyLocal* local, int family_id, 
                       SharedData* shared, const SimConfig* config) {
    int i;
    
    memset(local, 0, sizeof(FamilyLocal));
    
    local->family_id = family_id;
    local->shared = shared;
    local->config = config;
    
    local->basket_bananas = 0;
    local->male_energy = config->male_initial_energy;
    local->female_energy = config->female_initial_energy;
    local->female_collected = 0;
    local->female_in_maze = 0;
    local->female_resting = 0;
    local->male_fighting = 0;
    local->should_withdraw = 0;
    
    local->num_babies = config->babies_per_family;
    for (i = 0; i < MAX_BABIES; i++) {
        local->baby_eaten[i] = 0;
    }
    
    pthread_mutex_init(&local->family_lock, NULL);
    pthread_cond_init(&local->fight_started, NULL);
    pthread_cond_init(&local->fight_ended, NULL);
    
    /* Initialize shared status */
    shared->families[family_id].is_active = 1;
    shared->families[family_id].basket_bananas = 0;
    shared->families[family_id].male_fighting = 0;
    shared->families[family_id].female_fighting = 0;
    shared->families[family_id].female_opponent = -1;
    shared->families[family_id].male_energy = config->male_initial_energy;
    shared->families[family_id].female_energy = config->female_initial_energy;
    shared->families[family_id].female_in_maze = 0;
    shared->families[family_id].female_resting = 0;
    shared->families[family_id].female_collected = 0;
    shared->families[family_id].total_collected = 0;
    shared->families[family_id].bananas_from_maze = 0;
    shared->families[family_id].bananas_from_male_fights = 0;
    shared->families[family_id].bananas_from_female_fights = 0;
    shared->families[family_id].bananas_lost_male_fights = 0;
    shared->families[family_id].bananas_lost_female_fights = 0;
    for (i = 0; i < MAX_BABIES; i++) {
        shared->families[family_id].baby_bananas_eaten[i] = 0;
    }
    
}

void cleanup_family_local(FamilyLocal* local) {
    pthread_mutex_destroy(&local->family_lock);
    pthread_cond_destroy(&local->fight_started);
    pthread_cond_destroy(&local->fight_ended);
}

void female_fight(FamilyLocal* local, int other_family_id) {
    SharedData* shared = local->shared;
    int my_id = local->family_id;
    
    /* Check if simulation is still running */
    if (!should_continue(local)) {
        return;
    }
    
    /* Lock both family baskets in order to prevent deadlock */
    int first = (my_id < other_family_id) ? my_id : other_family_id;
    int second = (my_id < other_family_id) ? other_family_id : my_id;
    
    sem_wait(&shared->basket_locks[first]);
    if (!should_continue(local)) {
        sem_post(&shared->basket_locks[first]);
        return;
    }
    
    sem_wait(&shared->basket_locks[second]);
    if (!should_continue(local)) {
        sem_post(&shared->basket_locks[second]);
        sem_post(&shared->basket_locks[first]);
        return;
    }
    
    /* Get other female's collected bananas from shared memory */
    int other_collected = shared->families[other_family_id].female_collected;
    int my_collected = local->female_collected;
    
    /* Mark both females as fighting */
    shared->families[my_id].female_fighting = 1;
    shared->families[my_id].female_opponent = other_family_id;
    shared->families[other_family_id].female_fighting = 1;
    shared->families[other_family_id].female_opponent = my_id;
    
    add_shared_event(shared, "FEMALE FIGHT: Fam%d vs Fam%d (carrying %d vs %d)", 
                     my_id, other_family_id, my_collected, other_collected);
    
    /* Random winner */
    int i_win = random_chance(0.5);
    
    if (i_win) {
        /* I win - take their bananas */
        local->female_collected += other_collected;
        shared->families[other_family_id].female_collected = 0;
        
        /* Track banana flow */
        shared->families[my_id].bananas_from_female_fights += other_collected;
        shared->families[other_family_id].bananas_lost_female_fights += other_collected;
        
        add_shared_event(shared, "Female %d WON! Took %d bananas from Female %d", 
                         my_id, other_collected, other_family_id);
    } else {
        /* They win - they take my bananas */
        shared->families[other_family_id].female_collected += my_collected;
        local->female_collected = 0;
        
        /* Track banana flow */
        shared->families[my_id].bananas_lost_female_fights += my_collected;
        shared->families[other_family_id].bananas_from_female_fights += my_collected;
        
        add_shared_event(shared, "Female %d LOST! Lost %d bananas to Female %d", 
                         my_id, my_collected, other_family_id);
    }
    
    /* Update my collected in shared memory */
    shared->families[my_id].female_collected = local->female_collected;
    
    /* Both lose energy */
    pthread_mutex_lock(&local->family_lock);
    local->female_energy -= local->config->female_fight_energy_cost;
    if (local->female_energy < 0) local->female_energy = 0;  /* Prevent negative */
    shared->families[my_id].female_energy = local->female_energy;
    pthread_mutex_unlock(&local->family_lock);
    
    /* Clear fighting flags */
    shared->families[my_id].female_fighting = 0;
    shared->families[my_id].female_opponent = -1;
    shared->families[other_family_id].female_fighting = 0;
    shared->families[other_family_id].female_opponent = -1;
    
    sem_post(&shared->basket_locks[second]);
    sem_post(&shared->basket_locks[first]);
}

/* ==================== Male Fight ==================== */

/*
 * Execute male fight with neighbor
 */
void male_fight(FamilyLocal* local, int opponent_id) {
    SharedData* shared = local->shared;
    int my_id = local->family_id;
    
    /* Check if simulation is still running */
    if (!should_continue(local)) {
        return;
    }
    
    /* Check if opponent is still active */
    if (!shared->families[opponent_id].is_active) {
        return;
    }
    
    /* Lock both baskets in order to prevent deadlock */
    int first = (my_id < opponent_id) ? my_id : opponent_id;
    int second = (my_id < opponent_id) ? opponent_id : my_id;
    
    sem_wait(&shared->basket_locks[first]);
    if (!should_continue(local)) {
        sem_post(&shared->basket_locks[first]);
        return;
    }
    
    sem_wait(&shared->basket_locks[second]);
    if (!should_continue(local)) {
        sem_post(&shared->basket_locks[second]);
        sem_post(&shared->basket_locks[first]);
        return;
    }
    
    /* Read CURRENT values from shared memory (authoritative source) */
    local->basket_bananas = shared->families[my_id].basket_bananas;
    int my_basket = local->basket_bananas;
    int their_basket = shared->families[opponent_id].basket_bananas;
    
    add_shared_event(shared, "MALE FIGHT: Fam%d vs Fam%d (basket %d vs %d)", 
                     my_id, opponent_id, my_basket, their_basket);
    
    /* Signal that fight started - babies can steal! */
    pthread_mutex_lock(&local->family_lock);
    local->male_fighting = 1;
    shared->families[my_id].male_fighting = 1;
    shared->families[opponent_id].male_fighting = 1;
    pthread_cond_broadcast(&local->fight_started);
    pthread_mutex_unlock(&local->family_lock);
    
    /* Fight duration - release locks during sleep to allow babies to steal */
    sem_post(&shared->basket_locks[second]);
    sem_post(&shared->basket_locks[first]);
    
    sleep_ms(200 + random_int(0, 300));
    
    /* Re-acquire locks to determine outcome */
    sem_wait(&shared->basket_locks[first]);
    sem_wait(&shared->basket_locks[second]);
    
    /* Re-read current values (may have changed during fight!) */
    my_basket = shared->families[my_id].basket_bananas;
    their_basket = shared->families[opponent_id].basket_bananas;
    
    /* Determine winner */
    int i_win = random_chance(0.5);
    
    if (i_win) {
        /* I win - take their basket */
        local->basket_bananas = my_basket + their_basket;
        shared->families[opponent_id].basket_bananas = 0;
        shared->families[my_id].basket_bananas = local->basket_bananas;
        
        /* Track banana flow */
        shared->families[my_id].bananas_from_male_fights += their_basket;
        shared->families[opponent_id].bananas_lost_male_fights += their_basket;
        
        add_shared_event(shared, "Male %d WON! Took %d from Male %d (basket=%d)", 
                         my_id, their_basket, opponent_id, local->basket_bananas);
        
        /* Check winning threshold */
        if (local->basket_bananas >= local->config->winning_basket_threshold) {
            sem_wait(&shared->global_lock);
            shared->simulation_running = 0;
            shared->termination_reason = TERM_BASKET_THRESHOLD;
            shared->winning_family = my_id;
            sem_post(&shared->global_lock);
            add_shared_event(shared, "Family %d WINS! Reached basket threshold!", my_id);
        }
    } else {
        /* They win - lose my basket */
        shared->families[opponent_id].basket_bananas = their_basket + my_basket;
        local->basket_bananas = 0;
        shared->families[my_id].basket_bananas = 0;
        
        /* Track banana flow */
        shared->families[opponent_id].bananas_from_male_fights += my_basket;
        shared->families[my_id].bananas_lost_male_fights += my_basket;
        
        add_shared_event(shared, "Male %d LOST! Lost %d bananas to Male %d", 
                         my_id, my_basket, opponent_id);
    }
    
    /* BOTH fighters lose energy */
    pthread_mutex_lock(&local->family_lock);
    local->male_energy -= local->config->male_fight_energy_cost;
    if (local->male_energy < 0) local->male_energy = 0;  /* Prevent negative */
    shared->families[my_id].male_energy = local->male_energy;
    pthread_mutex_unlock(&local->family_lock);
    
    /* OPPONENT also loses energy! */
    int opponent_old_energy = shared->families[opponent_id].male_energy;
    shared->families[opponent_id].male_energy -= local->config->male_fight_energy_cost;
    if (shared->families[opponent_id].male_energy < 0) {
        shared->families[opponent_id].male_energy = 0;
    }
    
    add_shared_event(shared, "Male %d energy: %d->%d, Male %d energy: %d->%d (fight cost: %d each)", 
                     my_id, local->male_energy + local->config->male_fight_energy_cost, local->male_energy,
                     opponent_id, opponent_old_energy, shared->families[opponent_id].male_energy,
                     local->config->male_fight_energy_cost);
    
    /* Signal fight ended */
    pthread_mutex_lock(&local->family_lock);
    local->male_fighting = 0;
    shared->families[my_id].male_fighting = 0;
    shared->families[opponent_id].male_fighting = 0;
    pthread_cond_broadcast(&local->fight_ended);
    pthread_mutex_unlock(&local->family_lock);
    
    sem_post(&shared->basket_locks[second]);
    sem_post(&shared->basket_locks[first]);
}


void* female_thread(void* arg) {
    FamilyLocal* local = (FamilyLocal*)arg;
    SharedData* shared = local->shared;
    const SimConfig* config = local->config;
    int family_id = local->family_id;
    
    while (should_continue(local)) {
        /* Check if resting */
        if (local->female_resting) {
            sleep_ms(1000);
            
            pthread_mutex_lock(&local->family_lock);
            int old_energy = local->female_energy;
            local->female_energy += config->female_rest_recovery;
            if (local->female_energy > config->female_initial_energy) {
                local->female_energy = config->female_initial_energy;
            }
            local->female_resting = 0;
            shared->families[family_id].female_resting = 0;  /* Clear resting flag */
            shared->families[family_id].female_energy = local->female_energy;
            pthread_mutex_unlock(&local->family_lock);
            
            add_shared_event(shared, "Female %d recovered energy (%d -> %d)", 
                             family_id, old_energy, local->female_energy);
            continue;
        }
        
        /* Check energy level */
        if (local->female_energy <= 0) {
            /* ZERO energy - MUST rest, even if carrying bananas (vulnerable!) */
            local->female_resting = 1;
            shared->families[family_id].female_resting = 1;  /* Mark as resting in shared memory */
            add_shared_event(shared, "Female %d EXHAUSTED (energy=0)! Resting in maze%s", 
                             family_id, 
                             local->female_collected > 0 ? " - VULNERABLE with bananas!" : "");
            continue;
        } else if (local->female_energy < config->female_rest_threshold) {
            /* Low energy but not zero - if carrying bananas, head to exit first */
            if (!(local->female_collected > 0 && local->female_in_maze)) {
                local->female_resting = 1;
                shared->families[family_id].female_resting = 1;
                add_shared_event(shared, "Female %d resting (energy=%d < threshold=%d)", 
                                 family_id, local->female_energy, config->female_rest_threshold);
                continue;
            }
        }
        
        /* If not in maze, enter from bottom border (row 19 = entry) */
        if (!local->female_in_maze) {
            if (get_random_start_position(shared, &local->female_x, &local->female_y)) {
                local->female_in_maze = 1;
                set_female_in_cell(shared, local->female_x, local->female_y, family_id, 1);
                
                shared->families[family_id].female_in_maze = 1;
                shared->families[family_id].female_x = local->female_x;
                shared->families[family_id].female_y = local->female_y;
                
                add_shared_event(shared, ">>> Female %d ENTERED maze at BORDER row %d, col %d", 
                                 family_id, local->female_x, local->female_y);
            } else {
                sleep_ms(500);
                continue;
            }
        }
        
        /* In maze - decide what to do */
        
        /* Check for collision with other female */
        if (should_continue(local)) {
            sem_wait(&shared->maze_locks[local->female_x][local->female_y]);
            int other = check_female_collision(shared, local->female_x, local->female_y, family_id);
            sem_post(&shared->maze_locks[local->female_x][local->female_y]);
            
            if (other >= 0 && should_continue(local)) {
                /* Check if other female is resting with 0 energy - STEAL without fight! */
                if (shared->families[other].female_resting && 
                    shared->families[other].female_energy <= 0 &&
                    shared->families[other].female_collected > 0) {
                    
                    /* Steal bananas without fighting - she has no energy to resist! */
                    int stolen = shared->families[other].female_collected;
                    
                    pthread_mutex_lock(&local->family_lock);
                    local->female_collected += stolen;
                    shared->families[family_id].female_collected = local->female_collected;
                    shared->families[family_id].bananas_from_female_fights += stolen;
                    pthread_mutex_unlock(&local->family_lock);
                    
                    shared->families[other].female_collected = 0;
                    shared->families[other].bananas_lost_female_fights += stolen;
                    
                    add_shared_event(shared, "Female %d STOLE %d bananas from EXHAUSTED Female %d (no fight!)", 
                                     family_id, stolen, other);
                } else if (shared->families[other].female_collected > 0 || local->female_collected > 0) {
                    /* Normal fight - both have energy */
                    female_fight(local, other);
                }
            }
        }
        
        /* Check if at exit row (row 0) - ONLY place to exit maze */
        if (local->female_x == 0) {
            /* At exit - leave maze and deposit bananas */
            set_female_in_cell(shared, local->female_x, local->female_y, family_id, 0);
            local->female_in_maze = 0;
            shared->families[family_id].female_in_maze = 0;
            
            add_shared_event(shared, "Female %d exited maze at EXIT row 0, col %d", 
                             family_id, local->female_y);
            
            if (local->female_collected > 0) {
                /* Deposit to basket using thread-safe function */
                int collected = local->female_collected;
                int new_total = add_to_basket(local, collected);
                
                pthread_mutex_lock(&local->family_lock);
                local->female_collected = 0;
                shared->families[family_id].female_collected = 0;
                shared->families[family_id].total_collected += collected;
                pthread_mutex_unlock(&local->family_lock);
                
                add_shared_event(shared, "Female %d deposited %d bananas (basket=%d)", 
                                 family_id, collected, new_total);
                
                /* Check winning threshold */
                if (new_total >= config->winning_basket_threshold) {
                    sem_wait(&shared->global_lock);
                    shared->simulation_running = 0;
                    shared->termination_reason = TERM_BASKET_THRESHOLD;
                    shared->winning_family = family_id;
                    sem_post(&shared->global_lock);
                    add_shared_event(shared, "Family %d WINS! Basket threshold reached!", family_id);
                }
            } else {
                add_shared_event(shared, "Female %d exited empty-handed", family_id);
            }
            
            sleep_ms(300);  /* Brief rest before re-entering */
            continue;
        }
        
        /* Collect bananas at current cell */
        if (should_continue(local)) {
            int bananas_here = get_bananas_at(shared, local->female_x, local->female_y);
            if (bananas_here > 0 && should_continue(local)) {
                int to_take = bananas_here;
                /* Don't take more than needed to reach goal */
                int needed = config->female_collection_goal - local->female_collected;
                if (to_take > needed) to_take = needed;
                
                int taken = take_bananas(shared, local->female_x, local->female_y, to_take);
                if (should_continue(local)) {
                    local->female_collected += taken;
                    shared->families[family_id].female_collected = local->female_collected;
                    
                    /* Track collection from maze */
                    shared->families[family_id].bananas_from_maze += taken;
                    add_shared_event(shared, "Female %d collected %d at (%d,%d), carrying=%d", 
                                     family_id, taken, local->female_x, local->female_y, local->female_collected);
                }
            }
        }
        
        /* Decide direction: towards exit if have enough, else explore */
        int direction;
        if (local->female_collected >= config->female_collection_goal || 
            local->female_energy < config->female_rest_threshold) {
            direction = get_direction_to_exit(shared, local->female_x, local->female_y);
        } else {
            direction = get_direction_to_explore(shared, local->female_x, local->female_y);
        }
        
        if (direction >= 0) {
            /* Remove from current cell */
            set_female_in_cell(shared, local->female_x, local->female_y, family_id, 0);
            
            /* Move */
            int old_x = local->female_x, old_y = local->female_y;
            if (move_in_direction(shared, &local->female_x, &local->female_y, direction)) {
                /* Add to new cell */
                set_female_in_cell(shared, local->female_x, local->female_y, family_id, 1);
                
                shared->families[family_id].female_x = local->female_x;
                shared->families[family_id].female_y = local->female_y;
                
                /* Lose energy for moving */
                pthread_mutex_lock(&local->family_lock);
                local->female_energy -= config->female_move_energy_cost;
                if (local->female_energy < 0) local->female_energy = 0;  /* Prevent negative */
                shared->families[family_id].female_energy = local->female_energy;
                pthread_mutex_unlock(&local->family_lock);
            } else {
                /* Couldn't move, stay in place */
                set_female_in_cell(shared, old_x, old_y, family_id, 1);
            }
        }
        
        sleep_ms(300);  /* Movement delay */
    }
    
    /* Cleanup: remove from maze if still there */
    if (local->female_in_maze) {
        set_female_in_cell(shared, local->female_x, local->female_y, family_id, 0);
    }
    
    return NULL;
}


void* male_thread(void* arg) {
    FamilyLocal* local = (FamilyLocal*)arg;
    SharedData* shared = local->shared;
    const SimConfig* config = local->config;
    int family_id = local->family_id;
    
    int left_neighbor, right_neighbor;
    get_neighbors(family_id, shared->num_families, &left_neighbor, &right_neighbor);
    
    while (should_continue(local)) {
        /* SYNC energy from shared memory - another male might have decreased it! */
        pthread_mutex_lock(&local->family_lock);
        local->male_energy = shared->families[family_id].male_energy;
        pthread_mutex_unlock(&local->family_lock);
        
        /* Check energy level */
        if (local->male_energy < config->male_withdraw_threshold) {
            /* Mark family as withdrawn */
            pthread_mutex_lock(&local->family_lock);
            local->should_withdraw = 1;
            shared->families[family_id].is_active = 0;
            pthread_mutex_unlock(&local->family_lock);
            
            /* Update global withdrawn count */
            sem_wait(&shared->global_lock);
            shared->withdrawn_count++;
            
            if (shared->withdrawn_count >= config->max_withdrawn_families) {
                shared->simulation_running = 0;
                shared->termination_reason = TERM_WITHDRAWN_THRESHOLD;
                add_shared_event(shared, "Too many families withdrawn! Simulation ends!");
            }
            sem_post(&shared->global_lock);
            
            add_shared_event(shared, "Family %d WITHDRAWN! Male energy=%d, basket=%d", 
                             family_id, local->male_energy, local->basket_bananas);
            break;
        }
        
        /* Try to start a fight with neighbors */
        int target = -1;
        
        /* Check if we should continue before any blocking operations */
        if (!should_continue(local)) {
            break;
        }
        
        /* Get our current basket from shared memory */
        int my_bananas = get_basket_count(local);
        
        if (!should_continue(local)) {
            break;
        }
        
        /* Check left neighbor */
        if (left_neighbor >= 0 && shared->families[left_neighbor].is_active && should_continue(local)) {
            int their_bananas = shared->families[left_neighbor].basket_bananas;
            float prob = calculate_fight_probability(my_bananas, their_bananas, config);
            
            if (random_chance(prob)) {
                target = left_neighbor;
                add_shared_event(shared, "Male %d decides to fight Male %d (prob=%.0f%%, baskets: %d vs %d)",
                                 family_id, left_neighbor, prob * 100, my_bananas, their_bananas);
            }
        }
        
        /* Check right neighbor if no fight with left */
        if (target < 0 && right_neighbor >= 0 && shared->families[right_neighbor].is_active && should_continue(local)) {
            int their_bananas = shared->families[right_neighbor].basket_bananas;
            float prob = calculate_fight_probability(my_bananas, their_bananas, config);
            
            if (random_chance(prob)) {
                target = right_neighbor;
                add_shared_event(shared, "Male %d decides to fight Male %d (prob=%.0f%%, baskets: %d vs %d)",
                                 family_id, right_neighbor, prob * 100, my_bananas, their_bananas);
            }
        }
        
        /* Execute fight if target found */
        if (target >= 0 && should_continue(local)) {
            male_fight(local, target);
        }
        
        sleep_ms(500);  /* Check interval */
    }
    
    /* Wake up babies so they can exit */
    pthread_mutex_lock(&local->family_lock);
    local->should_withdraw = 1;  /* Signal all threads to stop */
    pthread_cond_broadcast(&local->fight_started);
    pthread_cond_broadcast(&local->fight_ended);
    pthread_mutex_unlock(&local->family_lock);
    
    return NULL;
}

void* baby_thread(void* arg) {
    BabyArg* baby_arg = (BabyArg*)arg;
    int baby_id = baby_arg->baby_id;
    FamilyLocal* local = baby_arg->family;
    SharedData* shared = local->shared;
    const SimConfig* config = local->config;
    int family_id = local->family_id;
    
    while (should_continue(local)) {
        /* Wait for a fight to start */
        pthread_mutex_lock(&local->family_lock);
        
        while (!local->male_fighting && should_continue(local)) {
            /* Use timed wait to periodically check if we should exit */
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;  /* 1 second timeout */
            pthread_cond_timedwait(&local->fight_started, &local->family_lock, &ts);
        }
        
        pthread_mutex_unlock(&local->family_lock);
        
        if (!should_continue(local)) break;
        
        /* Dad is fighting! ONE opportunity to steal per fight! */
        
        /* Find a target family (not our own) */
        int target = -1;
        int attempts = 0;
        
        while (target < 0 && attempts < 10) {
            int candidate = random_int(0, shared->num_families - 1);
            
            if (candidate != family_id && 
                shared->families[candidate].is_active &&
                shared->families[candidate].basket_bananas > 0) {
                target = candidate;
            }
            attempts++;
        }
        
        if (target >= 0 && should_continue(local)) {
            /* Try to steal - need to lock both target basket and our basket */
            int first_lock = (target < family_id) ? target : family_id;
            int second_lock = (target < family_id) ? family_id : target;
            
            if (!should_continue(local)) break;
            
            sem_wait(&shared->basket_locks[first_lock]);
            if (!should_continue(local)) {
                sem_post(&shared->basket_locks[first_lock]);
                break;
            }
            
            sem_wait(&shared->basket_locks[second_lock]);
            if (!should_continue(local)) {
                sem_post(&shared->basket_locks[second_lock]);
                sem_post(&shared->basket_locks[first_lock]);
                break;
            }
            
            int available = shared->families[target].basket_bananas;
            if (available > 0) {
                /* Baby steals 1-2 bananas (reduced from 1-3) */
                int stolen = random_int(1, 2);
                if (stolen > available) stolen = available;
                
                shared->families[target].basket_bananas -= stolen;
                
                /* Decide: eat or give to dad? */
                if (random_chance(0.5)) {
                    /* Eat! - bananas are consumed, removed from circulation */
                    local->baby_eaten[baby_id] += stolen;
                    
                    add_shared_event(shared, "Baby%d Fam%d stole %d from Fam%d & ATE (total eaten: %d)", 
                                     baby_id, family_id, stolen, target, local->baby_eaten[baby_id]);
                    
                    /* Check termination threshold */
                    if (local->baby_eaten[baby_id] >= config->baby_eaten_threshold) {
                        sem_wait(&shared->global_lock);
                        shared->simulation_running = 0;
                        shared->termination_reason = TERM_BABY_ATE_THRESHOLD;
                        shared->winning_family = family_id;
                        sem_post(&shared->global_lock);
                        
                        add_shared_event(shared, "Baby%d Fam%d ate too much! Simulation ends!", baby_id, family_id);
                    }
                } else {
                    /* Give to dad's basket - read current value first! */
                    local->basket_bananas = shared->families[family_id].basket_bananas;
                    local->basket_bananas += stolen;
                    shared->families[family_id].basket_bananas = local->basket_bananas;
                    
                    add_shared_event(shared, "Baby%d Fam%d stole %d from Fam%d, gave to Dad (basket=%d)", 
                                     baby_id, family_id, stolen, target, local->basket_bananas);
                }
            }
            
            sem_post(&shared->basket_locks[second_lock]);
            sem_post(&shared->basket_locks[first_lock]);
        }
        
        /* IMPORTANT: Wait for THIS fight to end before looking for another opportunity */
        /* This limits baby to ONE steal attempt per fight */
        pthread_mutex_lock(&local->family_lock);
        while (local->male_fighting && should_continue(local)) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
            pthread_cond_timedwait(&local->fight_ended, &local->family_lock, &ts);
        }
        pthread_mutex_unlock(&local->family_lock);
    }
    
    /* Save baby's consumption to shared memory for final statistics */
    shared->families[family_id].baby_bananas_eaten[baby_id] = local->baby_eaten[baby_id];
    
    return NULL;
}

/* ==================== Main Family Process ==================== */

void run_family_process(int family_id, SharedData* shared, const SimConfig* config) {
    FamilyLocal local;
    pthread_t female_tid, male_tid;
    pthread_t baby_tids[MAX_BABIES];
    BabyArg baby_args[MAX_BABIES];
    int i;
    
    /* Initialize random seed for this process */
    init_random();
    
    /* Initialize family local data */
    init_family_local(&local, family_id, shared, config);
    
    add_shared_event(shared, "Family %d started (Male:%d, Female:%d, Babies:%d)", 
                     family_id, config->male_initial_energy, config->female_initial_energy, 
                     config->babies_per_family);
    
    /* Create female thread */
    if (pthread_create(&female_tid, NULL, female_thread, &local) != 0) {
        perror("Failed to create female thread");
        cleanup_family_local(&local);
        exit(1);
    }
    
    /* Create male thread */
    if (pthread_create(&male_tid, NULL, male_thread, &local) != 0) {
        perror("Failed to create male thread");
        /* Try to wait for female thread before exiting */
        pthread_join(female_tid, NULL);
        cleanup_family_local(&local);
        exit(1);
    }
    
    /* Create baby threads */
    for (i = 0; i < config->babies_per_family; i++) {
        baby_args[i].baby_id = i;
        baby_args[i].family = &local;
        
        if (pthread_create(&baby_tids[i], NULL, baby_thread, &baby_args[i]) != 0) {
            perror("Failed to create baby thread");
        }
    }
    
    /* Wait for all threads to complete */
    pthread_join(female_tid, NULL);
    pthread_join(male_tid, NULL);
    
    for (i = 0; i < config->babies_per_family; i++) {
        pthread_join(baby_tids[i], NULL);
    }
    
    cleanup_family_local(&local);
    
    /* Note: Shared memory and config cleanup handled in main.c child process code
     * We just exit here - the parent will handle shared memory cleanup */
    exit(0);
}

