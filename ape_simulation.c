#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <math.h>

// Configuration variables
int MAZE_SIZE, NUM_FEMALE_APES, NUM_MALE_APES, NUM_BABY_APES;
int BANANAS_TO_COLLECT, MAX_BANANAS_PER_CELL, OBSTACLE_PERCENTAGE, BANANA_PERCENTAGE;
int MAX_SIMULATION_TIME, WITHDRAWAL_THRESHOLD, FAMILY_BANANA_THRESHOLD, BABY_EATEN_THRESHOLD;
int FEMALE_REST_THRESHOLD, MALE_WITHDRAW_THRESHOLD, FEMALE_ENERGY_MAX, MALE_ENERGY_MAX;
int FEMALE_FIGHT_PROBABILITY, MALE_FIGHT_PROBABILITY, BABY_STEAL_PROBABILITY;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 750
#define MAZE_DISPLAY_SIZE 500
#define MAX_POPUPS 10

// Color definitions for better visualization
typedef struct { float r, g, b; } Color;
Color COLOR_MAZE_DARK = {0.08f, 0.08f, 0.12f};
Color COLOR_MAZE_LIGHT = {0.12f, 0.12f, 0.16f};
Color COLOR_OBSTACLE = {0.3f, 0.25f, 0.25f};
Color COLOR_BANANA = {1.0f, 0.85f, 0.2f};
Color COLOR_BANANA_DARK = {0.8f, 0.65f, 0.1f};
Color COLOR_TRAIL = {0.3f, 0.3f, 0.5f};
Color COLOR_STATS_BG = {0.05f, 0.05f, 0.08f};
Color COLOR_TEXT = {0.9f, 0.9f, 0.95f};
Color COLOR_TITLE = {1.0f, 0.7f, 0.2f};
Color COLOR_ACTIVE = {0.3f, 0.9f, 0.3f};
Color COLOR_INACTIVE = {0.7f, 0.2f, 0.2f};
Color COLOR_FIGHTING = {1.0f, 0.3f, 0.3f};
Color COLOR_RESTING = {1.0f, 0.8f, 0.3f};
Color COLOR_STEALING = {1.0f, 0.5f, 0.5f};
Color COLOR_RETURNING = {0.3f, 0.9f, 0.3f};
Color COLOR_SEARCHING = {0.7f, 0.7f, 1.0f};
Color COLOR_COLLECTING = {1.0f, 1.0f, 0.3f};

// Family colors - brighter and more distinct
Color FAMILY_COLORS[] = {
    {1.0f, 0.4f, 0.4f},    // Red
    {0.4f, 1.0f, 0.4f},    // Green
    {0.4f, 0.6f, 1.0f},    // Blue
    {1.0f, 1.0f, 0.4f},    // Yellow
    {1.0f, 0.4f, 1.0f},    // Magenta
    {0.4f, 1.0f, 1.0f},    // Cyan
    {0.8f, 0.6f, 0.2f},    // Orange
    {0.6f, 0.4f, 0.8f}     // Purple
};

// Popup structure for event feedback
typedef struct {
    char text[100];
    float x, y;
    int start_time;
    int duration;
    bool active;
} Popup;

typedef struct {
    int bananas;
    bool has_obstacle;
    int female_id;
    int trail_family;
    int trail_age;
    pthread_mutex_t cell_lock;
} MazeCell;

typedef struct { MazeCell** cells; int size; } Maze;
typedef struct { int x, y; } Position;
typedef struct { int bananas; pthread_mutex_t basket_lock; } Basket;

typedef struct {
    int id, family_id, energy, collected_bananas;
    bool active, resting, moving;
    Position pos;
    Position target_pos;          // For smooth animation
    Position display_pos;         // Current display position (float)
    Position family_base;         // Family's home base position
    float move_progress;          // 0.0 to 1.0
    pthread_t thread;
    int** visited_cells; // 2D array tracking visited cells
    int visit_threshold; // How many times visited before avoiding
    int steps_without_bananas;
    int consecutive_same_cells;
    Position last_positions[5];
    int last_pos_index;
    Position preferred_direction;  // ADD THIS LINE
} FemaleApe;

typedef struct {
    int id, family_id, energy;
    Basket* basket;
    bool active, fighting;
    Position family_base;         // Male also knows family base
    pthread_t thread;
} MaleApe;

typedef struct {
    int id, family_id, eaten_bananas;
    bool active, stealing;
    Position family_base;         // Baby knows family base
    pthread_t thread;
} BabyApe;

Maze maze;
FemaleApe* female_apes;
MaleApe* male_apes;
BabyApe* baby_apes;
Popup popups[MAX_POPUPS];
Position family_bases[10];  // Maximum 10 families
bool fight_effect_active = false;
Position fight_location;
int fight_effect_timer = 0;
int withdrawn_families = 0;
bool simulation_running = true;
time_t simulation_start_time;

pthread_mutex_t simulation_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_lock = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void set_color(Color c);
void set_color_bright(Color c, float brightness);
void set_family_color(int family_id, float brightness);
void add_popup(const char* text, float x, float y, int duration);
void draw_popups();
void read_config(const char* filename);
void init_maze();
void draw_text(float x, float y, const char* t);
void draw_text_large(float x, float y, const char* t);
void draw_text_title(float x, float y, const char* t);
void draw_rect(float x, float y, float w, float h);
void draw_rect_outlined(float x, float y, float w, float h, Color fill, Color outline);
void draw_circle(float x, float y, float r);
void draw_circle_outlined(float x, float y, float r, int family_id);
void draw_home_icon(float x, float y, float size);
void draw_search_icon(float x, float y, float size);
void draw_rest_icon(float x, float y, float size);
void draw_female_ape(FemaleApe* f, float base_x, float base_y, float cell_size);
void display();
void timer(int v);
void reshape(int w, int h);
bool check_simulation_end();
int get_neighbors(int x, int y, Position* neighbors);
Position find_start_position();
void move_female_to_position(FemaleApe* f, Position target);
void* female_ape_function(void* arg);
void* male_ape_function(void* arg);
void* baby_ape_function(void* arg);
void draw_flame_effect(float x, float y, float size, int timer);
void draw_explosion_effect(float x, float y, float size, int timer);
void initialize_family_bases();
void init_apes();
void start_simulation();
void cleanup();
void keyboard(unsigned char key, int x, int y);
bool are_families_neighbors(int family1, int family2, int max_distance);
int get_neighboring_families(int family_id, int* neighbor_list, int max_distance);
float calculate_territory_overlap(int family1, int family2);
int get_closest_family_to_position(Position pos);
bool is_position_in_family_territory(Position pos, int family_id, int territory_radius);
Position get_best_move_considering_territory(FemaleApe* f, Position* neighbors, int neighbor_count);

void set_color(Color c) {
    glColor3f(c.r, c.g, c.b);
}

void set_color_bright(Color c, float brightness) {
    glColor3f(c.r * brightness, c.g * brightness, c.b * brightness);
}

void set_family_color(int family_id, float brightness) {
    Color c = FAMILY_COLORS[family_id % 8];
    set_color_bright(c, brightness);
}

void add_popup(const char* text, float x, float y, int duration) {
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (!popups[i].active) {
            strncpy(popups[i].text, text, sizeof(popups[i].text) - 1);
            popups[i].text[sizeof(popups[i].text) - 1] = '\0';
            popups[i].x = x;
            popups[i].y = y;
            popups[i].start_time = glutGet(GLUT_ELAPSED_TIME);
            popups[i].duration = duration;
            popups[i].active = true;
            break;
        }
    }
}

void draw_popups() {
    int current_time = glutGet(GLUT_ELAPSED_TIME);
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (popups[i].active) {
            float elapsed = current_time - popups[i].start_time;
            float progress = elapsed / popups[i].duration;
            
            if (progress > 1.0f) {
                popups[i].active = false;
                continue;
            }
            
            float alpha = 1.0f - progress;
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            float y_offset = progress * 30.0f;
            
            glColor4f(0.1f, 0.1f, 0.2f, alpha * 0.7f);
            glBegin(GL_QUADS);
            glVertex2f(popups[i].x - 5, popups[i].y - y_offset - 8);
            glVertex2f(popups[i].x + strlen(popups[i].text) * 6 + 5, popups[i].y - y_offset - 8);
            glVertex2f(popups[i].x + strlen(popups[i].text) * 6 + 5, popups[i].y - y_offset + 12);
            glVertex2f(popups[i].x - 5, popups[i].y - y_offset + 12);
            glEnd();
            
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            glRasterPos2f(popups[i].x, popups[i].y - y_offset);
            for (int j = 0; popups[i].text[j] != '\0'; j++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, popups[i].text[j]);
            }
        }
    }
}

void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Config file not found. Using defaults\n");
        MAZE_SIZE = 12; NUM_FEMALE_APES = 3; NUM_MALE_APES = 3; NUM_BABY_APES = 3;
        BANANAS_TO_COLLECT = 5; MAX_BANANAS_PER_CELL = 5; OBSTACLE_PERCENTAGE = 15;
        BANANA_PERCENTAGE = 40; MAX_SIMULATION_TIME = 180; WITHDRAWAL_THRESHOLD = 2;
        FAMILY_BANANA_THRESHOLD = 40; BABY_EATEN_THRESHOLD = 15;
        FEMALE_REST_THRESHOLD = 40; MALE_WITHDRAW_THRESHOLD = 30;
        FEMALE_ENERGY_MAX = 150; MALE_ENERGY_MAX = 180;
        FEMALE_FIGHT_PROBABILITY = 20; MALE_FIGHT_PROBABILITY = 30; BABY_STEAL_PROBABILITY = 65;
        return;
    }
    
    char line[256], key[128]; int value;
    while (fgets(line, sizeof(line), file)) {
        if (line[0]=='#' || line[0]=='\n') continue;
        if (sscanf(line, "%[^=]=%d", key, &value)==2) {
            if (!strcmp(key,"MAZE_SIZE")) MAZE_SIZE=value;
            else if (!strcmp(key,"NUM_FEMALE_APES")) NUM_FEMALE_APES=value;
            else if (!strcmp(key,"NUM_MALE_APES")) NUM_MALE_APES=value;
            else if (!strcmp(key,"NUM_BABY_APES")) NUM_BABY_APES=value;
            else if (!strcmp(key,"BANANAS_TO_COLLECT")) BANANAS_TO_COLLECT=value;
            else if (!strcmp(key,"MAX_BANANAS_PER_CELL")) MAX_BANANAS_PER_CELL=value;
            else if (!strcmp(key,"OBSTACLE_PERCENTAGE")) OBSTACLE_PERCENTAGE=value;
            else if (!strcmp(key,"BANANA_PERCENTAGE")) BANANA_PERCENTAGE=value;
            else if (!strcmp(key,"MAX_SIMULATION_TIME")) MAX_SIMULATION_TIME=value;
            else if (!strcmp(key,"WITHDRAWAL_THRESHOLD")) WITHDRAWAL_THRESHOLD=value;
            else if (!strcmp(key,"FAMILY_BANANA_THRESHOLD")) FAMILY_BANANA_THRESHOLD=value;
            else if (!strcmp(key,"BABY_EATEN_THRESHOLD")) BABY_EATEN_THRESHOLD=value;
            else if (!strcmp(key,"FEMALE_REST_THRESHOLD")) FEMALE_REST_THRESHOLD=value;
            else if (!strcmp(key,"MALE_WITHDRAW_THRESHOLD")) MALE_WITHDRAW_THRESHOLD=value;
            else if (!strcmp(key,"FEMALE_ENERGY_MAX")) FEMALE_ENERGY_MAX=value;
            else if (!strcmp(key,"MALE_ENERGY_MAX")) MALE_ENERGY_MAX=value;
            else if (!strcmp(key,"FEMALE_FIGHT_PROBABILITY")) FEMALE_FIGHT_PROBABILITY=value;
            else if (!strcmp(key,"MALE_FIGHT_PROBABILITY")) MALE_FIGHT_PROBABILITY=value;
            else if (!strcmp(key,"BABY_STEAL_PROBABILITY")) BABY_STEAL_PROBABILITY=value;
        }
    }
    fclose(file);
}

