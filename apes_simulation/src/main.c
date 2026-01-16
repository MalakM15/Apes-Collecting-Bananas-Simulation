#include "local.h"

/* Global variables for signal handling */
static int shm_id = -1;
static SharedData* shared = NULL;
static pid_t* child_pids = NULL;
static int num_children = 0;

void signal_handler(int sig) { //  Signal handler for cleanup on Ctrl+C

    int i;
    
    printf("\n\nReceived signal %d, cleaning up...\n", sig);
    
    /* Stop simulation */
    if (shared != NULL) {
        shared->simulation_running = 0;
    }
    
    /* Kill child processes */
    for (i = 0; i < num_children; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGTERM);
        }
    }
    
    /* Wait for children */
    for (i = 0; i < num_children; i++) {
        if (child_pids[i] > 0) {
            waitpid(child_pids[i], NULL, 0);
        }
    }
    
    /* Cleanup shared memory */
    if (shared != NULL) {
        cleanup_maze(shared);
        
        /* Cleanup semaphores using wrapper */
        cleanup_simulation_semaphores(shared->num_families, shared->maze_rows, shared->maze_cols);
        
        /* Cleanup event lock */
        sem_destroy(&shared->event_lock);
        
        detach_shared_memory(shared);
    }
    
    if (shm_id >= 0) {
        destroy_shared_memory(shm_id);
    }
    
    if (child_pids != NULL) {
        free(child_pids);
    }
    
    printf("Cleanup complete. Exiting.\n");
    exit(0);
}

int init_shared_data(const SimConfig* config) {
    int i;
    
    /* Create shared memory */
    shm_id = create_shared_memory(sizeof(SharedData), SHM_KEY);
    if (shm_id < 0) {
        fprintf(stderr, "Failed to create shared memory\n");
        return -1;
    }
    
    /* Attach shared memory */
    shared = (SharedData*)attach_shared_memory(shm_id);
    if (shared == NULL) {
        fprintf(stderr, "Failed to attach shared memory\n");
        destroy_shared_memory(shm_id);
        return -1;
    }
    
    /* Initialize shared data */
    memset(shared, 0, sizeof(SharedData));
    
    shared->num_families = config->num_families;
    shared->withdrawn_count = 0;
    shared->simulation_running = 1;
    shared->termination_reason = TERM_RUNNING;
    shared->winning_family = -1;
    shared->start_time = 0;  /* Will be set when simulation actually starts */
    
    /* Initialize semaphores using cross-platform wrapper */
    if (init_simulation_semaphores(shared, config->num_families,  
                                    config->maze_rows, config->maze_cols) != 0) {
        fprintf(stderr, "Failed to initialize semaphores\\n");
        return -1;
    }
    
    /* Initialize event buffer */
    shared->event_head = 0;
    memset(shared->recent_events, 0, sizeof(shared->recent_events));
    if (sem_init(&shared->event_lock, 1, 1) != 0) {
        fprintf(stderr, "Failed to initialize event lock\n");
        return -1;
    }
    
    /* Initialize family status */
    for (i = 0; i < MAX_FAMILIES; i++) {
        shared->families[i].is_active = 0;
        shared->families[i].basket_bananas = 0;
        shared->families[i].male_fighting = 0;
    }
    
    log_event("Shared memory initialized (ID: %d)", shm_id);
    
    return 0;
}
void* monitor_thread(void* arg) {
    const SimConfig* config = (const SimConfig*)arg;
    
    while (shared->simulation_running) {
        /* Check timeout */
        double elapsed = get_elapsed_seconds(shared->start_time);
        
        if (elapsed >= config->max_simulation_time_seconds) {
            sem_wait(&shared->global_lock);
            if (shared->simulation_running) {
                shared->simulation_running = 0;
                shared->termination_reason = TERM_TIMEOUT;
                log_event("TIMEOUT! Simulation time exceeded %d seconds", 
                         config->max_simulation_time_seconds);
            }
            sem_post(&shared->global_lock);
            break;
        }
        
        sleep_ms(500);  /* Check every 500ms */
    }
    
    return NULL;
}

