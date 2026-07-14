#include "raylib.h"
#include <math.h>
#include <stdbool.h>

#define WIDTH 800
#define HEIGHT 600

//---------------Global constants ----------
#define STIFFNESS 1.0 // more stiff more hard ot stretch 
                      //(opposite of elasticity)
#define STRUCT_STIFFNESS 0.6f  // Keeps the grid structure intact
#define SHEAR_STIFFNESS  0.4f  // Allows some rubbery twisting
#define BEND_STIFFNESS   0.1f  // Allows it to fold naturally

#define GRAVITY 500.0f;
#define DAMPING 0.99 
#define SPRING_SOLVER_ITERATION 1 // 1 -> more elastic 

#define GRID_WIDTH 25
#define GRID_HEIGHT 25

#define STRUCT_HORIZ   ((GRID_WIDTH - 1) * GRID_HEIGHT)
#define STRUCT_VERT    (GRID_WIDTH * (GRID_HEIGHT - 1))
#define SHEAR_DIAG     ((GRID_WIDTH - 1) * (GRID_HEIGHT - 1))
#define SHEAR_ANTI     ((GRID_WIDTH - 1) * (GRID_HEIGHT - 1))
#define BEND_HORIZ     ((GRID_WIDTH - 2) * GRID_HEIGHT)
#define BEND_VERT      (GRID_WIDTH * (GRID_HEIGHT - 2))

#define TOTAL_SPRINGS  (STRUCT_HORIZ + STRUCT_VERT + SHEAR_DIAG + SHEAR_ANTI + BEND_HORIZ + BEND_VERT)
#define TOTAL_PARTICLES (GRID_WIDTH * GRID_HEIGHT)

// ------- structs --------
typedef struct{
    Vector2 position;
    Vector2 previous_position;
    Vector2 acceleration;
    float mass;
    bool is_pinned; // True for corners holding the cloth up
}Particle;

typedef struct {
    int particle_A; //Index of first particle
    int particle_B; //Index of second particle
    float rest_length; //original distance between them
    float stiffness; // structural stiffness (k)
}Spring;

// ---------function declarations --------
void update_particle(Particle *obj, float dt);
void calculate_spring(Particle *a, Particle *b, float rest_length, float stiffness);
void generate_particles(Particle particles[], int width, int height, Vector2 position, 
                float rest_length, float mass);
void generate_spring_grid(Spring springs[], Particle particles[], int width, int height);

void draw_particle(Particle obj);
void draw_link_particle(Particle a,Particle b);