void init_maze() {
    maze.size = MAZE_SIZE;
    maze.cells = malloc(MAZE_SIZE * sizeof(MazeCell*));
    for (int i = 0; i < MAZE_SIZE; i++) {
        maze.cells[i] = malloc(MAZE_SIZE * sizeof(MazeCell));
        for (int j = 0; j < MAZE_SIZE; j++) {
            maze.cells[i][j].bananas = 0;
            maze.cells[i][j].has_obstacle = false;
            maze.cells[i][j].female_id = -1;
            maze.cells[i][j].trail_family = -1;
            maze.cells[i][j].trail_age = 0;
            pthread_mutex_init(&maze.cells[i][j].cell_lock, NULL);
            
            if (rand() % 100 < OBSTACLE_PERCENTAGE) {
                maze.cells[i][j].has_obstacle = true;
            } else if (rand() % 100 < BANANA_PERCENTAGE) {
                maze.cells[i][j].bananas = (rand() % MAX_BANANAS_PER_CELL) + 1;
            }
        }
    }
}

void draw_text(float x, float y, const char* t) {
    glRasterPos2f(x, y);
    while (*t) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *t++);
}

void draw_text_large(float x, float y, const char* t) {
    glRasterPos2f(x, y);
    while (*t) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *t++);
}

void draw_text_title(float x, float y, const char* t) {
    glRasterPos2f(x, y);
    while (*t) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *t++);
}

void draw_rect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y); 
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

void draw_rect_outlined(float x, float y, float w, float h, Color fill, Color outline) {
    set_color(fill);
    draw_rect(x, y, w, h);
    set_color(outline);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y); 
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

void draw_circle(float x, float y, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 360; i += 10) {
        float a = i * 3.14159f / 180.0f;
        glVertex2f(x + cos(a) * r, y + sin(a) * r);
    }
    glEnd();
}

void draw_circle_outlined(float x, float y, float r, int family_id) {
    set_family_color(family_id, 1.0f);
    draw_circle(x, y, r);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i <= 360; i += 10) {
        float a = i * 3.14159f / 180.0f;
        glVertex2f(x + cos(a) * r, y + sin(a) * r);
    }
    glEnd();
}

void draw_home_icon(float x, float y, float size) {
    glColor3f(0, 1, 0);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y - size);
    glVertex2f(x - size, y + size/2);
    glVertex2f(x + size, y + size/2);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(x - size*0.7f, y + size/2);
    glVertex2f(x + size*0.7f, y + size/2);
    glVertex2f(x + size*0.7f, y + size);
    glVertex2f(x - size*0.7f, y + size);
    glEnd();
}

void draw_search_icon(float x, float y, float size) {
    glColor3f(1, 1, 0);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i += 30) {
        float a = i * 3.14159f / 180.0f;
        glVertex2f(x + cos(a) * size, y + sin(a) * size);
    }
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x + size*1.5f, y + size*1.5f);
    glEnd();
}

void draw_rest_icon(float x, float y, float size) {
    glColor3f(1, 0.8f, 0.3f);
    for (int z = 0; z < 3; z++) {
        glBegin(GL_QUADS);
        glVertex2f(x + z*3 - size/2, y);
        glVertex2f(x + z*3 + size/2, y);
        glVertex2f(x + z*3 + size/2, y + size);
        glVertex2f(x + z*3 - size/2, y + size);
        glEnd();
    }
}

void draw_flame_effect(float x, float y, float size, int timer) {
    float pulse = 0.8f + 0.2f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.01f);
    float alpha = 0.8f * (timer / 60.0f);
    
    glColor4f(1.0f, 0.5f, 0.1f, alpha);
    draw_circle(x, y, size * pulse);
    
    glColor4f(1.0f, 0.8f, 0.1f, alpha * 0.7f);
    draw_circle(x, y, size * 0.7f * pulse);
    
    glColor4f(1.0f, 1.0f, 0.8f, alpha * 0.6f);
    draw_circle(x, y, size * 0.4f * pulse);
    
    glColor4f(1.0f, 0.3f, 0.1f, alpha * 0.5f);
    for (int i = 0; i < 8; i++) {
        float angle = (glutGet(GLUT_ELAPSED_TIME) * 0.005f) + (i * 0.785f);
        float px = x + cos(angle) * size * 1.2f;
        float py = y + sin(angle) * size * 1.2f;
        draw_circle(px, py, size * 0.3f * pulse);
    }
}

void draw_explosion_effect(float x, float y, float size, int timer) {
    float progress = 1.0f - (timer / 30.0f);
    
    if (progress > 0) {
        glColor4f(1.0f, 0.8f, 0.1f, 0.6f * progress);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 360; i += 10) {
            float a = i * 3.14159f / 180.0f;
            glVertex2f(x + cos(a) * size * (1.0f + progress), 
                      y + sin(a) * size * (1.0f + progress));
        }
        glEnd();
        
        glColor4f(1.0f, 0.5f, 0.1f, 0.8f * progress);
        draw_circle(x, y, size * (0.5f + progress * 0.5f));
    }
}

// Track which areas have bananas
void update_banana_awareness(FemaleApe* f, Position current) {
    // Check surrounding 5x5 area for bananas
    int banana_directions[8] = {0};
    int max_bananas = 0;
    Position best_direction = {-1, -1};
    
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            int nx = current.x + dx;
            int ny = current.y + dy;
            
            if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
                if (maze.cells[nx][ny].bananas > 0) {
                    // Calculate general direction
                    int dir_x = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
                    int dir_y = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;
                    
                    // Update direction with most bananas
                    if (maze.cells[nx][ny].bananas > max_bananas) {
                        max_bananas = maze.cells[nx][ny].bananas;
                        best_direction.x = dir_x;
                        best_direction.y = dir_y;
                    }
                }
            }
        }
    }
    
    // Store best direction for future moves
    f->preferred_direction = best_direction;
}

void draw_female_ape(FemaleApe* f, float base_x, float base_y, float cell_size) {
    float display_x, display_y;
    
    if (f->moving) {
        float start_x = f->display_pos.x;
        float start_y = f->display_pos.y;
        float target_x = f->target_pos.x;
        float target_y = f->target_pos.y;
        
        display_x = start_x + (target_x - start_x) * f->move_progress;
        display_y = start_y + (target_y - start_y) * f->move_progress;
    } else {
        display_x = f->pos.x;
        display_y = f->pos.y;
    }
    
    float screen_x = base_x + display_y * cell_size + cell_size/2;
    float screen_y = base_y + display_x * cell_size + cell_size/2;
    
    if (f->resting) {
        float pulse = 0.7f + 0.3f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.005f);
        set_family_color(f->family_id, pulse);
    } else if (f->collected_bananas >= BANANAS_TO_COLLECT) {
        set_family_color(f->family_id, 1.3f);
    } else if (f->collected_bananas > 0) {
        float progress = (float)f->collected_bananas / BANANAS_TO_COLLECT;
        set_family_color(f->family_id, 0.8f + 0.5f * progress);
    } else {
        set_family_color(f->family_id, 1.0f);
    }
    
    float ape_size = cell_size * 0.35f;
    if (f->moving) {
        float squash = 0.9f + 0.1f * sin(f->move_progress * 3.14159f);
        draw_circle_outlined(screen_x, screen_y, ape_size * squash, f->family_id);
        
        if (f->move_progress > 0.1f && f->move_progress < 0.9f) {
            float dx = f->target_pos.y - f->display_pos.y;
            float dy = f->target_pos.x - f->display_pos.x;
            float angle = atan2(dy, dx);
            
            glColor3f(1, 1, 1);
            glBegin(GL_TRIANGLES);
            glVertex2f(screen_x + cos(angle) * ape_size*0.8f, 
                      screen_y + sin(angle) * ape_size*0.8f);
            glVertex2f(screen_x + cos(angle + 2.5) * ape_size*0.5f,
                      screen_y + sin(angle + 2.5) * ape_size*0.5f);
            glVertex2f(screen_x + cos(angle - 2.5) * ape_size*0.5f,
                      screen_y + sin(angle - 2.5) * ape_size*0.5f);
            glEnd();
        }
    } else {
        draw_circle_outlined(screen_x, screen_y, ape_size, f->family_id);
    }
    
    if (f->resting) {
        draw_rest_icon(screen_x, screen_y - ape_size - 10, 4);
    } else if (f->collected_bananas >= BANANAS_TO_COLLECT) {
        draw_home_icon(screen_x, screen_y - ape_size - 10, 6);
    } else if (f->collected_bananas > 0) {
        draw_search_icon(screen_x, screen_y - ape_size - 10, 4);
    }
    
    if (f->moving && f->target_pos.x >= 0 && f->move_progress < 0.9f) {
        float target_screen_x = base_x + f->target_pos.y * cell_size + cell_size/2;
        float target_screen_y = base_y + f->target_pos.x * cell_size + cell_size/2;
        
        glColor4f(1, 1, 1, 0.6f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(screen_x, screen_y);
        glVertex2f(target_screen_x, target_screen_y);
        glEnd();
        
        glColor4f(1, 1, 1, 0.3f);
        draw_rect(base_x + f->target_pos.y * cell_size + 2,
                  base_y + f->target_pos.x * cell_size + 2,
                  cell_size - 4, cell_size - 4);
    }
    
    glColor3f(1, 1, 1);
    char id_text[20];
    sprintf(id_text, "F%d", f->id);
    draw_text(screen_x - 6, screen_y + 5, id_text);
    
    if (f->collected_bananas > 0) {
        glColor3f(1, 0.9f, 0);
        char banana_text[10];
        sprintf(banana_text, "%d", f->collected_bananas);
        draw_text(screen_x - 4, screen_y - 15, banana_text);
    }
        float bar_width = cell_size * 0.6f;
    float bar_height = 3.0f;
    float bar_x = screen_x - bar_width/2;
    float bar_y = screen_y + ape_size + 5;
    
    // Background (empty bar)
    glColor3f(0.3f, 0.3f, 0.3f);
    draw_rect(bar_x, bar_y, bar_width, bar_height);
    
    // Foreground (energy level)
    float energy_ratio = (float)f->energy / FEMALE_ENERGY_MAX;
    if (energy_ratio > 0.6f) {
        glColor3f(0.2f, 1.0f, 0.2f); // Green
    } else if (energy_ratio > 0.3f) {
        glColor3f(1.0f, 0.8f, 0.2f); // Yellow
    } else {
        glColor3f(1.0f, 0.2f, 0.2f); // Red
    }
    draw_rect(bar_x, bar_y, bar_width * energy_ratio, bar_height);
    
    // Border
    glColor3f(0.8f, 0.8f, 0.8f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(bar_x, bar_y);
    glVertex2f(bar_x + bar_width, bar_y);
    glVertex2f(bar_x + bar_width, bar_y + bar_height);
    glVertex2f(bar_x, bar_y + bar_height);
    glEnd();
}

// SINGLE DEFINITION OF initialize_family_bases
void initialize_family_bases() {
    printf("\n=== INITIALIZING FAMILY BASES ===\n");
    
    int edges_tried[4] = {0};
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        Position base;
        bool found_valid = false;
        
        while (!found_valid) {
            int edge = rand() % 4;
            
            if (edges_tried[edge] > 1) {
                continue;
            }
            
            switch (edge) {
                case 0:
                    base.x = 0;
                    base.y = rand() % MAZE_SIZE;
                    break;
                case 1:
                    base.x = MAZE_SIZE - 1;
                    base.y = rand() % MAZE_SIZE;
                    break;
                case 2:
                    base.x = rand() % MAZE_SIZE;
                    base.y = 0;
                    break;
                case 3:
                    base.x = rand() % MAZE_SIZE;
                    base.y = MAZE_SIZE - 1;
                    break;
            }
            
            if (!maze.cells[base.x][base.y].has_obstacle) {
                bool too_close = false;
                for (int j = 0; j < i; j++) {
                    int dx = abs(base.x - family_bases[j].x);
                    int dy = abs(base.y - family_bases[j].y);
                    if (dx + dy < 3) {
                        too_close = true;
                        break;
                    }
                }
                
                if (!too_close) {
                    found_valid = true;
                    edges_tried[edge]++;
                    printf("Family %d placed on ", i);
                    switch (edge) {
                        case 0: printf("TOP"); break;
                        case 1: printf("BOTTOM"); break;
                        case 2: printf("LEFT"); break;
                        case 3: printf("RIGHT"); break;
                    }
                    printf(" edge at (%d, %d)\n", base.x, base.y);
                }
            }
        }
        
        family_bases[i] = base;
        maze.cells[base.x][base.y].trail_family = i;
        maze.cells[base.x][base.y].trail_age = 1000;
    }
    
    printf("\n=== NEIGHBORING FAMILIES ===\n");
    for (int i = 0; i < NUM_MALE_APES; i++) {
        int neighbors[10];
        int neighbor_count = 0;
        for (int j = 0; j < NUM_MALE_APES; j++) {
            if (j != i) {
                int dx = abs(family_bases[i].x - family_bases[j].x);
                int dy = abs(family_bases[i].y - family_bases[j].y);
                if (dx + dy <= 5) {
                    neighbors[neighbor_count++] = j;
                }
            }
        }
        
        if (neighbor_count > 0) {
            printf("Family %d neighbors: ", i);
            for (int j = 0; j < neighbor_count; j++) {
                printf("%d ", neighbors[j]);
            }
            printf("\n");
        } else {
            printf("Family %d has no close neighbors\n", i);
        }
    }
    printf("================================\n\n");
}

