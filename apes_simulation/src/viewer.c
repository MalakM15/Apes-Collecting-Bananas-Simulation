#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Platform-specific OpenGL headers */
#ifdef __APPLE__
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#endif

#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <time.h>
#include "../include/shared_data.h"

/* Window settings */
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 700
#define CELL_SIZE 30
#define MARGIN 50
#define STATUS_HEIGHT 150

/* Animation */
#define ANIMATION_INTERVAL 50  /* ms between frames */

/* Shared memory */
static SharedData* shared = NULL;
static int shm_id = -1;

/* Colors for families */
static float family_colors[][3] = {
    {1.0f, 0.3f, 0.3f},   /* Red */
    {0.3f, 1.0f, 0.3f},   /* Green */
    {0.3f, 0.5f, 1.0f},   /* Blue */
    {1.0f, 0.6f, 0.1f},   /* Orange */
    {0.8f, 0.3f, 1.0f},   /* Purple */
    {0.1f, 0.9f, 0.9f},   /* Cyan */
};
#define NUM_COLORS 6

/* Animation state */
static float time_offset = 0.0f;

/* Forward declarations */
void display(void);
void reshape(int w, int h);
void timer(int value);
void keyboard(unsigned char key, int x, int y);
void cleanup(void);

/* ==================== Drawing Helpers ==================== */

void draw_text(float x, float y, const char* text, void* font) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(font, *text);
        text++;
    }
}

void draw_rect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void draw_rect_outline(float x, float y, float w, float h) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void draw_circle(float cx, float cy, float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.14159f * i / segments;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

void draw_banana(float x, float y, float size) {
    /* Draw a simple banana shape */
    glColor3f(1.0f, 0.9f, 0.0f);  /* Yellow */
    
    /* Banana body (curved rectangle approximation) */
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 20; i++) {
        float t = (float)i / 20.0f;
        float angle = t * 3.14159f * 0.7f - 0.35f;
        float r = size * 0.4f;
        float cx = x + size * 0.3f;
        float cy = y + size * 0.5f;
        glVertex2f(cx + r * cosf(angle), cy + r * sinf(angle) * 0.5f);
    }
    glEnd();
    
    /* Stem */
    glColor3f(0.5f, 0.3f, 0.0f);  /* Brown */
    glBegin(GL_LINES);
    glVertex2f(x + size * 0.1f, y + size * 0.45f);
    glVertex2f(x + size * 0.25f, y + size * 0.55f);
    glEnd();
}

void draw_monkey(float x, float y, float size, int family_id, float anim_offset) {
    float* color = family_colors[family_id % NUM_COLORS];
    
    /* Body bounce animation */
    float bounce = sinf(anim_offset * 5.0f) * 2.0f;
    y += bounce;
    
    /* Body */
    glColor3f(color[0] * 0.7f, color[1] * 0.5f, color[2] * 0.3f);
    draw_circle(x + size * 0.5f, y + size * 0.4f, size * 0.35f, 16);
    
    /* Head */
    glColor3f(color[0] * 0.8f, color[1] * 0.6f, color[2] * 0.4f);
    draw_circle(x + size * 0.5f, y + size * 0.75f, size * 0.25f, 16);
    
    /* Face */
    glColor3f(0.9f, 0.8f, 0.7f);
    draw_circle(x + size * 0.5f, y + size * 0.7f, size * 0.15f, 12);
    
    /* Eyes */
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_circle(x + size * 0.42f, y + size * 0.78f, size * 0.05f, 8);
    draw_circle(x + size * 0.58f, y + size * 0.78f, size * 0.05f, 8);
    
    /* Ears */
    glColor3f(color[0] * 0.6f, color[1] * 0.4f, color[2] * 0.3f);
    draw_circle(x + size * 0.25f, y + size * 0.8f, size * 0.1f, 8);
    draw_circle(x + size * 0.75f, y + size * 0.8f, size * 0.1f, 8);
    
    /* Family number */
    glColor3f(1.0f, 1.0f, 1.0f);
    char num[4];
    snprintf(num, sizeof(num), "%d", family_id);
    draw_text(x + size * 0.45f, y + size * 0.35f, num, GLUT_BITMAP_HELVETICA_12);
}

void draw_obstacle(float x, float y, float size) {
    /* Rock/obstacle */
    glColor3f(0.4f, 0.4f, 0.45f);
    draw_rect(x + 2, y + 2, size - 4, size - 4);
    
    /* Highlight */
    glColor3f(0.5f, 0.5f, 0.55f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x + 2, y + size - 2);
    glVertex2f(x + 2, y + 2);
    glVertex2f(x + size - 2, y + 2);
    glEnd();
    
    /* Shadow */
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x + size - 2, y + 2);
    glVertex2f(x + size - 2, y + size - 2);
    glVertex2f(x + 2, y + size - 2);
    glEnd();
}

/* ==================== Main Drawing ==================== */