int main()
{
    InitWindow(WIDTH, HEIGHT, "Mass Spring Cloth Simulation");
    SetTargetFPS(60); 
    //-------variable declaration -------------
    //const int totalSprings = (6 * width * height) - (5 * width) - (5 * height) + 2;

    Particle particles[TOTAL_PARTICLES];
    Spring springs[TOTAL_SPRINGS];
    
    generate_particles(particles,GRID_WIDTH, GRID_HEIGHT,(Vector2){80,10}, 20, 10);
    generate_spring_grid(springs, particles, GRID_WIDTH, GRID_HEIGHT);


    // ------- game loop ---------
    while(!WindowShouldClose()){
        //----------update everything ------------
        float dt = GetFrameTime();
        
        static int grabbedIdx = -1; 
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
            Vector2 mousePos = GetMousePosition();
            float closestDist = 5.0f;

            for(int i = 0; i < TOTAL_PARTICLES; i++){
                if(!particles[i].is_pinned){
                    float dx = particles[i].position.x - mousePos.x;
                    float dy = particles[i].position.y - mousePos.y;
                    float dist = sqrtf(dx*dx + dy*dy);

                    if(dist < closestDist){
                        closestDist = dist;
                        grabbedIdx = i;
                    }
                }
            }
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && grabbedIdx != -1) {
            Vector2 mousePos = GetMousePosition();
            particles[grabbedIdx].position = mousePos;
            particles[grabbedIdx].previous_position = mousePos;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            int grabbedIdx = -1; // Let go
        }

        for(int y = 0; y < GRID_HEIGHT; y++){
            for(int x = 0; x < GRID_WIDTH; x++){
                int idx = x + (y*GRID_WIDTH);
                update_particle(&particles[idx], 1.0f/60.0f);
            }
        }

        for(int j = 0; j < SPRING_SOLVER_ITERATION; j++)
            for(int i = 0; i < TOTAL_SPRINGS; i++){
                calculate_spring(&particles[springs[i].particle_A], &particles[springs[i].particle_B], 
                springs[i].rest_length, springs[i].stiffness);
            }

        //------------draw -------------
        BeginDrawing();
        ClearBackground(BLACK);

        for(int y = 0; y < GRID_HEIGHT; y++){
            for(int x = 0; x < GRID_WIDTH; x++){
                int idx = x + (y*GRID_WIDTH);
                // Right neighbor
                if (x < GRID_WIDTH- 1) {
                    int neighborIndex = (x + 1) + (y * GRID_WIDTH);
                    draw_link_particle(particles[idx],particles[neighborIndex]);
                }
                // Down neighbor
                if (y <  GRID_HEIGHT- 1) {
                    int neighborIndex = x + ((y + 1) * GRID_WIDTH);
                    draw_link_particle(particles[idx],particles[neighborIndex]);
                }
            }
        }

        for(int y = 0; y < GRID_HEIGHT; y++){
            for(int x = 0; x < GRID_WIDTH; x++){
                int idx = x + (y*GRID_WIDTH);
                draw_particle(particles[idx]);
            }
        }

        EndDrawing();
    }
    // ----- close everything -------
    CloseWindow();
    return 0;
}
// -------- function definition -----------
void update_particle(Particle *obj, float dt)
{
   if(obj->is_pinned) return;// Do not move pinned anchor particles

   obj->acceleration.x = 0;
   obj->acceleration.y = GRAVITY;

   // a = f/m
   float tx = obj->position.x;
   float ty = obj->position.y;

   float vx = (tx - obj->previous_position.x) * DAMPING;
   float vy = (ty - obj->previous_position.y) * DAMPING;

   obj->position.x = tx + vx + (obj->acceleration.x * dt * dt);
   obj->position.y = ty + vy + (obj->acceleration.y * dt * dt);

   obj->previous_position.x = tx;
   obj->previous_position.y = ty;
}

void draw_particle(Particle obj)
{
    DrawCircleV(obj.position, 5, BLUE);
}
void draw_link_particle(Particle a,Particle b)
{
    DrawLineEx(a.position, b.position, 5, GREEN);
}

void calculate_spring(Particle *a, Particle *b, float rest_length, float stiffness)
{
    float dx = b->position.x - a->position.x;
    float dy = b->position.y - a->position.y;

    float current_length = sqrt(dx*dx + dy*dy);

    if(current_length <= 0) return;

    float nx = dx/current_length;
    float ny = dy/current_length;

    float error =  current_length - rest_length;
    float correctionx = nx * 0.5 * error * stiffness;
    float correctiony = ny * 0.5 * error * stiffness;
    
    // ------- optimized calculation ------------
    //float diff = (current_length - rest_length) / current_length;
    //float correctionx = dx * 0.5 * diff;
    //float correctiony = dy * 0.5 * diff;

    if(!a->is_pinned) {
        a->position.x += correctionx;
        a->position.y += correctiony;
    }
    if(!b->is_pinned) {
        b->position.x -= correctionx;
        b->position.y -= correctiony;
    }
}