bool are_families_neighbors(int family1, int family2, int max_distance) {
    if (family1 == family2) return false;
    if (family1 < 0 || family1 >= NUM_MALE_APES) return false;
    if (family2 < 0 || family2 >= NUM_MALE_APES) return false;
    
    Position base1 = family_bases[family1];
    Position base2 = family_bases[family2];
    int distance = abs(base1.x - base2.x) + abs(base1.y - base2.y);
    
    return distance <= max_distance;
}

int get_neighboring_families(int family_id, int* neighbor_list, int max_distance) {
    int count = 0;
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        if (i != family_id) {
            if (are_families_neighbors(family_id, i, max_distance)) {
                if (count < 10) {
                    neighbor_list[count] = i;
                    count++;
                }
            }
        }
    }
    
    return count;
}

float calculate_territory_overlap(int family1, int family2) {
    Position base1 = family_bases[family1];
    Position base2 = family_bases[family2];
    
    int dx = abs(base1.x - base2.x);
    int dy = abs(base1.y - base2.y);
    float max_territory_radius = 5.0f;
    
    float distance = sqrt(dx*dx + dy*dy);
    if (distance >= 2 * max_territory_radius) {
        return 0.0f;
    }
    
    float overlap = 1.0f - (distance / (2 * max_territory_radius));
    return fmax(0.0f, fmin(1.0f, overlap));
}

int get_closest_family_to_position(Position pos) {
    int closest_family = -1;
    int min_distance = MAZE_SIZE * 2;
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        int dx = abs(pos.x - family_bases[i].x);
        int dy = abs(pos.y - family_bases[i].y);
        int distance = dx + dy;
        
        if (distance < min_distance) {
            min_distance = distance;
            closest_family = i;
        }
    }
    
    return closest_family;
}

bool is_position_in_family_territory(Position pos, int family_id, int territory_radius) {
    if (family_id < 0 || family_id >= NUM_MALE_APES) return false;
    
    Position base = family_bases[family_id];
    int dx = abs(pos.x - base.x);
    int dy = abs(pos.y - base.y);
    
    return (dx + dy) <= territory_radius;
}

Position get_best_move_considering_territory(FemaleApe* f, Position* neighbors, int neighbor_count) {
    Position best_move = neighbors[0];
    float best_score = -1000.0f;
    
    for (int i = 0; i < neighbor_count; i++) {
        float score = 0.0f;
        Position pos = neighbors[i];
        
        score += maze.cells[pos.x][pos.y].bananas * 10.0f;
        
        if (maze.cells[pos.x][pos.y].bananas > 0) {
            score += 5.0f;
        }
        
        if (f->collected_bananas >= BANANAS_TO_COLLECT) {
            int dx_to_base = abs(pos.x - f->family_base.x);
            int dy_to_base = abs(pos.y - f->family_base.y);
            int distance_to_base = dx_to_base + dy_to_base;
            score -= distance_to_base * 2.0f;
            
            for (int fam = 0; fam < NUM_MALE_APES; fam++) {
                if (fam != f->family_id) {
                    if (is_position_in_family_territory(pos, fam, 3)) {
                        score -= 15.0f;
                    }
                }
            }
        } else {
            bool in_enemy_territory = false;
            for (int fam = 0; fam < NUM_MALE_APES; fam++) {
                if (fam != f->family_id) {
                    if (is_position_in_family_territory(pos, fam, 2)) {
                        score -= 10.0f;
                        in_enemy_territory = true;
                    }
                }
            }
            
            if (!in_enemy_territory) {
                int current_distance = abs(f->pos.x - f->family_base.x) + abs(f->pos.y - f->family_base.y);
                int new_distance = abs(pos.x - f->family_base.x) + abs(pos.y - f->family_base.y);
                
                if (current_distance < 3) {
                    score += (new_distance - current_distance) * 0.5f;
                }
            }
        }
        
        if (maze.cells[pos.x][pos.y].has_obstacle) {
            score -= 100.0f;
        }
        
        if (score > best_score) {
            best_score = score;
            best_move = pos;
        }
    }
    
    return best_move;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    pthread_mutex_lock(&display_lock);
    
    float cell_size = (float)MAZE_DISPLAY_SIZE / MAZE_SIZE;
    float maze_x = 30, maze_y = 80;
    
    // ============================================
    // TITLE
    // ============================================
    set_color(COLOR_TITLE);
    draw_text_title(WINDOW_WIDTH/2 - 180, 30, "APES COLLECTING BANANAS SIMULATION");
    
    // ============================================
    // MAZE RENDERING
    // ============================================
    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            float x = maze_x + j * cell_size, y = maze_y + i * cell_size;
            
            // Checkerboard pattern
            if ((i + j) % 2 == 0) {
                set_color_bright(COLOR_MAZE_DARK, 1.0f);
            } else {
                set_color_bright(COLOR_MAZE_LIGHT, 1.0f);
            }
            draw_rect(x, y, cell_size, cell_size);
            
            // Trail effects
            if (maze.cells[i][j].trail_age > 0 && maze.cells[i][j].trail_family >= 0 && 
                maze.cells[i][j].trail_age < 1000) {
                float fade = maze.cells[i][j].trail_age / 20.0f;
                Color trail_color = FAMILY_COLORS[maze.cells[i][j].trail_family % 8];
                set_color_bright(trail_color, 0.2f * fade);
                draw_rect(x + 2, y + 2, cell_size - 4, cell_size - 4);
            }
            
            // Grid lines
            set_color((Color){0.2f, 0.2f, 0.25f});
            glLineWidth(1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x, y); glVertex2f(x + cell_size, y);
            glVertex2f(x + cell_size, y + cell_size); glVertex2f(x, y + cell_size);
            glEnd();
            
            // Obstacles
            if (maze.cells[i][j].has_obstacle) {
                set_color(COLOR_OBSTACLE);
                draw_rect(x + 2, y + 2, cell_size - 4, cell_size - 4);
            } 
            // Bananas
            else if (maze.cells[i][j].bananas > 0) {
                int banana_count = maze.cells[i][j].bananas;
                float radius = cell_size * 0.2f;
                
                set_color(COLOR_BANANA);
                draw_circle(x + cell_size/2, y + cell_size/2, radius);
                
                if (banana_count > 1) {
                    glColor3f(0, 0, 0);
                    char n[4];
                    sprintf(n, "%d", banana_count);
                    draw_text(x + cell_size/2 - 3, y + cell_size/2 + 3, n);
                }
            }
        }
    }
    
    // ============================================
    // FAMILY BASES (Home markers)
    // ============================================
    for (int i = 0; i < NUM_MALE_APES; i++) {
        float base_x = maze_x + family_bases[i].y * cell_size + cell_size/2;
        float base_y = maze_y + family_bases[i].x * cell_size + cell_size/2;
        
        // Base highlight circle
        Color family_color = FAMILY_COLORS[i % 8];
        set_color_bright(family_color, 0.3f);
        draw_circle(base_x, base_y, cell_size * 0.4f);
        
        // Base border
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        for (int a = 0; a <= 360; a += 20) {
            float angle = a * 3.14159f / 180.0f;
            glVertex2f(base_x + cos(angle) * cell_size * 0.4f, 
                      base_y + sin(angle) * cell_size * 0.4f);
        }
        glEnd();
        
        // Home icon
        glColor3f(0.2f, 0.8f, 0.2f);
        glBegin(GL_TRIANGLES);
        glVertex2f(base_x, base_y - cell_size*0.2f);
        glVertex2f(base_x - cell_size*0.15f, base_y);
        glVertex2f(base_x + cell_size*0.15f, base_y);
        glEnd();
        
        // Family label
        glColor3f(1, 1, 1);
        char base_text[10];
        sprintf(base_text, "F%d", i);
        draw_text(base_x - 6, base_y + 25, base_text);
    }
    
    // ============================================
    // FIGHT EFFECTS
    // ============================================
    if (fight_effect_active && fight_effect_timer > 0) {
        float fight_x = maze_x + fight_location.y * cell_size + cell_size/2;
        float fight_y = maze_y + fight_location.x * cell_size + cell_size/2;
        
        draw_flame_effect(fight_x, fight_y, cell_size * 0.8f, fight_effect_timer);
        
        if (fight_effect_timer % 15 < 5) {
            draw_explosion_effect(fight_x, fight_y, cell_size * 0.6f, fight_effect_timer % 15);
        }
        
        glColor4f(1.0f, 0.3f, 0.1f, 0.9f * (fight_effect_timer / 60.0f));
        draw_text_large(fight_x - 15, fight_y - cell_size*0.8f, "FIGHT!");
        
        fight_effect_timer--;
        if (fight_effect_timer <= 0) {
            fight_effect_active = false;
        }
    }
    
    // ============================================
    // FEMALE APES
    // ============================================
    for (int i = 0; i < NUM_FEMALE_APES; i++) {
        if (female_apes[i].active && female_apes[i].pos.x >= 0) {
            draw_female_ape(&female_apes[i], maze_x, maze_y, cell_size);
        }
    }
    
    // ============================================
    // STATISTICS PANEL (RIGHT SIDE) - PROPERLY SPACED
    // ============================================
    float stats_x = maze_x + MAZE_DISPLAY_SIZE + 20;
    float stats_y = maze_y;
    float panel_width = 300;
    float panel_height = WINDOW_HEIGHT - maze_y - 20;
    
    // Panel background
    set_color(COLOR_STATS_BG);
    draw_rect_outlined(stats_x - 10, stats_y - 10, panel_width, panel_height,
                      COLOR_STATS_BG, (Color){0.3f, 0.3f, 0.4f});
    
    // Header
    set_color((Color){0.6f, 1.0f, 1.0f});
    draw_text_large(stats_x + 10, stats_y, "STATISTICS");
    stats_y += 28;
    
    // Time and status
    int elapsed = (int)difftime(time(NULL), simulation_start_time);
    set_color(COLOR_TEXT);
    char buf[256];
    sprintf(buf, "Time: %d/%ds", elapsed, MAX_SIMULATION_TIME);
    draw_text(stats_x, stats_y, buf);
    stats_y += 20;
    
    if (simulation_running) {
        set_color((Color){0.3f, 1.0f, 0.3f});
        draw_text(stats_x, stats_y, "Status: RUNNING");
    } else {
        set_color((Color){1.0f, 0.3f, 0.3f});
        draw_text(stats_x, stats_y, "Status: ENDED");
    }
    stats_y += 20;
    
    set_color(COLOR_TEXT);
    sprintf(buf, "Withdrawn: %d/%d", withdrawn_families, WITHDRAWAL_THRESHOLD);
    draw_text(stats_x, stats_y, buf);
    stats_y += 30;
    
    // ============================================
    // FAMILIES SECTION (PROPERLY SPACED)
    // ============================================
    set_color((Color){0.4f, 1.0f, 0.4f});
    draw_text_large(stats_x + 20, stats_y, "FAMILIES");
    stats_y += 25;
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        // Check if we have space (leave room for babies section)
        if (stats_y > maze_y + panel_height - 150) {
            set_color((Color){1.0f, 0.7f, 0.3f});
            draw_text(stats_x, stats_y, "...");
            break;
        }
        
        // Family color indicator
        Color family_color = FAMILY_COLORS[i % 8];
        set_color(family_color);
        draw_rect(stats_x - 5, stats_y - 10, 12, 12);
        
        // Family status color
        if (male_apes[i].active) {
            set_color(COLOR_ACTIVE);
        } else {
            set_color(COLOR_INACTIVE);
        }
        
        // Family header
        sprintf(buf, "F%d @(%d,%d)", i, family_bases[i].x, family_bases[i].y);
        draw_text(stats_x + 12, stats_y, buf);
        stats_y += 18;
        
        // Male stats
        set_color(COLOR_TEXT);
        sprintf(buf, " M%d: B:%d E:%d", 
                i,
                male_apes[i].basket->bananas, 
                male_apes[i].energy);
        draw_text(stats_x + 5, stats_y, buf);
        stats_y += 16;
        
        // Male energy bar
        float male_bar_x = stats_x + 5;
        float male_bar_y = stats_y;
        float male_bar_w = 100;
        float male_bar_h = 5;
        
        glColor3f(0.2f, 0.2f, 0.2f);
        draw_rect(male_bar_x, male_bar_y, male_bar_w, male_bar_h);
        
        float male_energy_ratio = (float)male_apes[i].energy / MALE_ENERGY_MAX;
        if (male_energy_ratio > 0.5f) {
            glColor3f(0.2f, 0.8f, 0.2f);
        } else if (male_energy_ratio > 0.25f) {
            glColor3f(0.9f, 0.7f, 0.2f);
        } else {
            glColor3f(0.9f, 0.2f, 0.2f);
        }
        draw_rect(male_bar_x, male_bar_y, male_bar_w * male_energy_ratio, male_bar_h);
        stats_y += 12;
        
        // Female stats - find female for this family
        bool found_female = false;
        for (int f = 0; f < NUM_FEMALE_APES; f++) {
            if (female_apes[f].family_id == i && female_apes[f].active) {
                found_female = true;
                
                char state_char = 'S';
                Color state_color = COLOR_SEARCHING;
                
                if (female_apes[f].resting) {
                    state_char = 'R';
                    state_color = COLOR_RESTING;
                } else if (female_apes[f].collected_bananas >= BANANAS_TO_COLLECT) {
                    state_char = 'H';
                    state_color = COLOR_RETURNING;
                } else if (female_apes[f].collected_bananas > 0) {
                    state_char = 'C';
                    state_color = COLOR_COLLECTING;
                }
                
                set_color(state_color);
                sprintf(buf, " F%d:%c B:%d E:%d", 
                        f, state_char,
                        female_apes[f].collected_bananas,
                        female_apes[f].energy);
                draw_text(stats_x + 5, stats_y, buf);
                stats_y += 16;
                
                // Female energy bar
                float female_bar_x = stats_x + 5;
                float female_bar_y = stats_y;
                float female_bar_w = 100;
                float female_bar_h = 5;
                
                glColor3f(0.2f, 0.2f, 0.2f);
                draw_rect(female_bar_x, female_bar_y, female_bar_w, female_bar_h);
                
                float female_energy_ratio = (float)female_apes[f].energy / FEMALE_ENERGY_MAX;
                if (female_energy_ratio > 0.5f) {
                    glColor3f(0.2f, 0.8f, 0.2f);
                } else if (female_energy_ratio > 0.25f) {
                    glColor3f(0.9f, 0.7f, 0.2f);
                } else {
                    glColor3f(0.9f, 0.2f, 0.2f);
                }
                draw_rect(female_bar_x, female_bar_y, female_bar_w * female_energy_ratio, female_bar_h);
                stats_y += 12;
                
                break; // Only show one female per family
            }
        }
        
        // If no female was found but there should be one
        if (!found_female) {
            stats_y += 0; // No extra space needed
        }
        
        // Fighting indicator
        if (male_apes[i].fighting) {
            set_color(COLOR_FIGHTING);
            draw_text(stats_x + 5, stats_y, " [FIGHTING!]");
            stats_y += 16;
        }
        
        stats_y += 10; // Gap between families (increased from 8)
    }
    
    // ============================================
    // BABIES SECTION
    // ============================================
    stats_y += 15;
    
    if (stats_y <= maze_y + panel_height - 100) {
        set_color((Color){1.0f, 0.8f, 0.4f});
        draw_text_large(stats_x + 30, stats_y, "BABIES");
        stats_y += 22;
        
        for (int i = 0; i < NUM_BABY_APES; i++) {
            if (!baby_apes[i].active) continue;
            
            if (stats_y > maze_y + panel_height - 50) {
                set_color((Color){1.0f, 0.7f, 0.3f});
                draw_text(stats_x, stats_y, "...");
                break;
            }
            
            // Baby color indicator
            Color family_color = FAMILY_COLORS[baby_apes[i].family_id % 8];
            set_color_bright(family_color, 0.8f);
            draw_rect(stats_x - 5, stats_y - 10, 10, 10);
            
            // Baby status
            if (baby_apes[i].stealing) {
                set_color(COLOR_STEALING);
            } else {
                set_color(COLOR_TEXT);
            }
            
            sprintf(buf, "B%d(F%d): %d/%d", 
                    i, baby_apes[i].family_id,
                    baby_apes[i].eaten_bananas,
                    BABY_EATEN_THRESHOLD);
            draw_text(stats_x + 8, stats_y, buf);
            stats_y += 18; // Increased spacing
        }
    }
    
    // ============================================
    // LEGEND & CONTROLS
    // ============================================
    stats_y += 18;
    if (stats_y <= maze_y + panel_height - 70) {
        set_color((Color){0.5f, 0.5f, 0.6f});
        draw_text(stats_x, stats_y, "Legend:");
        stats_y += 16;
        
        set_color((Color){0.6f, 0.6f, 0.7f});
        draw_text(stats_x, stats_y, "S=Search C=Collect");
        stats_y += 14;
        draw_text(stats_x, stats_y, "H=Home R=Rest");
        stats_y += 14;
        draw_text(stats_x, stats_y, "M=Male F=Female");
        stats_y += 14;
        draw_text(stats_x, stats_y, "B=Bananas E=Energy");
        stats_y += 20;
        
        set_color((Color){0.7f, 0.7f, 1.0f});
        draw_text(stats_x, stats_y, "Press Q or ESC to quit");
    }
    
    // ============================================
    // POPUPS
    // ============================================
    draw_popups();
    
    pthread_mutex_unlock(&display_lock);
    glutSwapBuffers();
}