void* display_thread(void* arg) {
    const SimConfig* config = (const SimConfig*)arg;
    int i;
    
    while (shared->simulation_running) {
        /* Move cursor to home and clear screen */
        printf("\033[H\033[J");
        
        double elapsed = get_elapsed_seconds(shared->start_time);
        
        /* Header */
        printf("================================================================================\n");
        printf("  APES SIMULATION | Time: %.0fs/%ds | Bananas in maze: %d | Withdrawn: %d/%d\n",
               elapsed, config->max_simulation_time_seconds,
               shared->total_bananas_in_maze,
               shared->withdrawn_count, config->max_withdrawn_families);
        printf("================================================================================\n\n");
        
        /* Family status - ALL families */
        printf("FAMILY STATUS:\n");
        printf("------------------------------------------------------------------------------\n");
        for (i = 0; i < shared->num_families; i++) {
            FamilyStatus* f = &shared->families[i];
            
            /* Build status string */
            char status[32] = "";
            if (!f->is_active) {
                strcpy(status, "WITHDRAWN");
            } else if (f->male_fighting) {
                strcpy(status, "MALE FIGHTING");
            } else if (f->female_fighting) {
                strcpy(status, "FEMALE FIGHTING");
            } else {
                strcpy(status, "Active");
            }
            
            /* Build female location string */
            char female_loc[80];
            if (f->female_in_maze) {
                if (f->female_resting && f->female_collected > 0) {
                    snprintf(female_loc, sizeof(female_loc), "Female@(%d,%d) RESTING carry=%d VULNERABLE!", 
                             f->female_x, f->female_y, f->female_collected);
                } else if (f->female_resting) {
                    snprintf(female_loc, sizeof(female_loc), "Female@(%d,%d) RESTING", 
                             f->female_x, f->female_y);
                } else {
                    snprintf(female_loc, sizeof(female_loc), "Female@(%d,%d) carry=%d", 
                             f->female_x, f->female_y, f->female_collected);
                }
            } else {
                if (f->female_resting) {
                    strcpy(female_loc, "Female at basket RESTING");
                } else {
                    strcpy(female_loc, "Female at basket");
                }
            }
            
            /* Print ALL info for every family */
            printf("[Family %d] %-15s | Basket: %2d | M:%3d F:%3d | %s\n",
                   i, status, f->basket_bananas, f->male_energy, f->female_energy, female_loc);
        }
        printf("------------------------------------------------------------------------------\n");
        
        /* Show maze */
        printf("\nMAZE (Row 0=Exit, Row %d=Entry):\n", config->maze_rows - 1);
        print_maze_compact(shared);
        
        /* Show recent events */
        printf("\nRECENT EVENTS:\n");
        printf("------------------------------------------------------------------------------\n");
        
        sem_wait(&shared->event_lock);
        int event_count = 0;
        for (i = 0; i < MAX_EVENTS; i++) {
            int idx = (shared->event_head + i) % MAX_EVENTS;
            EventEntry* e = &shared->recent_events[idx];
            
            if (e->message[0] != '\0') {
                printf("[t=%5.1fs] %s\n", e->timestamp, e->message);
                event_count++;
            }
        }
        sem_post(&shared->event_lock);
        
        if (event_count == 0) {
            printf("(No events yet)\n");
        }
        printf("------------------------------------------------------------------------------\n");
        
        printf("\nPress Ctrl+C to stop simulation\n");
        
        fflush(stdout);
        sleep_ms(1000);  /* Update display every 1 second */
    }
    
    return NULL;
}