void draw_maze(void) {
    if (shared == NULL) return;
    
    int rows = shared->maze_rows;
    int cols = shared->maze_cols;
    
    float cell_w = (float)(WINDOW_WIDTH - 2 * MARGIN) / cols;
    float cell_h = (float)(WINDOW_HEIGHT - STATUS_HEIGHT - 2 * MARGIN) / rows;
    float cell_size = (cell_w < cell_h) ? cell_w : cell_h;
    
    float start_x = (WINDOW_WIDTH - cols * cell_size) / 2.0f;
    float start_y = MARGIN;
    
    /* Draw grid background */
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            float x = start_x + j * cell_size;
            float y = start_y + (rows - 1 - i) * cell_size;  /* Flip Y */
            
            const MazeCell* cell = &shared->maze[i][j];
            
            /* Cell background */
            if (i == 0) {
                /* Exit row - green tint */
                glColor3f(0.2f, 0.4f, 0.2f);
            } else {
                glColor3f(0.15f, 0.18f, 0.15f);
            }
            draw_rect(x, y, cell_size - 1, cell_size - 1);
            
            if (cell->is_obstacle) {
                draw_obstacle(x, y, cell_size);
            } else {
                /* Check for female ape */
                int female_here = -1;
                for (int k = 0; k < shared->num_families; k++) {
                    if (cell->females_in_cell[k]) {
                        female_here = k;
                        break;
                    }
                }
                
                if (female_here >= 0) {
                    /* Draw monkey */
                    draw_monkey(x, y, cell_size, female_here, time_offset + female_here * 0.5f);
                } else if (cell->bananas > 0) {
                    /* Draw bananas */
                    draw_banana(x + cell_size * 0.2f, y + cell_size * 0.2f, cell_size * 0.6f);
                    
                    /* Show count */
                    glColor3f(1.0f, 1.0f, 1.0f);
                    char count[4];
                    snprintf(count, sizeof(count), "%d", cell->bananas);
                    draw_text(x + cell_size * 0.7f, y + cell_size * 0.2f, count, 
                             GLUT_BITMAP_HELVETICA_10);
                }
            }
        }
    }
    
    /* Draw grid lines */
    glColor3f(0.3f, 0.35f, 0.3f);
    glLineWidth(1.0f);
    for (int i = 0; i <= rows; i++) {
        glBegin(GL_LINES);
        glVertex2f(start_x, start_y + i * cell_size);
        glVertex2f(start_x + cols * cell_size, start_y + i * cell_size);
        glEnd();
    }
    for (int j = 0; j <= cols; j++) {
        glBegin(GL_LINES);
        glVertex2f(start_x + j * cell_size, start_y);
        glVertex2f(start_x + j * cell_size, start_y + rows * cell_size);
        glEnd();
    }
    
    /* Exit label */
    glColor3f(0.5f, 1.0f, 0.5f);
    draw_text(start_x + cols * cell_size / 2 - 20, start_y + rows * cell_size + 5, 
              "EXIT", GLUT_BITMAP_HELVETICA_12);
}