void timer(int v) {
    if (simulation_running || v < 100) {
        pthread_mutex_lock(&display_lock);
        
        for (int i = 0; i < MAZE_SIZE; i++) {
            for (int j = 0; j < MAZE_SIZE; j++) {
                if (maze.cells[i][j].trail_age > 0 && maze.cells[i][j].trail_family >= 0) {
                    if (maze.cells[i][j].trail_age < 1000) {
                        maze.cells[i][j].trail_age--;
                    }
                    if (maze.cells[i][j].trail_age == 0) {
                        maze.cells[i][j].trail_family = -1;
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&display_lock);
        glutPostRedisplay();
        glutTimerFunc(50, timer, v + 1);
    }
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

bool check_simulation_end() {
    pthread_mutex_lock(&simulation_lock);
    
    if (difftime(time(NULL), simulation_start_time) > MAX_SIMULATION_TIME) {
        if (simulation_running) {
            pthread_mutex_lock(&print_lock);
            printf("\n*** SIMULATION END: Time limit reached (%d seconds) ***\n", MAX_SIMULATION_TIME);
            pthread_mutex_unlock(&print_lock);
            simulation_running = false;
        }
        pthread_mutex_unlock(&simulation_lock);
        return true;
    }
    
    if (withdrawn_families >= WITHDRAWAL_THRESHOLD) {
        if (simulation_running) {
            pthread_mutex_lock(&print_lock);
            printf("\n*** SIMULATION END: %d families withdrawn (threshold: %d) ***\n", 
                   withdrawn_families, WITHDRAWAL_THRESHOLD);
            pthread_mutex_unlock(&print_lock);
            simulation_running = false;
        }
        pthread_mutex_unlock(&simulation_lock);
        return true;
    }
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        if (male_apes[i].active && male_apes[i].basket->bananas >= FAMILY_BANANA_THRESHOLD) {
            if (simulation_running) {
                pthread_mutex_lock(&print_lock);
                printf("\n*** SIMULATION END: Family %d collected %d bananas (threshold: %d) ***\n", 
                       i, male_apes[i].basket->bananas, FAMILY_BANANA_THRESHOLD);
                pthread_mutex_unlock(&print_lock);
                simulation_running = false;
            }
            pthread_mutex_unlock(&simulation_lock);
            return true;
        }
    }
    
    for (int i = 0; i < NUM_BABY_APES; i++) {
        if (baby_apes[i].active && baby_apes[i].eaten_bananas >= BABY_EATEN_THRESHOLD) {
            if (simulation_running) {
                pthread_mutex_lock(&print_lock);
                printf("\n*** SIMULATION END: Baby %d ate %d bananas (threshold: %d) ***\n", 
                       i, baby_apes[i].eaten_bananas, BABY_EATEN_THRESHOLD);
                pthread_mutex_unlock(&print_lock);
                simulation_running = false;
            }
            pthread_mutex_unlock(&simulation_lock);
            return true;
        }
    }
    
    pthread_mutex_unlock(&simulation_lock);
    return false;
}

int get_neighbors(int x, int y, Position* neighbors) {
    int count = 0;
    int dx[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
    
    for (int i = 0; i < 8; i++) {
        int nx = x + dx[i], ny = y + dy[i];
        if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE && 
            !maze.cells[nx][ny].has_obstacle) {
            neighbors[count].x = nx;
            neighbors[count].y = ny;
            count++;
        }
    }
    return count;
}

Position find_start_position() {
    Position pos;
    do {
        pos.x = rand() % MAZE_SIZE;
        pos.y = rand() % MAZE_SIZE;
    } while (maze.cells[pos.x][pos.y].has_obstacle);
    return pos;
}

void move_female_to_position(FemaleApe* f, Position target) {
    pthread_mutex_lock(&display_lock);
    f->moving = true;
    f->target_pos = target;
    f->move_progress = 0.0f;
    f->display_pos = f->pos;
    pthread_mutex_unlock(&display_lock);
    
    int animation_steps = 10;
    for (int step = 0; step <= animation_steps; step++) {
        if (!simulation_running || !f->active) break;
        
        pthread_mutex_lock(&display_lock);
        f->move_progress = (float)step / animation_steps;
        pthread_mutex_unlock(&display_lock);
        
        usleep(80000);
    }
    
    pthread_mutex_lock(&display_lock);
    f->pos = target;
    f->display_pos = target;
    f->moving = false;
    f->move_progress = 0.0f;
    
    if (f->pos.x >= 0 && f->pos.y >= 0) {
        maze.cells[f->pos.x][f->pos.y].female_id = f->id;
    }
    pthread_mutex_unlock(&display_lock);
}

void* female_ape_function(void* arg) {
    FemaleApe* f = (FemaleApe*)arg;
    Position neighbors[8];
    int steps_in_maze, max_steps = MAZE_SIZE * MAZE_SIZE * 3; // Increased max steps
    float maze_x = 30, maze_y = 80, cell_size = (float)MAZE_DISPLAY_SIZE/MAZE_SIZE;
    
    // Initialize memory arrays if not already initialized
    if (f->visited_cells == NULL) {
        f->visited_cells = malloc(MAZE_SIZE * sizeof(int*));
        for (int x = 0; x < MAZE_SIZE; x++) {
            f->visited_cells[x] = malloc(MAZE_SIZE * sizeof(int));
            for (int y = 0; y < MAZE_SIZE; y++) {
                f->visited_cells[x][y] = 0;
            }
        }
    }
    
    // Initialize stuck detection arrays
    int* last_positions_x = malloc(10 * sizeof(int));
    int* last_positions_y = malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        last_positions_x[i] = -1;
        last_positions_y[i] = -1;
    }
    int last_pos_index = 0;
    int steps_without_bananas = 0;
    int consecutive_same_cells = 0;
    Position last_cell = {-1, -1};
    
    // Add preferred direction for movement
    Position preferred_direction = {0, 0};
    int direction_persistence = 0;
    
    while (simulation_running && f->active) {
        // Check if family is still active
        if (!male_apes[f->family_id].active) {
            f->active = false;
            pthread_mutex_lock(&print_lock);
            printf("Female %d stopping - Family %d withdrew\n", f->id, f->family_id);
            pthread_mutex_unlock(&print_lock);
            break;
        }
        
        // Rest if energy is low - now at family base
        if (f->energy < FEMALE_REST_THRESHOLD) {
            pthread_mutex_lock(&display_lock);
            f->resting = true;
            f->pos = f->family_base;  // Return to base to rest
            f->display_pos = f->family_base;
            pthread_mutex_unlock(&display_lock);
            
            pthread_mutex_lock(&print_lock);
            printf("Female %d (Family %d) resting at base (%d, %d)... Energy: %d\n", 
                   f->id, f->family_id, f->family_base.x, f->family_base.y, f->energy);
            pthread_mutex_unlock(&print_lock);
            
            // Add popup for resting
            float screen_x = maze_x + f->family_base.y * cell_size + cell_size/2;
            float screen_y = maze_y + f->family_base.x * cell_size + cell_size/2;
            add_popup("RESTING AT BASE", screen_x - 30, screen_y - 30, 2000);
            
            // Reset exploration memory when resting
            for (int x = 0; x < MAZE_SIZE; x++) {
                for (int y = 0; y < MAZE_SIZE; y++) {
                    f->visited_cells[x][y] = 0;
                }
            }
            
            // Reset stuck detection
            for (int i = 0; i < 10; i++) {
                last_positions_x[i] = -1;
                last_positions_y[i] = -1;
            }
            last_pos_index = 0;
            steps_without_bananas = 0;
            consecutive_same_cells = 0;
            direction_persistence = 0;
            
            sleep(3); // Rest for 3 seconds
            f->energy += 40;
            if (f->energy > FEMALE_ENERGY_MAX) f->energy = FEMALE_ENERGY_MAX;
            
            pthread_mutex_lock(&display_lock);
            f->resting = false;
            pthread_mutex_unlock(&display_lock);
            continue;
        }
        
        // Start from family base
        Position current = f->family_base;
        pthread_mutex_lock(&display_lock);
        f->pos = current;
        f->display_pos = current;
        f->collected_bananas = 0;
        pthread_mutex_unlock(&display_lock);
        
        int collected = 0;
        steps_in_maze = 0;
        
        pthread_mutex_lock(&print_lock);
        printf("Female %d (Family %d) starting from base (%d, %d) | Energy: %d\n", 
               f->id, f->family_id, current.x, current.y, f->energy);
        pthread_mutex_unlock(&print_lock);
        
        // Calculate screen coordinates for popups
        float screen_x = maze_x + current.y * cell_size + cell_size/2;
        float screen_y = maze_y + current.x * cell_size + cell_size/2;
        
        // Collect bananas
        while (simulation_running && f->active && collected < BANANAS_TO_COLLECT && steps_in_maze < max_steps) {
            // Update position history for oscillation detection
            last_positions_x[last_pos_index] = current.x;
            last_positions_y[last_pos_index] = current.y;
            last_pos_index = (last_pos_index + 1) % 10;
            
            // Check for stuck in same cell
            if (current.x == last_cell.x && current.y == last_cell.y) {
                consecutive_same_cells++;
            } else {
                consecutive_same_cells = 0;
                last_cell = current;
            }
            
            // Check energy before continuing - with early return logic
            if (f->energy <= 10) {
                // Critically low energy - force return
                pthread_mutex_lock(&print_lock);
                printf("Female %d CRITICALLY low energy (%d)! Forcing return to base.\n", f->id, f->energy);
                pthread_mutex_unlock(&print_lock);
                
                // Clear current cell
                pthread_mutex_lock(&display_lock);
                if (current.x >= 0 && current.y >= 0) {
                    maze.cells[current.x][current.y].female_id = -1;
                }
                pthread_mutex_unlock(&display_lock);
                
                // Return to base regardless of collection
                move_female_to_position(f, f->family_base);
                current = f->family_base;
                break;
            }
            
            // Energy-aware decision making
            if (f->energy < FEMALE_REST_THRESHOLD * 1.5) { // Getting low
                if (collected > 0) {
                    // Have some bananas, better return with them
                    pthread_mutex_lock(&print_lock);
                    printf("Female %d low energy (%d) but has %d bananas - returning early\n", 
                           f->id, f->energy, collected);
                    pthread_mutex_unlock(&print_lock);
                    
                    pthread_mutex_lock(&display_lock);
                    if (current.x >= 0 && current.y >= 0) {
                        maze.cells[current.x][current.y].female_id = -1;
                    }
                    pthread_mutex_unlock(&display_lock);
                    
                    move_female_to_position(f, f->family_base);
                    current = f->family_base;
                    break;
                } else if (steps_in_maze > max_steps / 4) {
                    // Been searching too long with low energy and no bananas
                    pthread_mutex_lock(&print_lock);
                    printf("Female %d low energy, no bananas after %d steps - returning empty\n", 
                           f->id, steps_in_maze);
                    pthread_mutex_unlock(&print_lock);
                    
                    pthread_mutex_lock(&display_lock);
                    if (current.x >= 0 && current.y >= 0) {
                        maze.cells[current.x][current.y].female_id = -1;
                    }
                    pthread_mutex_unlock(&display_lock);
                    
                    move_female_to_position(f, f->family_base);
                    current = f->family_base;
                    break;
                }
            }
            
            // Check for bananas in current cell
            if (maze.cells[current.x][current.y].bananas > 0) {
                pthread_mutex_lock(&maze.cells[current.x][current.y].cell_lock);
                if (maze.cells[current.x][current.y].bananas > 0) {
                    int take = fmin(maze.cells[current.x][current.y].bananas, 
                                   BANANAS_TO_COLLECT - collected);
                    maze.cells[current.x][current.y].bananas -= take;
                    collected += take;
                    f->collected_bananas = collected;
                    
                    // Reset stuck counters when finding bananas
                    steps_without_bananas = 0;
                    consecutive_same_cells = 0;
                    direction_persistence = 0; // Reset direction persistence
                    
                    // Energy cost for collecting (more for larger collection)
                    int energy_cost = 2 + (take / 2);
                    f->energy -= energy_cost;
                    if (f->energy < 0) f->energy = 0;
                    
                    pthread_mutex_lock(&print_lock);
                    printf("  Female %d collected %d bananas at (%d, %d)! Total: %d, Energy: %d (-%d)\n", 
                           f->id, take, current.x, current.y, collected, f->energy, energy_cost);
                    pthread_mutex_unlock(&print_lock);
                    
                    // Update screen coordinates for current position
                    screen_x = maze_x + current.y * cell_size + cell_size/2;
                    screen_y = maze_y + current.x * cell_size + cell_size/2;
                    char popup_text[50];
                    sprintf(popup_text, "+%d BANANAS!", take);
                    add_popup(popup_text, screen_x - 20, screen_y - 30, 1500);
                    
                    // Mark this cell as highly visited (good spot)
                    f->visited_cells[current.x][current.y] += 2;
                }
                pthread_mutex_unlock(&maze.cells[current.x][current.y].cell_lock);
            } else {
                // No bananas in this cell
                steps_without_bananas++;
                f->visited_cells[current.x][current.y]++; // Mark as visited
            }
            
            if (collected >= BANANAS_TO_COLLECT) {
                pthread_mutex_lock(&print_lock);
                printf("Female %d collected enough! Returning to base.\n", f->id);
                pthread_mutex_unlock(&print_lock);
                
                // Clear path memory for next trip
                for (int x = 0; x < MAZE_SIZE; x++) {
                    for (int y = 0; y < MAZE_SIZE; y++) {
                        if (f->visited_cells[x][y] > 0) {
                            f->visited_cells[x][y]--; // Fade memory
                        }
                    }
                }
                break;
            }
            
            // ============================================
            // ADVANCED STUCK DETECTION AND RECOVERY LOGIC
            // ============================================
            
            bool force_teleport = false;
            bool force_direction_change = false;
            
            // Detection 1: Too many steps without bananas
            if (steps_without_bananas > 25) {
                pthread_mutex_lock(&print_lock);
                printf("Female %d stuck detection: %d steps without bananas\n", 
                       f->id, steps_without_bananas);
                pthread_mutex_unlock(&print_lock);
                force_teleport = true;
            }
            
            // Detection 2: Oscillation (going back and forth)
            int oscillation_score = 0;
            for (int i = 0; i < 9; i++) {
                for (int j = i + 1; j < 10; j++) {
                    if (last_positions_x[i] == last_positions_x[j] && 
                        last_positions_y[i] == last_positions_y[j] &&
                        last_positions_x[i] != -1) {
                        oscillation_score++;
                    }
                }
            }
            
            if (oscillation_score > 12) { // High oscillation detected
                pthread_mutex_lock(&print_lock);
                printf("Female %d stuck detection: High oscillation (score: %d)\n", 
                       f->id, oscillation_score);
                pthread_mutex_unlock(&print_lock);
                force_direction_change = true;
                if (oscillation_score > 15) force_teleport = true;
            }
            
            // Detection 3: Stuck in same cell
            if (consecutive_same_cells > 3) {
                pthread_mutex_lock(&print_lock);
                printf("Female %d stuck detection: Stuck in cell (%d,%d) for %d moves\n", 
                       f->id, current.x, current.y, consecutive_same_cells);
                pthread_mutex_unlock(&print_lock);
                force_teleport = true;
            }
            
            // Detection 4: Too many visits to same area
            int local_visit_count = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int nx = current.x + dx;
                    int ny = current.y + dy;
                    if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
                        local_visit_count += f->visited_cells[nx][ny];
                    }
                }
            }
            
            if (local_visit_count > 30) {
                pthread_mutex_lock(&print_lock);
                printf("Female %d stuck detection: Overvisited local area (count: %d)\n", 
                       f->id, local_visit_count);
                pthread_mutex_unlock(&print_lock);
                force_direction_change = true;
            }
            
            // ============================================
            // RECOVERY ACTIONS
            // ============================================
            
            if (force_teleport) {
                // RADICAL RECOVERY: Teleport to new area
                pthread_mutex_lock(&print_lock);
                printf("  -> Female %d initiating emergency teleport!\n", f->id);
                pthread_mutex_unlock(&print_lock);
                
                // Find a cell with bananas if possible
                Position teleport_target = {-1, -1};
                bool found_banana_cell = false;
                
                // First try to find a cell with bananas
                for (int attempt = 0; attempt < 20 && !found_banana_cell; attempt++) {
                    int tx = rand() % MAZE_SIZE;
                    int ty = rand() % MAZE_SIZE;
                    if (!maze.cells[tx][ty].has_obstacle && maze.cells[tx][ty].bananas > 0) {
                        teleport_target.x = tx;
                        teleport_target.y = ty;
                        found_banana_cell = true;
                        pthread_mutex_lock(&print_lock);
                        printf("    Found banana cell at (%d,%d) for teleport\n", tx, ty);
                        pthread_mutex_unlock(&print_lock);
                    }
                }
                
                // If no banana cell found, find any open cell far from current
                if (!found_banana_cell) {
                    int max_distance = 0;
                    for (int attempt = 0; attempt < 30; attempt++) {
                        int tx = rand() % MAZE_SIZE;
                        int ty = rand() % MAZE_SIZE;
                        if (!maze.cells[tx][ty].has_obstacle) {
                            int distance = abs(tx - current.x) + abs(ty - current.y);
                            if (distance > max_distance) {
                                max_distance = distance;
                                teleport_target.x = tx;
                                teleport_target.y = ty;
                            }
                        }
                    }
                }
                
                if (teleport_target.x != -1 && teleport_target.y != -1) {
                    // Clear current cell
                    pthread_mutex_lock(&display_lock);
                    if (current.x >= 0 && current.y >= 0) {
                        maze.cells[current.x][current.y].female_id = -1;
                    }
                    
                    // Perform teleport
                    f->pos = teleport_target;
                    f->display_pos = teleport_target;
                    current = teleport_target;
                    maze.cells[current.x][current.y].female_id = f->id;
                    pthread_mutex_unlock(&display_lock);
                    
                    // Reset all stuck detection
                    steps_without_bananas = 0;
                    consecutive_same_cells = 0;
                    for (int i = 0; i < 10; i++) {
                        last_positions_x[i] = -1;
                        last_positions_y[i] = -1;
                    }
                    last_pos_index = 0;
                    
                    // Clear visited memory in teleport area
                    for (int dx = -3; dx <= 3; dx++) {
                        for (int dy = -3; dy <= 3; dy++) {
                            int nx = current.x + dx;
                            int ny = current.y + dy;
                            if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
                                f->visited_cells[nx][ny] = 0;
                            }
                        }
                    }
                    
                    // Energy cost for teleport
                    f->energy -= 8;
                    if (f->energy < 0) f->energy = 0;
                    
                    screen_x = maze_x + current.y * cell_size + cell_size/2;
                    screen_y = maze_y + current.x * cell_size + cell_size/2;
                    add_popup("EMERGENCY TELEPORT!", screen_x - 40, screen_y - 30, 2000);
                    
                    pthread_mutex_lock(&print_lock);
                    printf("    Teleported to (%d,%d), energy now: %d\n", 
                           current.x, current.y, f->energy);
                    pthread_mutex_unlock(&print_lock);
                    
                    continue; // Skip normal movement this cycle
                }
            }
            else if (force_direction_change) {
                // MODERATE RECOVERY: Force new exploration direction
                pthread_mutex_lock(&print_lock);
                printf("  -> Female %d forcing direction change\n", f->id);
                pthread_mutex_unlock(&print_lock);
                
                // Clear recent position history
                for (int i = 0; i < 5; i++) {
                    int idx = (last_pos_index - i - 1 + 10) % 10;
                    last_positions_x[idx] = -1;
                    last_positions_y[idx] = -1;
                }
                
                // Set a completely new random direction
                preferred_direction.x = (rand() % 3) - 1; // -1, 0, or 1
                preferred_direction.y = (rand() % 3) - 1;
                direction_persistence = 5; // Persist for 5 moves
                
                // Reduce visited counts in local area
                for (int dx = -2; dx <= 2; dx++) {
                    for (int dy = -2; dy <= 2; dy++) {
                        int nx = current.x + dx;
                        int ny = current.y + dy;
                        if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
                            if (f->visited_cells[nx][ny] > 0) {
                                f->visited_cells[nx][ny]--;
                            }
                        }
                    }
                }
                
                add_popup("CHANGING DIRECTION", screen_x - 35, screen_y - 30, 1500);
            }
            
            // ============================================
            // NORMAL MOVEMENT WITH INTELLIGENCE
            // ============================================
            
            // Move to neighboring cell
            int neighbor_count = get_neighbors(current.x, current.y, neighbors);
            if (neighbor_count > 0) {
                Position next;
                
                // Use intelligent movement if not in recovery mode
                if (!force_direction_change && direction_persistence <= 0) {
                    // SMART MOVEMENT: Evaluate all possible moves
                    float best_score = -100000.0f;
                    int best_index = 0;
                    
                    for (int i = 0; i < neighbor_count; i++) {
                        Position pos = neighbors[i];
                        float score = 0.0f;
                        
                        // 1. BANANA VALUE (Highest priority)
                        score += maze.cells[pos.x][pos.y].bananas * 100.0f;
                        
                        // 2. VISITED CELL PENALTY (Avoid revisiting)
                        int visits = f->visited_cells[pos.x][pos.y];
                        score -= visits * 20.0f;
                        
                        // 3. DISTANCE FROM BASE (Strategic)
                        int dist_from_base = abs(pos.x - f->family_base.x) + 
                                            abs(pos.y - f->family_base.y);
                        
                        if (collected >= BANANAS_TO_COLLECT) {
                            // Returning home: closer to base is better
                            score -= dist_from_base * 10.0f;
                        } else if (collected > 0) {
                            // Has some bananas: moderate preference for base
                            score -= dist_from_base * 3.0f;
                        } else {
                            // Searching: explore outward from base
                            int current_dist = abs(current.x - f->family_base.x) + 
                                             abs(current.y - f->family_base.y);
                            if (dist_from_base > current_dist) {
                                score += 5.0f; // Bonus for exploring outward
                            }
                        }
                        
                        // 4. ENEMY TERRITORY AVOIDANCE
                        for (int fam = 0; fam < NUM_MALE_APES; fam++) {
                            if (fam != f->family_id && male_apes[fam].active) {
                                if (is_position_in_family_territory(pos, fam, 2)) {
                                    score -= 30.0f; // Heavy penalty for enemy core territory
                                } else if (is_position_in_family_territory(pos, fam, 4)) {
                                    score -= 10.0f; // Moderate penalty for enemy territory
                                }
                            }
                        }
                        
                        // 5. PREFERRED DIRECTION BONUS (if we have one)
                        if (preferred_direction.x != 0 || preferred_direction.y != 0) {
                            int dx = pos.x - current.x;
                            int dy = pos.y - current.y;
                            if ((dx == preferred_direction.x || preferred_direction.x == 0) &&
                                (dy == preferred_direction.y || preferred_direction.y == 0)) {
                                score += 15.0f;
                            }
                        }
                        
                        // 6. LOCAL BANANA AWARENESS
                        // Check surrounding 3x3 area for bananas
                        int nearby_bananas = 0;
                        for (int dx = -1; dx <= 1; dx++) {
                            for (int dy = -1; dy <= 1; dy++) {
                                int nx = pos.x + dx;
                                int ny = pos.y + dy;
                                if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
                                    nearby_bananas += maze.cells[nx][ny].bananas;
                                }
                            }
                        }
                        score += nearby_bananas * 5.0f;
                        
                        if (score > best_score) {
                            best_score = score;
                            best_index = i;
                        }
                    }
                    
                    next = neighbors[best_index];
                    
                    // Update preferred direction based on chosen move
                    preferred_direction.x = next.x - current.x;
                    preferred_direction.y = next.y - current.y;
                    direction_persistence = 3; // Persist for 3 moves
                    
                } else {
                    // FORCED DIRECTION or PERSISTENT DIRECTION mode
                    if (direction_persistence > 0) {
                        // Try to continue in preferred direction
                        bool found_direction = false;
                        for (int i = 0; i < neighbor_count; i++) {
                            Position pos = neighbors[i];
                            int dx = pos.x - current.x;
                            int dy = pos.y - current.y;
                            
                            if ((preferred_direction.x == 0 || dx == preferred_direction.x) &&
                                (preferred_direction.y == 0 || dy == preferred_direction.y)) {
                                next = pos;
                                found_direction = true;
                                direction_persistence--;
                                break;
                            }
                        }
                        
                        if (!found_direction) {
                            // Can't continue in preferred direction
                            next = neighbors[rand() % neighbor_count];
                            direction_persistence = 0; // Reset
                        }
                    } else {
                        // Random move (fallback)
                        next = neighbors[rand() % neighbor_count];
                    }
                }
                
                // Clear current cell
                pthread_mutex_lock(&display_lock);
                if (current.x >= 0 && current.y >= 0) {
                    maze.cells[current.x][current.y].female_id = -1;
                    maze.cells[current.x][current.y].trail_family = f->family_id;
                    maze.cells[current.x][current.y].trail_age = 20;
                }
                pthread_mutex_unlock(&display_lock);
                
                // Animate movement to next cell
                move_female_to_position(f, next);
                current = next;
                
                // Energy cost for moving
                f->energy--;
                if (f->energy < 0) f->energy = 0;
                
                steps_in_maze++;
            } else {
                // No valid neighbors, return to base
                pthread_mutex_lock(&print_lock);
                printf("  Female %d stuck with no neighbors, returning to base...\n", f->id);
                pthread_mutex_unlock(&print_lock);
                
                pthread_mutex_lock(&display_lock);
                if (current.x >= 0 && current.y >= 0) {
                    maze.cells[current.x][current.y].female_id = -1;
                }
                pthread_mutex_unlock(&display_lock);
                
                move_female_to_position(f, f->family_base);
                current = f->family_base;
                break;
            }
            
            // Small delay to make movement visible
            usleep(50000);
        }
        
        // Return to base if not already there
        // Replace the instant teleport with gradual pathfinding
