#include "local.h"

/* ==================== Random Functions ==================== */

void init_random(void) {
    srand(time(NULL) ^ getpid());
}

int random_int(int min, int max) {
    if (min >= max) return min;
    return min + rand() % (max - min + 1);
}

float random_float(float min, float max) {
    if (min >= max) return min;
    return min + (float)rand() / RAND_MAX * (max - min);
}

int random_chance(float probability) {
    if (probability <= 0.0f) return 0;
    if (probability >= 1.0f) return 1;
    return (random_float(0.0f, 1.0f) < probability) ? 1 : 0;
}

/* ==================== Time Functions ==================== */

double get_elapsed_seconds(time_t start_time) {
    return difftime(time(NULL), start_time);
}

void sleep_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

void get_time_string(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%H:%M:%S", tm_info);
}

/* ==================== Logging Functions ==================== */

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_event(const char* format, ...) {
    char time_str[16];
    va_list args;
    
    pthread_mutex_lock(&log_mutex);
    
    get_time_string(time_str, sizeof(time_str));
    printf("[%s] ", time_str);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_family(int family_id, const char* format, ...) {
    char time_str[16];
    va_list args;
    
    pthread_mutex_lock(&log_mutex);
    
    get_time_string(time_str, sizeof(time_str));
    printf("[%s] [Family %d] ", time_str, family_id);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_female(int family_id, const char* format, ...) {
    char time_str[16];
    va_list args;
    
    pthread_mutex_lock(&log_mutex);
    
    get_time_string(time_str, sizeof(time_str));
    printf("[%s] [Family %d] [Female] ", time_str, family_id);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_male(int family_id, const char* format, ...) {
    char time_str[16];
    va_list args;
    
    pthread_mutex_lock(&log_mutex);
    
    get_time_string(time_str, sizeof(time_str));
    printf("[%s] [Family %d] [Male] ", time_str, family_id);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_baby(int family_id, int baby_id, const char* format, ...) {
    char time_str[16];
    va_list args;
    
    pthread_mutex_lock(&log_mutex);
    
    get_time_string(time_str, sizeof(time_str));
    printf("[%s] [Family %d] [Baby %d] ", time_str, family_id, baby_id);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}

/* ==================== Shared Memory Helpers ==================== */

int create_shared_memory(size_t size, int key) {
    int shm_id = shmget(key, size, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        return -1;
    }
    return shm_id;
}

void* attach_shared_memory(int shm_id) {
    void* ptr = shmat(shm_id, NULL, 0);
    if (ptr == (void*)-1) {
        perror("shmat failed");
        return NULL;
    }
    return ptr;
}

void detach_shared_memory(void* ptr) {
    if (ptr != NULL) {
        shmdt(ptr);
    }
}

void destroy_shared_memory(int shm_id) {
    shmctl(shm_id, IPC_RMID, NULL);
}

/* ==================== Console Helpers ==================== */

void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

void set_color(int color) {
    switch (color) {
        case COLOR_RED:     printf("\033[31m"); break;
        case COLOR_GREEN:   printf("\033[32m"); break;
        case COLOR_YELLOW:  printf("\033[33m"); break;
        case COLOR_BLUE:    printf("\033[34m"); break;
        case COLOR_MAGENTA: printf("\033[35m"); break;
        case COLOR_CYAN:    printf("\033[36m"); break;
        case COLOR_WHITE:   printf("\033[37m"); break;
        default:            printf("\033[0m");  break;
    }
}

void reset_color(void) {
    printf("\033[0m");
}

/* ==================== String Helpers ==================== */

char* trim(char* str) {
    char* end;
    
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    
    return str;
}

void safe_strcpy(char* dest, const char* src, size_t size) {
    if (size > 0) {
        strncpy(dest, src, size - 1);
        dest[size - 1] = '\0';
    }
}

/* ==================== Event Logging to File ==================== */

/*
 * Log event to file with timestamp
 * DISABLED - simulation_events.log is no longer created
 */
void log_event_to_file(const char* format, ...) {
    /* Logging to simulation_events.log disabled */
    (void)format;  /* Suppress unused parameter warning */
}

/* ==================== Shared Event Buffer ==================== */

/* Pointer to shared data for event logging */
static SharedData* g_shared_for_events = NULL;

void set_shared_for_events(SharedData* shared) {
    g_shared_for_events = shared;
}

void add_shared_event(SharedData* shared, const char* format, ...) {
    if (shared == NULL) return;
    
    sem_wait(&shared->event_lock);
    
    /* Get current timestamp */
    double elapsed = difftime(time(NULL), shared->start_time);
    
    /* Format the message */
    EventEntry* entry = &shared->recent_events[shared->event_head];
    entry->timestamp = elapsed;
    
    va_list args;
    va_start(args, format);
    vsnprintf(entry->message, MAX_EVENT_LEN - 1, format, args);
    va_end(args);
    entry->message[MAX_EVENT_LEN - 1] = '\0';
    
    /* Advance head (circular) */
    shared->event_head = (shared->event_head + 1) % MAX_EVENTS;
    
    sem_post(&shared->event_lock);
}