void draw_status(void) {
    if (shared == NULL) return;
    
    float y = WINDOW_HEIGHT - STATUS_HEIGHT + 20;
    
    /* Title bar */
    glColor3f(0.1f, 0.15f, 0.2f);
    draw_rect(0, WINDOW_HEIGHT - STATUS_HEIGHT, WINDOW_WIDTH, STATUS_HEIGHT);
    
    /* Title */
    glColor3f(1.0f, 0.9f, 0.3f);
    draw_text(WINDOW_WIDTH / 2 - 120, y + 100, "APES COLLECTING BANANAS", 
              GLUT_BITMAP_HELVETICA_18);
    
    /* Simulation status */
    glColor3f(0.8f, 0.8f, 0.8f);
    char status[128];
    
    if (shared->simulation_running) {
        time_t elapsed = time(NULL) - shared->start_time;
        snprintf(status, sizeof(status), "Time: %lds  |  Bananas in maze: %d  |  Withdrawn: %d",
                 elapsed, shared->total_bananas_in_maze, shared->withdrawn_count);
    } else {
        const char* reason = "Unknown";
        switch (shared->termination_reason) {
            case TERM_WITHDRAWN_THRESHOLD: reason = "Too many withdrawals"; break;
            case TERM_BASKET_THRESHOLD: reason = "Basket threshold reached"; break;
            case TERM_BABY_ATE_THRESHOLD: reason = "Baby ate too much"; break;
            case TERM_TIMEOUT: reason = "Time limit reached"; break;
        }
        snprintf(status, sizeof(status), "SIMULATION ENDED: %s", reason);
        glColor3f(1.0f, 0.5f, 0.5f);
    }
    draw_text(20, y + 75, status, GLUT_BITMAP_HELVETICA_12);
    
    /* Family status boxes */
    float box_width = (WINDOW_WIDTH - 40) / shared->num_families;
    
    for (int i = 0; i < shared->num_families; i++) {
        FamilyStatus* f = &shared->families[i];
        float bx = 20 + i * box_width;
        float by = y;
        
        /* Box background */
        float* color = family_colors[i % NUM_COLORS];
        if (!f->is_active) {
            glColor3f(0.2f, 0.2f, 0.2f);
        } else if (f->male_fighting) {
            /* Flashing red when fighting */
            float flash = (sinf(time_offset * 10) + 1) / 2;
            glColor3f(0.5f + flash * 0.3f, 0.1f, 0.1f);
        } else {
            glColor3f(color[0] * 0.3f, color[1] * 0.3f, color[2] * 0.3f);
        }
        draw_rect(bx, by, box_width - 5, 60);
        
        /* Box border */
        glColor3f(color[0], color[1], color[2]);
        glLineWidth(2.0f);
        draw_rect_outline(bx, by, box_width - 5, 60);
        
        /* Family info */
        glColor3f(1.0f, 1.0f, 1.0f);
        char info[64];
        snprintf(info, sizeof(info), "Family %d", i);
        draw_text(bx + 5, by + 45, info, GLUT_BITMAP_HELVETICA_12);
        
        /* Basket with banana icon */
        glColor3f(1.0f, 0.9f, 0.0f);
        snprintf(info, sizeof(info), "Basket: %d", f->basket_bananas);
        draw_text(bx + 5, by + 28, info, GLUT_BITMAP_HELVETICA_10);
        
        /* Energy bars */
        glColor3f(0.5f, 0.5f, 0.5f);
        draw_rect(bx + 5, by + 5, box_width - 15, 8);
        draw_rect(bx + 5, by + 15, box_width - 15, 8);
        
        /* Male energy (blue) */
        float male_pct = f->male_energy / 100.0f;
        if (male_pct > 1.0f) male_pct = 1.0f;
        glColor3f(0.2f, 0.5f, 1.0f);
        draw_rect(bx + 5, by + 15, (box_width - 15) * male_pct, 8);
        
        /* Female energy (pink) */
        float female_pct = f->female_energy / 100.0f;
        if (female_pct > 1.0f) female_pct = 1.0f;
        glColor3f(1.0f, 0.4f, 0.6f);
        draw_rect(bx + 5, by + 5, (box_width - 15) * female_pct, 8);
        
        /* Status text */
        const char* state;
        if (!f->is_active) {
            state = "WITHDRAWN";
            glColor3f(0.5f, 0.5f, 0.5f);
        } else if (f->male_fighting) {
            state = "FIGHTING!";
            glColor3f(1.0f, 0.3f, 0.3f);
        } else if (f->female_in_maze) {
            state = "Collecting";
            glColor3f(0.3f, 1.0f, 0.3f);
        } else {
            state = "At basket";
            glColor3f(0.8f, 0.8f, 0.8f);
        }
        draw_text(bx + box_width - 70, by + 45, state, GLUT_BITMAP_HELVETICA_10);
    }
}

/* ==================== GLUT Callbacks ==================== */

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    /* Dark green jungle background */
    glClearColor(0.05f, 0.1f, 0.05f, 1.0f);
    
    if (shared == NULL) {
        glColor3f(1.0f, 0.5f, 0.5f);
        draw_text(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2, 
                  "Waiting for simulation...", GLUT_BITMAP_HELVETICA_18);
        draw_text(WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 - 30, 
                  "Run: ./apes_simulation", GLUT_BITMAP_HELVETICA_12);
    } else {
        draw_maze();
        draw_status();
    }
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void timer(int value) {
    (void)value;
    
    /* Try to connect to shared memory if not connected */
    if (shared == NULL) {
        shm_id = shmget(SHM_KEY, sizeof(SharedData), 0666);
        if (shm_id >= 0) {
            shared = (SharedData*)shmat(shm_id, NULL, 0);
            if (shared == (void*)-1) {
                shared = NULL;
            } else {
                printf("Connected to simulation shared memory\n");
            }
        }
    }
    
    /* Update animation */
    time_offset += 0.05f;
    
    glutPostRedisplay();
    glutTimerFunc(ANIMATION_INTERVAL, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    
    if (key == 27 || key == 'q') {  /* ESC or Q */
        cleanup();
        exit(0);
    }
}

void cleanup(void) {
    if (shared != NULL) {
        shmdt(shared);
        shared = NULL;
    }
    printf("Viewer closed\n");
}

/* ==================== Main ==================== */

int main(int argc, char** argv) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║        APES SIMULATION - OpenGL Viewer                         ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ This viewer connects to the running simulation                 ║\n");
    printf("║ Press 'Q' or ESC to quit                                       ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    /* Initialize GLUT */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Apes Collecting Bananas - Visualization");
    
    /* Set callbacks */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(ANIMATION_INTERVAL, timer, 0);
    
    /* Enable anti-aliasing */
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    /* Try initial connection */
    shm_id = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shm_id >= 0) {
        shared = (SharedData*)shmat(shm_id, NULL, 0);
        if (shared == (void*)-1) {
            shared = NULL;
            printf("Shared memory exists but couldn't attach. Waiting...\n");
        } else {
            printf("Connected to simulation!\n");
        }
    } else {
        printf("Simulation not running yet. Waiting for connection...\n");
    }
    
    /* Register cleanup */
    atexit(cleanup);
    
    /* Start main loop */
    glutMainLoop();
    
    return 0;
}