if (current.x != f->family_base.x || current.y != f->family_base.y) {
    // Return home step by step
    while (current.x != f->family_base.x || current.y != f->family_base.y) {
        Position neighbors[8];
        int neighbor_count = get_neighbors(current.x, current.y, neighbors);
        
        if (neighbor_count > 0) {
            // Find neighbor closest to base
            Position best = neighbors[0];
            int min_dist = abs(best.x - f->family_base.x) + abs(best.y - f->family_base.y);
            
            for (int i = 1; i < neighbor_count; i++) {
                int dist = abs(neighbors[i].x - f->family_base.x) + 
                          abs(neighbors[i].y - f->family_base.y);
                if (dist < min_dist) {
                    min_dist = dist;
                    best = neighbors[i];
                }
            }
            
            // CHECK FOR FEMALE ENCOUNTER HERE
            pthread_mutex_lock(&display_lock);
            int other_female_id = maze.cells[best.x][best.y].female_id;
            pthread_mutex_unlock(&display_lock);
            
            if (other_female_id >= 0 && other_female_id != f->id &&
                female_apes[other_female_id].active &&
                female_apes[other_female_id].collected_bananas > 0 &&
                rand() % 100 < FEMALE_FIGHT_PROBABILITY) {
                
                // FIGHT LOGIC HERE
                pthread_mutex_lock(&print_lock);
                printf("Female %d encountered Female %d at (%d,%d)! FIGHT!\n",
                       f->id, other_female_id, best.x, best.y);
                pthread_mutex_unlock(&print_lock);
                
                float maze_x = 30, maze_y = 80;
                float cell_size = (float)MAZE_DISPLAY_SIZE / MAZE_SIZE;
                float screen_x = maze_x + best.y * cell_size + cell_size/2;
                float screen_y = maze_y + best.x * cell_size + cell_size/2;
                
                pthread_mutex_lock(&display_lock);
                fight_effect_active = true;
                fight_location.x = best.x;
                fight_location.y = best.y;
                fight_effect_timer = 60;
                pthread_mutex_unlock(&display_lock);
                
                add_popup("FEMALE FIGHT!", screen_x - 25, screen_y - 30, 2000);
                
                // Both lose energy
                f->energy -= 20;
                female_apes[other_female_id].energy -= 20;
                if (f->energy < 0) f->energy = 0;
                if (female_apes[other_female_id].energy < 0) 
                    female_apes[other_female_id].energy = 0;
                
                // Fight outcome
                int total_bananas = f->collected_bananas + 
                                   female_apes[other_female_id].collected_bananas;
                float my_chance = (f->collected_bananas * 100.0f) / total_bananas;
                
                if (rand() % 100 < my_chance) {
                    // I win
                    collected += female_apes[other_female_id].collected_bananas;
                    f->collected_bananas = collected;
                    female_apes[other_female_id].collected_bananas = 0;
                    
                    pthread_mutex_lock(&print_lock);
                    printf("  -> Female %d WON! Stole %d bananas\n", 
                           f->id, female_apes[other_female_id].collected_bananas);
                    pthread_mutex_unlock(&print_lock);
                    add_popup("WON FIGHT!", screen_x - 20, screen_y + 20, 1500);
                } else {
                    // I lose
                    female_apes[other_female_id].collected_bananas += collected;
                    collected = 0;
                    f->collected_bananas = 0;
                    
                    pthread_mutex_lock(&print_lock);
                    printf("  -> Female %d LOST! Lost %d bananas\n", f->id, collected);
                    pthread_mutex_unlock(&print_lock);
                    add_popup("LOST FIGHT!", screen_x - 25, screen_y + 20, 1500);
                }
                
                sleep(1);
            }
            
            // Move to next cell
            pthread_mutex_lock(&display_lock);
            if (current.x >= 0 && current.y >= 0) {
                maze.cells[current.x][current.y].female_id = -1;
            }
            pthread_mutex_unlock(&display_lock);
            
            move_female_to_position(f, best);
            current = best;
            
            f->energy--; // Energy cost for moving
            if (f->energy < 0) f->energy = 0;
        } else {
            break; // Stuck
        }
    }
}
        
        // Now at base - deposit bananas
        pthread_mutex_lock(&print_lock);
        printf("Female %d at base with %d bananas after %d steps. Energy: %d\n", 
               f->id, collected, steps_in_maze, f->energy);
        pthread_mutex_unlock(&print_lock);
        
        // Check for female-female encounter fights at base
        if (collected > 0 && rand() % 100 < FEMALE_FIGHT_PROBABILITY) {
            // Check if any other female is at the same base
            bool other_female_at_base = false;
            for (int i = 0; i < NUM_FEMALE_APES; i++) {
                if (i != f->id && female_apes[i].active && 
                    female_apes[i].pos.x == f->family_base.x && 
                    female_apes[i].pos.y == f->family_base.y) {
                    other_female_at_base = true;
                    break;
                }
            }
            
            if (other_female_at_base) {
                pthread_mutex_lock(&print_lock);
                printf("Female %d encountered another female at base! FIGHT!\n", f->id);
                pthread_mutex_unlock(&print_lock);
                
                // Set fight effect at base location
                pthread_mutex_lock(&display_lock);
                fight_effect_active = true;
                fight_location = f->family_base;
                fight_effect_timer = 60;
                pthread_mutex_unlock(&display_lock);
                
                add_popup("FEMALE FIGHT AT BASE!", screen_x - 35, screen_y - 30, 2000);
                sleep(1);
                
                f->energy -= 20;
                if (f->energy < 0) f->energy = 0;
                
                if (rand() % 2 == 0) {
                    collected = 0;
                    f->collected_bananas = 0;
                    pthread_mutex_lock(&print_lock);
                    printf("  -> Female %d LOST the fight!\n", f->id);
                    pthread_mutex_unlock(&print_lock);
                    add_popup("LOST FIGHT!", screen_x - 25, screen_y - 30, 2000);
                } else {
                    pthread_mutex_lock(&print_lock);
                    printf("  -> Female %d WON the fight!\n", f->id);
                    pthread_mutex_unlock(&print_lock);
                    add_popup("WON FIGHT!", screen_x - 20, screen_y - 30, 2000);
                }
            }
        }
        
        // Deposit bananas to family basket
        if (collected > 0 && male_apes[f->family_id].active) {
            pthread_mutex_lock(&male_apes[f->family_id].basket->basket_lock);
            male_apes[f->family_id].basket->bananas += collected;
            int total = male_apes[f->family_id].basket->bananas;
            pthread_mutex_unlock(&male_apes[f->family_id].basket->basket_lock);
            
            pthread_mutex_lock(&print_lock);
            printf("  -> Deposited %d bananas to Family %d. Total: %d\n", 
                   collected, f->family_id, total);
            pthread_mutex_unlock(&print_lock);
            
            // Check if family reached threshold
            if (total >= FAMILY_BANANA_THRESHOLD) {
                add_popup("FAMILY REACHED GOAL!", screen_x - 35, screen_y - 30, 3000);
            }
        }
        
        f->collected_bananas = 0;
        
        // Fade memory over time
        for (int x = 0; x < MAZE_SIZE; x++) {
            for (int y = 0; y < MAZE_SIZE; y++) {
                if (f->visited_cells[x][y] > 0) {
                    f->visited_cells[x][y]--; // Gradual forgetting
                }
            }
        }
        
        sleep(1); // Wait before next collection trip
        check_simulation_end();
    }
    
    // Cleanup memory
    if (f->visited_cells != NULL) {
        for (int x = 0; x < MAZE_SIZE; x++) {
            free(f->visited_cells[x]);
        }
        free(f->visited_cells);
        f->visited_cells = NULL;
    }
    
    free(last_positions_x);
    free(last_positions_y);
    
    pthread_mutex_lock(&display_lock);
    if (f->pos.x >= 0 && f->pos.y >= 0) {
        maze.cells[f->pos.x][f->pos.y].female_id = -1;
    }
    f->pos.x = -1;
    f->pos.y = -1;
    f->display_pos.x = -1;
    f->display_pos.y = -1;
    f->moving = false;
    pthread_mutex_unlock(&display_lock);
    
    pthread_mutex_lock(&print_lock);
    printf("Female %d thread ending\n", f->id);
    pthread_mutex_unlock(&print_lock);
    
    return NULL;
}