void print_final_results(const SimConfig* config) {
    int i, j;
    int max_basket = 0;
    int winner = -1;
    int total_in_baskets = 0;
    int total_eaten = 0;
    int total_collected = 0;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                    SIMULATION ENDED                            ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    /* Termination reason */
    printf("║ Reason: ");
    switch (shared->termination_reason) {
        case TERM_WITHDRAWN_THRESHOLD:
            printf("Too many families withdrew (%d/%d)                    ║\n",
                   shared->withdrawn_count, config->max_withdrawn_families);
            break;
        case TERM_BASKET_THRESHOLD:
            printf("Family %d reached basket threshold (%d bananas)       ║\n",
                   shared->winning_family, config->winning_basket_threshold);
            break;
        case TERM_BABY_ATE_THRESHOLD:
            printf("Baby in Family %d ate too much (%d bananas)           ║\n",
                   shared->winning_family, config->baby_eaten_threshold);
            break;
        case TERM_TIMEOUT:
            printf("Simulation time exceeded (%d seconds)                 ║\n",
                   config->max_simulation_time_seconds);
            break;
        default:
            printf("Unknown                                               ║\n");
    }
    
    double elapsed = get_elapsed_seconds(shared->start_time);
    printf("║ Duration: %.1f seconds                                        ║\n", elapsed);
    
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ DETAILED BANANA STATISTICS                                     ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    /* Detailed per-family statistics */
    for (i = 0; i < shared->num_families; i++) {
        FamilyStatus* f = &shared->families[i];
        int family_eaten = 0;
        
        for (j = 0; j < config->babies_per_family; j++) {
            family_eaten += f->baby_bananas_eaten[j];
        }
        
        total_in_baskets += f->basket_bananas;
        total_eaten += family_eaten;
        total_collected += f->total_collected;
        
        printf("║                                                                ║\n");
        printf("║ Family %d [%s]:                                     ║\n",
               i, f->is_active ? "Active   " : "Withdrawn");
        printf("║   📥 COLLECTED:                                                ║\n");
        printf("║      • From maze:        %3d bananas                           ║\n", f->bananas_from_maze);
        printf("║      • From male fights: %3d bananas                           ║\n", f->bananas_from_male_fights);
        printf("║      • From fem. fights: %3d bananas                           ║\n", f->bananas_from_female_fights);
        printf("║   📤 LOST:                                                     ║\n");
        printf("║      • In male fights:   %3d bananas                           ║\n", f->bananas_lost_male_fights);
        printf("║      • In fem. fights:   %3d bananas                           ║\n", f->bananas_lost_female_fights);
        printf("║   📊 FINAL:                                                    ║\n");
        printf("║      • In basket:        %3d bananas                           ║\n", f->basket_bananas);
        printf("║      • Eaten by babies:  %3d bananas                           ║\n", family_eaten);
        
        for (j = 0; j < config->babies_per_family; j++) {
            printf("║     - Baby %d ate: %2d                                           ║\n",
                   j, f->baby_bananas_eaten[j]);
        }
        
        /* Show basket calculation - CORRECT EQUATION */
        /* Step 1: What female deposited = maze + fem_wins - fem_losses */
        int deposited = f->bananas_from_maze + f->bananas_from_female_fights - f->bananas_lost_female_fights;
        /* Step 2: Male fights affect basket directly */
        int after_male_fights = deposited + f->bananas_from_male_fights - f->bananas_lost_male_fights;
        /* Step 3: Babies eat from basket */
        int after_eating = after_male_fights - family_eaten;
        /* Step 4: What's missing must be stolen by other babies */
        int stolen_by_others = after_eating - f->basket_bananas;
        if (stolen_by_others < 0) stolen_by_others = 0;
        
        printf("║   📝 BASKET CALCULATION:                                      ║\n");
        printf("║      From maze:              %3d                              ║\n", f->bananas_from_maze);
        printf("║      + Female fight wins:    %3d  (added while carrying)      ║\n", f->bananas_from_female_fights);
        printf("║      - Female fight losses:  %3d  (lost while carrying)       ║\n", f->bananas_lost_female_fights);
        printf("║      = Total DEPOSITED:      %3d                              ║\n", deposited);
        printf("║      + Male fight wins:      %3d  (stole other's basket)      ║\n", f->bananas_from_male_fights);
        printf("║      - Male fight losses:    %3d  (lost our basket)           ║\n", f->bananas_lost_male_fights);
        printf("║      - Eaten by our babies:  %3d                              ║\n", family_eaten);
        printf("║      - Stolen by other kids: %3d                              ║\n", stolen_by_others);
        printf("║      = FINAL BASKET:         %3d                              ║\n", f->basket_bananas);
        
        if (f->basket_bananas > max_basket) {
            max_basket = f->basket_bananas;
            winner = i;
        }
    }
    
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ SUMMARY TOTALS                                                 ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ Initial bananas in maze:    %3d                                ║\n", config->total_bananas);
    printf("║ Remaining in maze:          %3d                                ║\n", shared->total_bananas_in_maze);
    printf("║ Total collected by females: %3d                                ║\n", total_collected);
    printf("║ Total in baskets (saved):   %3d                                ║\n", total_in_baskets);
    printf("║ Total eaten by babies:      %3d                                ║\n", total_eaten);
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    if (winner >= 0) {
        printf("║                   🏆 WINNER: FAMILY %d 🏆                       ║\n", winner);
        printf("║              (Most bananas in basket: %d)                      ║\n", max_basket);
    } else {
        printf("║                   No clear winner                              ║\n");
    }
    
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    /* Log to file as well */
    FILE* log = fopen("simulation_summary.txt", "w");
    if (log) {
        fprintf(log, "APES SIMULATION - FINAL RESULTS\n");
        fprintf(log, "================================\n\n");
        
        /* Explanation of how the simulation works */
        fprintf(log, "HOW THE SIMULATION WORKS:\n");
        fprintf(log, "-------------------------\n");
        fprintf(log, "- Each family has: 1 Male, 1 Female, %d Babies\n", config->babies_per_family);
        fprintf(log, "- Female enters maze from bottom row (row %d), collects bananas, exits at row 0\n", config->maze_rows - 1);
        fprintf(log, "- Female deposits collected bananas into family basket when exiting\n");
        fprintf(log, "- Males can fight neighboring males - winner takes loser's basket!\n");
        fprintf(log, "- Females can fight if they meet in same cell - winner takes loser's carried bananas\n");
        fprintf(log, "- During male fights, babies can steal from OTHER families' baskets\n");
        fprintf(log, "- Babies either eat stolen bananas (removed from game) or give to their dad's basket\n");
        fprintf(log, "- Male withdraws family if energy drops below %d\n\n", config->male_withdraw_threshold);
        
        fprintf(log, "Duration: %.1f seconds\n", elapsed);
        fprintf(log, "Termination: ");
        switch (shared->termination_reason) {
            case TERM_TIMEOUT: fprintf(log, "Timeout (%d seconds max)\n", config->max_simulation_time_seconds); break;
            case TERM_WITHDRAWN_THRESHOLD: fprintf(log, "Too many families withdrew (%d/%d)\n", shared->withdrawn_count, config->max_withdrawn_families); break;
            case TERM_BASKET_THRESHOLD: fprintf(log, "Family %d reached %d bananas in basket\n", shared->winning_family, config->winning_basket_threshold); break;
            case TERM_BABY_ATE_THRESHOLD: fprintf(log, "Baby ate %d+ bananas\n", config->baby_eaten_threshold); break;
            default: fprintf(log, "Unknown\n");
        }
        
        fprintf(log, "\n================================================================================\n");
        fprintf(log, "PER-FAMILY STATISTICS (with explanations):\n");
        fprintf(log, "================================================================================\n");
        
        for (i = 0; i < shared->num_families; i++) {
            FamilyStatus* f = &shared->families[i];
            int family_eaten = 0;
            for (j = 0; j < config->babies_per_family; j++) {
                family_eaten += f->baby_bananas_eaten[j];
            }
            fprintf(log, "\nFamily %d (%s):\n", i, f->is_active ? "Active" : "Withdrawn");
            fprintf(log, "  COLLECTED (ways family gained bananas):\n");
            fprintf(log, "    From maze:        %3d  <- Female picked up from maze cells\n", f->bananas_from_maze);
            fprintf(log, "    From male fights: %3d  <- Male won fights, took opponent's basket\n", f->bananas_from_male_fights);
            fprintf(log, "    From fem. fights: %3d  <- Female won fights, took opponent's carried bananas\n", f->bananas_from_female_fights);
            fprintf(log, "  LOST (ways family lost bananas):\n");
            fprintf(log, "    In male fights:   %3d  <- Male lost fights, opponent took our basket\n", f->bananas_lost_male_fights);
            fprintf(log, "    In fem. fights:   %3d  <- Female lost fights, opponent took our carried bananas\n", f->bananas_lost_female_fights);
            fprintf(log, "  FINAL STATUS:\n");
            fprintf(log, "    In basket:        %3d  <- Bananas saved in family basket\n", f->basket_bananas);
            fprintf(log, "    Eaten by babies:  %3d  <- Bananas consumed by babies (removed from game)\n", family_eaten);
            for (j = 0; j < config->babies_per_family; j++) {
                fprintf(log, "      Baby %d ate: %2d\n", j, f->baby_bananas_eaten[j]);
            }
            
            /* Calculate and explain the basket value - CORRECT EQUATION WITH FEMALE FIGHTS */
            int file_deposited = f->bananas_from_maze + f->bananas_from_female_fights - f->bananas_lost_female_fights;
            int file_after_male = file_deposited + f->bananas_from_male_fights - f->bananas_lost_male_fights;
            int file_after_eating = file_after_male - family_eaten;
            int file_stolen = file_after_eating - f->basket_bananas;
            if (file_stolen < 0) file_stolen = 0;
            
            fprintf(log, "\n  BASKET CALCULATION (how we get %d bananas in basket):\n", f->basket_bananas);
            fprintf(log, "    ┌──────────────────────────────────────────────────────────────────────┐\n");
            fprintf(log, "    │ EQUATION:                                                           │\n");
            fprintf(log, "    │ Basket = (Maze + FemWins - FemLosses) + MaleWins - MaleLosses       │\n");
            fprintf(log, "    │          - EatenByBabies - StolenByOtherKids                        │\n");
            fprintf(log, "    └──────────────────────────────────────────────────────────────────────┘\n");
            fprintf(log, "    Step 1: Female collects from maze:                +%3d\n", f->bananas_from_maze);
            fprintf(log, "    Step 2: Female wins fights (added to carried):    +%3d\n", f->bananas_from_female_fights);
            fprintf(log, "    Step 3: Female loses fights (lost while carried): -%3d\n", f->bananas_lost_female_fights);
            fprintf(log, "            >>> Total DEPOSITED to basket:            =%3d\n", file_deposited);
            fprintf(log, "    Step 4: Male wins (steals other's basket):        +%3d\n", f->bananas_from_male_fights);
            fprintf(log, "    Step 5: Male loses (our basket stolen):           -%3d\n", f->bananas_lost_male_fights);
            fprintf(log, "    Step 6: Our babies eat from basket:               -%3d\n", family_eaten);
            fprintf(log, "    Step 7: Other babies steal from us:               -%3d (estimated)\n", file_stolen);
            fprintf(log, "    ─────────────────────────────────────────────────────────────────────\n");
            fprintf(log, "    RESULT: (%d + %d - %d) + %d - %d - %d - %d = %d bananas\n",
                    f->bananas_from_maze, f->bananas_from_female_fights, f->bananas_lost_female_fights,
                    f->bananas_from_male_fights, f->bananas_lost_male_fights, 
                    family_eaten, file_stolen, f->basket_bananas);
        }
        
        fprintf(log, "\n================================================================================\n");
        fprintf(log, "TOTALS:\n");
        fprintf(log, "================================================================================\n");
        fprintf(log, "Initial bananas in maze:    %3d\n", config->total_bananas);
        fprintf(log, "Remaining in maze:          %3d  <- Still uncollected\n", shared->total_bananas_in_maze);
        fprintf(log, "Total collected by females: %3d  <- Sum of all females' collections from maze\n", total_collected);
        fprintf(log, "Total in all baskets:       %3d  <- Sum of all family baskets\n", total_in_baskets);
        fprintf(log, "Total eaten by all babies:  %3d  <- Removed from circulation\n", total_eaten);
        
        int accounted = shared->total_bananas_in_maze + total_in_baskets + total_eaten;
        fprintf(log, "\nBALANCE CHECK:\n");
        fprintf(log, "  Initial (%d) should = Remaining (%d) + In baskets (%d) + Eaten (%d) = %d\n",
                config->total_bananas, shared->total_bananas_in_maze, total_in_baskets, total_eaten, accounted);
        if (accounted == config->total_bananas) {
            fprintf(log, "  ✓ Balance correct!\n");
        } else {
            fprintf(log, "  Note: Difference of %d may be due to bananas in transit (carried by females)\n",
                    config->total_bananas - accounted);
        }
        
        fprintf(log, "\n================================================================================\n");
        if (winner >= 0) {
            fprintf(log, "WINNER: Family %d with %d bananas in basket!\n", winner, max_basket);
        } else {
            fprintf(log, "No clear winner\n");
        }
        fprintf(log, "================================================================================\n");
        
        fclose(log);
        printf("Detailed summary saved to: simulation_summary.txt\n\n");
    }
}