void generate_particles(Particle particles[], int width, int height, Vector2 position, 
float rest_length, float mass)
{ 
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            int idx = x + (y*width);
            Vector2 pos = {position.x+(rest_length*x), position.y+(rest_length*y)};
            Particle new = {
                .position = pos,
                .previous_position = pos,
                .acceleration = (Vector2){0,0},
                .mass = mass,
                .is_pinned = false
            };
            if(y == 0) new.is_pinned = true;
            particles[idx] = new;
        }
    }
}
void generate_spring_grid(Spring springs[], Particle particles[], int width, int height)
{
    int count = 0;
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            if(count >= TOTAL_SPRINGS) return;
            int currentIndex = x + (y*width);
            // ------- Structural Springs ---------
            // Right neighbor
            if (x < width - 1) {
                int neighborIndex = (x + 1) + (y * width);

                float dx = particles[neighborIndex].position.x - particles[currentIndex].position.x;
                float dy = particles[neighborIndex].position.y - particles[currentIndex].position.y;
                float rest_length = sqrt(dx*dx + dy*dy);

                springs[count++] = (Spring){
                        .particle_A  = currentIndex, 
                        .particle_B  = neighborIndex,
                        .rest_length = rest_length,
                        .stiffness   = STRUCT_STIFFNESS
                };
            }
            // Down neighbor
            if (y < height - 1) {
                int neighborIndex = x + ((y + 1) * width);

                float dx = particles[neighborIndex].position.x - particles[currentIndex].position.x;
                float dy = particles[neighborIndex].position.y - particles[currentIndex].position.y;
                float rest_length = sqrt(dx*dx + dy*dy);

                springs[count++] = (Spring){
                        .particle_A  = currentIndex, 
                        .particle_B  = neighborIndex,
                        .rest_length = rest_length,
                        .stiffness   = STRUCT_STIFFNESS
                };
            }
            // -------- Shear Springs ------------
            // Down-Right
            if (x < width - 1 && y < height - 1) {
                int neighborIndex = (x + 1) + ((y + 1) * width);
                
                float dx = particles[neighborIndex].position.x - particles[currentIndex].position.x;
                float dy = particles[neighborIndex].position.y - particles[currentIndex].position.y;
                float rest_length = sqrt(dx*dx + dy*dy);

                springs[count++] = (Spring){
                        .particle_A  = currentIndex, 
                        .particle_B  = neighborIndex,
                        .rest_length = rest_length,
                        .stiffness = SHEAR_STIFFNESS
                };
            }
            // Down-Left
            if (x > 0 && y < height - 1) {
                int neighborIndex = (x - 1) + ((y + 1) * width);

                float dx = particles[neighborIndex].position.x - particles[currentIndex].position.x;
                float dy = particles[neighborIndex].position.y - particles[currentIndex].position.y;
                float rest_length = sqrt(dx*dx + dy*dy);

                springs[count++] = (Spring){
                        .particle_A  = currentIndex, 
                        .particle_B  = neighborIndex,
                        .rest_length = rest_length,
                        .stiffness = SHEAR_STIFFNESS
                };
            }
            // -------- Bend Springs -----------
            // Two units Right
            if (x < width - 2) {
                int neighborIndex = (x + 2) + (y * width);

                float dx = particles[neighborIndex].position.x - particles[currentIndex].position.x;
                float dy = particles[neighborIndex].position.y - particles[currentIndex].position.y;
                float rest_length = sqrt(dx*dx + dy*dy);

                springs[count++] = (Spring){
                        .particle_A  = currentIndex, 
                        .particle_B  = neighborIndex,
                        .rest_length = rest_length,
                        .stiffness = BEND_STIFFNESS
                };
            }
            // Two units Down
            if (y < height - 2) {
                int neighborIndex = x + ((y + 2) * width);

                float dx = particles[neighborIndex].position.x - particles[currentIndex].position.x;
                float dy = particles[neighborIndex].position.y - particles[currentIndex].position.y;
                float rest_length = sqrt(dx*dx + dy*dy);

                springs[count++] = (Spring){
                        .particle_A  = currentIndex, 
                        .particle_B  = neighborIndex,
                        .rest_length = rest_length,
                        .stiffness = BEND_STIFFNESS
                };
            }
        }
    }
}