void* male_ape_function(void* arg) {
    MaleApe* m = (MaleApe*)arg;
    
    while (simulation_running && m->active) {
        m->energy--;
        if (m->energy < 0) m->energy = 0;
        
        if (m->energy <= MALE_WITHDRAW_THRESHOLD) {
            pthread_mutex_lock(&simulation_lock);
            if (m->active) {
                m->active = false;
                withdrawn_families++;
                
                for (int i = 0; i < NUM_FEMALE_APES; i++) {
                    if (female_apes[i].family_id == m->family_id) {
                        female_apes[i].active = false;
                    }
                }
                for (int i = 0; i < NUM_BABY_APES; i++) {
                    if (baby_apes[i].family_id == m->family_id) {
                        baby_apes[i].active = false;
                    }
                }
                
                pthread_mutex_lock(&print_lock);
                printf("\n*** Family %d WITHDRAWS from simulation! ***\n", m->family_id);
                printf("    Base Location: (%d, %d)\n", m->family_base.x, m->family_base.y);
                printf("    Male Energy: %d (threshold: %d)\n", m->energy, MALE_WITHDRAW_THRESHOLD);
                printf("    Final Basket: %d bananas\n", m->basket->bananas);
                pthread_mutex_unlock(&print_lock);
                
                float maze_x = 30, maze_y = 80, cell_size = (float)MAZE_DISPLAY_SIZE/MAZE_SIZE;
                float screen_x = maze_x + m->family_base.y * cell_size + cell_size/2;
                float screen_y = maze_y + m->family_base.x * cell_size + cell_size/2;
                char withdraw_text[50];
                sprintf(withdraw_text, "FAMILY %d WITHDRAWS!", m->family_id);
                add_popup(withdraw_text, screen_x - 35, screen_y, 3000);
            }
            pthread_mutex_unlock(&simulation_lock);
            break;
        }
        
        if (m->basket->bananas > 0 && !m->fighting && rand() % 100 < 30) {
            m->energy += 3;
            if (m->energy > MALE_ENERGY_MAX) m->energy = MALE_ENERGY_MAX;
        }
        
        int fight_probability = fmin(m->basket->bananas * MALE_FIGHT_PROBABILITY / 10, 80);
        
        if (m->basket->bananas > 2 && rand() % 100 < fight_probability) {
            int neighbor_opponent = -1;
            int max_neighbor_distance = 5;
            
            for (int i = 0; i < NUM_MALE_APES; i++) {
                if (i != m->id && male_apes[i].active && male_apes[i].basket->bananas > 0) {
                    int dx = abs(m->family_base.x - male_apes[i].family_base.x);
                    int dy = abs(m->family_base.y - male_apes[i].family_base.y);
                    int distance = dx + dy;
                    
                    if (distance <= max_neighbor_distance) {
                        neighbor_opponent = i;
                        pthread_mutex_lock(&print_lock);
                        printf("Male %d (Family %d) targets NEIGHBOR Family %d (distance: %d)\n", 
                               m->id, m->family_id, i, distance);
                        pthread_mutex_unlock(&print_lock);
                        break;
                    }
                }
            }
            
            int opponent = -1;
            
            if (neighbor_opponent != -1) {
                opponent = neighbor_opponent;
            } else {
                int max_bananas = 0;
                for (int i = 0; i < NUM_MALE_APES; i++) {
                    if (i != m->id && male_apes[i].active && male_apes[i].basket->bananas > 0) {
                        if (male_apes[i].basket->bananas > max_bananas) {
                            max_bananas = male_apes[i].basket->bananas;
                            opponent = i;
                        }
                    }
                }
                
                if (opponent == -1) {
                    do {
                        opponent = rand() % NUM_MALE_APES;
                    } while (opponent == m->id || !male_apes[opponent].active || 
                            male_apes[opponent].basket->bananas == 0);
                }
            }
            
            if (opponent != -1 && opponent != m->id) {
                pthread_mutex_lock(&display_lock);
                m->fighting = true;
                male_apes[opponent].fighting = true;
                fight_effect_active = true;
                fight_location = m->family_base;
                fight_effect_timer = 60;
                pthread_mutex_unlock(&display_lock);
                
                pthread_mutex_lock(&print_lock);
                printf("\n*** MALE FIGHT! Family %d (%d,%d) vs Family %d (%d,%d) ***\n", 
                       m->family_id, m->family_base.x, m->family_base.y,
                       male_apes[opponent].family_id, 
                       male_apes[opponent].family_base.x, male_apes[opponent].family_base.y);
                printf("    Basket: %d vs %d bananas | Energy: %d vs %d\n", 
                       m->basket->bananas, male_apes[opponent].basket->bananas,
                       m->energy, male_apes[opponent].energy);
                pthread_mutex_unlock(&print_lock);
                
                float maze_x = 30, maze_y = 80, cell_size = (float)MAZE_DISPLAY_SIZE/MAZE_SIZE;
                float screen_x = maze_x + m->family_base.y * cell_size + cell_size/2;
                float screen_y = maze_y + m->family_base.x * cell_size + cell_size/2;
                char fight_text[50];
                sprintf(fight_text, "FAMILY %d ATTACKS!", m->family_id);
                add_popup(fight_text, screen_x - 25, screen_y, 3000);
                
                float opp_screen_x = maze_x + male_apes[opponent].family_base.y * cell_size + cell_size/2;
                float opp_screen_y = maze_y + male_apes[opponent].family_base.x * cell_size + cell_size/2;
                char defense_text[50];
                sprintf(defense_text, "FAMILY %d DEFENDS!", male_apes[opponent].family_id);
                add_popup(defense_text, opp_screen_x - 25, opp_screen_y, 3000);
                
                sleep(2);
                
                int total_bananas = m->basket->bananas + male_apes[opponent].basket->bananas;
                int total_energy = m->energy + male_apes[opponent].energy;
                
                float defender_advantage = 1.0f;
                int distance = abs(m->family_base.x - male_apes[opponent].family_base.x) + 
                              abs(m->family_base.y - male_apes[opponent].family_base.y);
                
                if (distance <= 2) {
                    defender_advantage = 1.3f;
                }
                
                float aggressor_chance = (m->basket->bananas * 60.0f / total_bananas) + 
                                        (m->energy * 40.0f / total_energy);
                float defender_chance = (male_apes[opponent].basket->bananas * 60.0f / total_bananas) + 
                                       (male_apes[opponent].energy * 40.0f / total_energy) * defender_advantage;
                
                float total = aggressor_chance + defender_chance;
                aggressor_chance = (aggressor_chance / total) * 100.0f;
                
                bool wins = (rand() % 100) < aggressor_chance;
                
                m->energy -= 25;
                male_apes[opponent].energy -= 25;
                if (m->energy < 0) m->energy = 0;
                if (male_apes[opponent].energy < 0) male_apes[opponent].energy = 0;
                
                if (wins) {
                    pthread_mutex_lock(&m->basket->basket_lock);
                    pthread_mutex_lock(&male_apes[opponent].basket->basket_lock);
                    
                    int stolen = male_apes[opponent].basket->bananas;
                    m->basket->bananas += stolen;
                    male_apes[opponent].basket->bananas = 0;
                    
                    pthread_mutex_unlock(&male_apes[opponent].basket->basket_lock);
                    pthread_mutex_unlock(&m->basket->basket_lock);
                    
                    pthread_mutex_lock(&print_lock);
                    printf("  -> Family %d WINS! Stole %d bananas from Family %d\n", 
                           m->family_id, stolen, male_apes[opponent].family_id);
                    printf("     Distance between bases: %d cells\n", distance);
                    printf("     New energy: Family %d=%d, Family %d=%d\n", 
                           m->family_id, m->energy, 
                           male_apes[opponent].family_id, male_apes[opponent].energy);
                    pthread_mutex_unlock(&print_lock);
                    
                    pthread_mutex_lock(&display_lock);
                    fight_location = m->family_base;
                    fight_effect_timer = 30;
                    pthread_mutex_unlock(&display_lock);
                    
                    char win_text[50];
                    sprintf(win_text, "FAMILY %d WINS %d BANANAS!", m->family_id, stolen);
                    add_popup(win_text, screen_x - 40, screen_y + 20, 2000);
                } else {
                    pthread_mutex_lock(&m->basket->basket_lock);
                    pthread_mutex_lock(&male_apes[opponent].basket->basket_lock);
                    
                    int stolen = m->basket->bananas;
                    male_apes[opponent].basket->bananas += stolen;
                    m->basket->bananas = 0;
                    
                    pthread_mutex_unlock(&male_apes[opponent].basket->basket_lock);
                    pthread_mutex_unlock(&m->basket->basket_lock);
                    
                    pthread_mutex_lock(&print_lock);
                    printf("  -> Family %d LOSES! Lost %d bananas to Family %d\n", 
                           m->family_id, stolen, male_apes[opponent].family_id);
                    printf("     Distance between bases: %d cells\n", distance);
                    printf("     Defender advantage: %.1fx\n", defender_advantage);
                    printf("     New energy: Family %d=%d, Family %d=%d\n", 
                           m->family_id, m->energy, 
                           male_apes[opponent].family_id, male_apes[opponent].energy);
                    pthread_mutex_unlock(&print_lock);
                    
                    pthread_mutex_lock(&display_lock);
                    fight_location = male_apes[opponent].family_base;
                    fight_effect_timer = 30;
                    pthread_mutex_unlock(&display_lock);
                    
                    char lose_text[50];
                    sprintf(lose_text, "FAMILY %d DEFENDS SUCCESSFULLY!", male_apes[opponent].family_id);
                    add_popup(lose_text, opp_screen_x - 40, opp_screen_y + 20, 2000);
                }
                
                pthread_mutex_lock(&display_lock);
                m->fighting = false;
                male_apes[opponent].fighting = false;
                pthread_mutex_unlock(&display_lock);
            }
        }
        
        if (rand() % 100 < 20) {
            for (int i = 0; i < NUM_MALE_APES; i++) {
                if (i != m->id && male_apes[i].active) {
                    int dx = abs(m->family_base.x - male_apes[i].family_base.x);
                    int dy = abs(m->family_base.y - male_apes[i].family_base.y);
                    int distance = dx + dy;
                    
                    if (distance <= 3) {
                        pthread_mutex_lock(&print_lock);
                        printf("Family %d notes close neighbor Family %d (distance: %d)\n", 
                               m->family_id, i, distance);
                        pthread_mutex_unlock(&print_lock);
                        
                        if (distance <= 2 && rand() % 100 < 50) {
                            pthread_mutex_lock(&display_lock);
                            fight_effect_active = true;
                            fight_location.x = (m->family_base.x + male_apes[i].family_base.x) / 2;
                            fight_location.y = (m->family_base.y + male_apes[i].family_base.y) / 2;
                            fight_effect_timer = 10;
                            pthread_mutex_unlock(&display_lock);
                        }
                    }
                }
            }
        }
        
        sleep(3);
        check_simulation_end();
    }
    
    pthread_mutex_lock(&print_lock);
    printf("Male %d (Family %d) thread ending. Base was at (%d, %d)\n", 
           m->id, m->family_id, m->family_base.x, m->family_base.y);
    pthread_mutex_unlock(&print_lock);
    
    return NULL;
}