int main(int argc, char* argv[]) {
    SimConfig* config;
    const char* config_file = "simulation.conf";
    pthread_t monitor_tid, display_tid;
    int i;
    
    printf("\n=== APES COLLECTING BANANAS SIMULATION ===\n\n");
    
    /* Parse command line arguments */
    if (argc > 1) {
        config_file = argv[1];
    }
    
    /* Load configuration */
    config = load_config(config_file);
    if (config == NULL) {
        fprintf(stderr, "Failed to load configuration from: %s\n", config_file);
        return 1;
    }
    
    /* Initialize random seed */
    init_random();
    
    /* Set up signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Initialize shared memory */
    if (init_shared_data(config) != 0) {
        fprintf(stderr, "Failed to initialize shared data\n");
        free_config(config);
        return 1;
    }
    
    /* Initialize maze */
    init_maze(shared, config);
    
    /* Allocate child PID array */
    child_pids = (pid_t*)malloc(config->num_families * sizeof(pid_t));
    if (child_pids == NULL) {
        fprintf(stderr, "Failed to allocate memory for child PIDs\n");
        signal_handler(0);
        return 1;
    }
    memset(child_pids, 0, config->num_families * sizeof(pid_t));
    
    printf("Starting simulation: %d families, %d bananas, %dx%d maze\n", 
           config->num_families, config->total_bananas, config->maze_rows, config->maze_cols);
    printf("Females enter from bottom row (row %d), exit at row 0\n", config->maze_rows - 1);
    printf("Female collection goal: %d bananas before heading to exit\n", config->female_collection_goal);
    printf("Press Ctrl+C to stop\n\n");
    
    /* Show initial state (Time 0) */
    printf("================================================================================\n");
    printf("  TIME 0 - INITIAL STATE | Bananas in maze: %d | All families ready\n", config->total_bananas);
    printf("================================================================================\n");
    printf("Maze initialized. Females will enter from row %d (bottom border).\n", config->maze_rows - 1);
    printf("Exit is at row 0 (top). Females must reach row 0 to deposit bananas.\n\n");
    
    /* Brief pause then clear screen */
    sleep_ms(1500);
    clear_screen();
    
    /* NOW set start_time - this is when the simulation truly begins */
    shared->start_time = time(NULL);
    
    /* Fork family processes */
    for (i = 0; i < config->num_families; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            signal_handler(0);
            return 1;
        }
        
        if (pid == 0) {
            /* Child process - run family */
            free(child_pids);  /* Child doesn't need this */
            
            run_family_process(i, shared, config);
            
            /* Cleanup and exit child */
            detach_shared_memory(shared);
            free_config(config);
            exit(0);
        }
        
        /* Parent - save child PID */
        child_pids[i] = pid;
        num_children++;
    }
    
    /* Start monitor thread */
    if (pthread_create(&monitor_tid, NULL, monitor_thread, config) != 0) {
        perror("Failed to create monitor thread");
    }
    
    /* Start display thread */
    if (pthread_create(&display_tid, NULL, display_thread, config) != 0) {
        perror("Failed to create display thread");
    }
    
    /* Wait for all child processes to finish */
    for (i = 0; i < config->num_families; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
    }
    
    /* Stop threads */
    shared->simulation_running = 0;
    pthread_join(monitor_tid, NULL);
    pthread_join(display_tid, NULL);
    
    /* Print final results */
    print_final_results(config);
    
    /* Cleanup */
    cleanup_maze(shared);
    
    cleanup_simulation_semaphores(shared->num_families, shared->maze_rows, shared->maze_cols);
    
    /* Cleanup event lock */
    sem_destroy(&shared->event_lock);
    
    detach_shared_memory(shared);
    destroy_shared_memory(shm_id);
    
    free(child_pids);
    free_config(config);
    
    printf("Simulation complete!\n\n");
    
    return 0;
}