void* baby_ape_function(void* arg) {
    BabyApe* b = (BabyApe*)arg;
    
    while (simulation_running && b->active) {
        if (!male_apes[b->family_id].active) {
            b->active = false;
            break;
        }
        
        bool any_fighting = false;
        for (int i = 0; i < NUM_MALE_APES; i++) {
            if (male_apes[i].fighting) {
                any_fighting = true;
                break;
            }
        }
        
        int steal_chance = any_fighting ? BABY_STEAL_PROBABILITY : 20;
        
        if (rand() % 100 < steal_chance) {
            pthread_mutex_lock(&display_lock);
            b->stealing = true;
            pthread_mutex_unlock(&display_lock);
            
            int target_family = -1;
            for (int i = 0; i < NUM_MALE_APES; i++) {
                if (i != b->family_id && male_apes[i].active && 
                    male_apes[i].basket->bananas > 0) {
                    target_family = i;
                    break;
                }
            }
            
            if (target_family >= 0) {
                pthread_mutex_lock(&male_apes[target_family].basket->basket_lock);
                int available = male_apes[target_family].basket->bananas;
                
                if (available > 0) {
                    int steal_amount = fmin((rand() % 3) + 1, available);
                    male_apes[target_family].basket->bananas -= steal_amount;
                    
                    float maze_x = 30, maze_y = 80;
                    float screen_x = maze_x + MAZE_DISPLAY_SIZE/2;
                    float screen_y = maze_y + MAZE_DISPLAY_SIZE/2;
                    
                    if (rand() % 100 < 60) {
                        b->eaten_bananas += steal_amount;
                        
                        pthread_mutex_lock(&print_lock);
                        printf("Baby %d (Family %d) ate %d stolen bananas from Family %d! Total eaten: %d\n", 
                               b->id, b->family_id, steal_amount, target_family, b->eaten_bananas);
                        pthread_mutex_unlock(&print_lock);
                        
                        char eat_text[50];
                        sprintf(eat_text, "BABY %d EATS %d!", b->id, steal_amount);
                        add_popup(eat_text, screen_x - 30, screen_y + 40, 2000);
                        
                        if (b->eaten_bananas >= BABY_EATEN_THRESHOLD) {
                            pthread_mutex_lock(&print_lock);
                            printf("\n*** Baby %d has eaten %d bananas! ***\n", 
                                   b->id, b->eaten_bananas);
                            pthread_mutex_unlock(&print_lock);
                            
                            char threshold_text[50];
                            sprintf(threshold_text, "BABY %d REACHED LIMIT!", b->id);
                            add_popup(threshold_text, screen_x - 40, screen_y + 60, 3000);
                        }
                    } else {
                        if (male_apes[b->family_id].active) {
                            pthread_mutex_lock(&male_apes[b->family_id].basket->basket_lock);
                            male_apes[b->family_id].basket->bananas += steal_amount;
                            pthread_mutex_unlock(&male_apes[b->family_id].basket->basket_lock);
                            
                            pthread_mutex_lock(&print_lock);
                            printf("Baby %d (Family %d) stole %d bananas from Family %d for Dad!\n", 
                                   b->id, b->family_id, steal_amount, target_family);
                            pthread_mutex_unlock(&print_lock);
                            
                            char give_text[50];
                            sprintf(give_text, "BABY %d GIVES TO DAD!", b->id);
                            add_popup(give_text, screen_x - 35, screen_y + 40, 2000);
                        }
                    }
                }
                pthread_mutex_unlock(&male_apes[target_family].basket->basket_lock);
            }
            
            pthread_mutex_lock(&display_lock);
            b->stealing = false;
            pthread_mutex_unlock(&display_lock);
        }
        
        sleep(4 + (rand() % 3));
        check_simulation_end();
    }
    
    return NULL;
}

void init_apes() {
    female_apes = malloc(NUM_FEMALE_APES * sizeof(FemaleApe));
    male_apes = malloc(NUM_MALE_APES * sizeof(MaleApe));
    baby_apes = malloc(NUM_BABY_APES * sizeof(BabyApe));
    
    initialize_family_bases();
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        male_apes[i].id = i;
        male_apes[i].family_id = i;
        male_apes[i].energy = MALE_ENERGY_MAX;
        male_apes[i].active = true;
        male_apes[i].fighting = false;
        male_apes[i].family_base = family_bases[i];
        
        male_apes[i].basket = malloc(sizeof(Basket));
        male_apes[i].basket->bananas = 0;
        pthread_mutex_init(&male_apes[i].basket->basket_lock, NULL);
    }
    
    for (int i = 0; i < NUM_FEMALE_APES; i++) {
        female_apes[i].id = i;
        female_apes[i].family_id = i % NUM_MALE_APES;
        female_apes[i].energy = FEMALE_ENERGY_MAX;
        female_apes[i].collected_bananas = 0;
        female_apes[i].active = true;
        female_apes[i].resting = false;
        female_apes[i].moving = false;
        female_apes[i].move_progress = 0.0f;
        female_apes[i].family_base = family_bases[female_apes[i].family_id];
        female_apes[i].pos = female_apes[i].family_base;
        female_apes[i].display_pos = female_apes[i].family_base;
        female_apes[i].target_pos.x = -1;
        female_apes[i].target_pos.y = -1;
        female_apes[i].preferred_direction.x = 0;  // ADD THIS
        female_apes[i].preferred_direction.y = 0;  // ADD THIS
        
        // ADD THIS BLOCK INSIDE THE LOOP:
        female_apes[i].visited_cells = malloc(MAZE_SIZE * sizeof(int*));
        for (int x = 0; x < MAZE_SIZE; x++) {
            female_apes[i].visited_cells[x] = malloc(MAZE_SIZE * sizeof(int));
            for (int y = 0; y < MAZE_SIZE; y++) {
                female_apes[i].visited_cells[x][y] = 0;
            }
        }
        female_apes[i].visit_threshold = 3;
    }
    
    for (int i = 0; i < NUM_BABY_APES; i++) {
        baby_apes[i].id = i;
        baby_apes[i].family_id = i % NUM_MALE_APES;
        baby_apes[i].eaten_bananas = 0;
        baby_apes[i].active = true;
        baby_apes[i].stealing = false;
        baby_apes[i].family_base = family_bases[baby_apes[i].family_id];
    }
    
    for (int i = 0; i < MAX_POPUPS; i++) {
        popups[i].active = false;
    }
}
Position get_smart_move(FemaleApe* f, Position* neighbors, int neighbor_count) {
    Position best_move = neighbors[0];
    float best_score = -1000.0f;
    
    for (int i = 0; i < neighbor_count; i++) {
        Position pos = neighbors[i];
        float score = 0.0f;
        
        // Banana value (highest priority)
        score += maze.cells[pos.x][pos.y].bananas * 20.0f;
        
        // Avoid recently visited cells
        int visits = f->visited_cells[pos.x][pos.y];
        score -= visits * 10.0f;
        
        // Prefer unexplored areas
        if (visits == 0) {
            score += 15.0f;
        }
        
        // When returning home, prioritize moves toward base
        if (f->collected_bananas >= BANANAS_TO_COLLECT) {
            int dx = abs(pos.x - f->family_base.x);
            int dy = abs(pos.y - f->family_base.y);
            score -= (dx + dy) * 3.0f; // Closer to base is better
        } else {
            // When searching, avoid backtracking too much
            int dx_from_current = abs(pos.x - f->pos.x);
            int dy_from_current = abs(pos.y - f->pos.y);
            score += (dx_from_current + dy_from_current) * 0.5f; // Encourage movement
        }
        
        if (score > best_score) {
            best_score = score;
            best_move = pos;
        }
    }
    
    return best_move;
}
void start_simulation() {
    printf("\n===================================================\n");
    printf("    APES COLLECTING BANANAS - SIMULATION START    \n");
    printf("===================================================\n\n");
    
    simulation_start_time = time(NULL);
    
    for (int i = 0; i < NUM_FEMALE_APES; i++) {
        pthread_create(&female_apes[i].thread, NULL, female_ape_function, &female_apes[i]);
    }
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        pthread_create(&male_apes[i].thread, NULL, male_ape_function, &male_apes[i]);
    }
    
    for (int i = 0; i < NUM_BABY_APES; i++) {
        pthread_create(&baby_apes[i].thread, NULL, baby_ape_function, &baby_apes[i]);
    }
}

void cleanup() {
    printf("\n=== Cleaning up simulation... ===\n");
    
    for (int i = 0; i < NUM_FEMALE_APES; i++) {
        if (female_apes[i].active) {
            pthread_join(female_apes[i].thread, NULL);
        }
    }
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        if (male_apes[i].active) {
            pthread_join(male_apes[i].thread, NULL);
        }
    }
    
    for (int i = 0; i < NUM_BABY_APES; i++) {
        if (baby_apes[i].active) {
            pthread_join(baby_apes[i].thread, NULL);
        }
    }
    
    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            pthread_mutex_destroy(&maze.cells[i][j].cell_lock);
        }
        free(maze.cells[i]);
    }
    free(maze.cells);
    
    for (int i = 0; i < NUM_MALE_APES; i++) {
        pthread_mutex_destroy(&male_apes[i].basket->basket_lock);
        free(male_apes[i].basket);
    }
    
    free(female_apes);
    free(male_apes);
    free(baby_apes);
    
    pthread_mutex_destroy(&simulation_lock);
    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&display_lock);
    
    printf("Cleanup completed.\n");
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27 || key == 'q' || key == 'Q') {
        printf("\n*** Simulation manually terminated by user ***\n");
        simulation_running = false;
        cleanup();
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    char config_file[256] = "config.txt";
    if (argc > 1) {
        strcpy(config_file, argv[1]);
    }
    read_config(config_file);
    
    printf("===================================================\n");
    printf("               SIMULATION CONFIGURATION            \n");
    printf("===================================================\n");
    printf("Maze Size: %dx%d\n", MAZE_SIZE, MAZE_SIZE);
    printf("Obstacles: %d%% | Bananas: %d%%\n", OBSTACLE_PERCENTAGE, BANANA_PERCENTAGE);
    printf("Female Apes: %d | Male Apes: %d | Baby Apes: %d\n", 
           NUM_FEMALE_APES, NUM_MALE_APES, NUM_BABY_APES);
    printf("Bananas to Collect: %d\n", BANANAS_TO_COLLECT);
    printf("Simulation Time Limit: %d seconds\n", MAX_SIMULATION_TIME);
    printf("Withdrawal Threshold: %d families\n", WITHDRAWAL_THRESHOLD);
    printf("Family Banana Threshold: %d\n", FAMILY_BANANA_THRESHOLD);
    printf("Baby Eaten Threshold: %d\n", BABY_EATEN_THRESHOLD);
    printf("Female Fight Probability: %d%%\n", FEMALE_FIGHT_PROBABILITY);
    printf("Male Fight Probability: %d%%\n", MALE_FIGHT_PROBABILITY);
    printf("Baby Steal Probability: %d%%\n", BABY_STEAL_PROBABILITY);
    printf("===================================================\n\n");
    
    init_maze();
    init_apes();
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Apes Collecting Bananas - ENCS4330 Project 3");
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(50, timer, 0);
    glutKeyboardFunc(keyboard);
    
    start_simulation();
    
    glutMainLoop();
    
    return 0;
}
